#include "cheats/CheatOption.h"

#include <exception>
#include <format>

#include "core/MemoryAccess.h"
#include "patches/CavePatch.h"
#include "patches/NopPatch.h"
#include "patches/WriteAddressPatch.h"
#include "platform/AudioService.h"
#include "platform/Logger.h"
#include "platform/Utils.h"

namespace
{
    // Сколько горит переключатель разовой записи, если автор не задал время.
    constexpr int DEFAULT_ONE_SHOT_FLASH_MS = 400;

    long long NowTicks()
    {
        return CheatOption::Clock::now().time_since_epoch().count();
    }
}

CheatOption::CheatOption(std::wstring description, std::vector<int> keys, std::wstring moduleName)
    : m_description(std::move(description))
    , m_moduleName(std::move(moduleName))
    , m_keys(std::move(keys))
{
    m_hotkey.SetKeys(m_keys);
}

CheatOption::~CheatOption() = default;

void CheatOption::AddPatch(std::unique_ptr<IPatch> patch)
{
    if (patch) m_patches.push_back(std::move(patch));
}

CheatOption* CheatOption::AddNopPatch(LPCWSTR signature, SIZE_T length, std::ptrdiff_t offset)
{
    AddPatch(std::make_unique<NopPatch>(this, signature, length, offset));
    return this;
}

CheatOption* CheatOption::AddCavePatch(LPCWSTR signature, const BYTE* bytes, SIZE_T size,
                                       CaveMode mode, bool preserveRegisters, std::ptrdiff_t offset)
{
    AddPatch(std::make_unique<CavePatch>(this, signature, bytes, size, mode, preserveRegisters, offset));
    return this;
}

CheatOption* CheatOption::AddCavePatchAsm(LPCWSTR signature, std::string asmText, AsmSyntax syntax,
                                          CaveMode mode, bool preserveRegisters, std::ptrdiff_t offset)
{
    AddPatch(std::make_unique<CavePatch>(this, signature, std::move(asmText), syntax,
                                         mode, preserveRegisters, offset));
    return this;
}

CheatOption* CheatOption::AddWriteValuePatch(std::vector<uintptr_t> offsets, PatchValue value,
                                             bool absolute, WriteAddressPatch::Mode mode,
                                             std::wstring module)
{
    AddPatch(std::make_unique<WriteAddressPatch>(this, std::move(offsets), value, absolute,
                                                 mode, std::move(module)));
    return this;
}

bool CheatOption::IsOneShot() const
{
    if (m_patches.empty()) return false;

    for (const auto& patch : m_patches)
    {
        if (!patch->IsOneShot()) return false;
    }
    return true;
}

bool CheatOption::HasFreeze() const
{
    for (const auto& patch : m_patches)
    {
        if (auto* write = dynamic_cast<const WriteAddressPatch*>(patch.get()); write && !write->IsOneShot())
            return true;
    }
    return false;
}

std::string CheatOption::LastError() const
{
    std::lock_guard lock(m_errorMutex);
    return m_lastError;
}

void CheatOption::SetLastError(std::string error)
{
    std::lock_guard lock(m_errorMutex);
    m_lastError = std::move(error);
}

float CheatOption::SecondsSinceFailure() const
{
    const long long failed = m_failedAtTicks.load();
    if (failed == 0) return -1.0f;

    const auto elapsed = Clock::now() - Clock::time_point(Clock::duration(failed));
    return std::chrono::duration<float>(elapsed).count();
}

bool CheatOption::ApplyAll(MemoryAccess& mem)
{
    // Патч может бросить исключение или вернуть false. Раньше при отказе
    // одного патча уже применённые оставались в памяти игры, а опция
    // считалась выключенной — откатить их потом было уже некому.
    std::size_t applied = 0;
    std::string error;

    for (; applied < m_patches.size(); ++applied)
    {
        IPatch& patch = *m_patches[applied];

        bool ok = false;
        try
        {
            ok = patch.Apply(mem);
        }
        catch (const std::exception& e)
        {
            error = std::string("исключение: ") + e.what();
        }

        if (!ok)
        {
            if (error.empty()) error = patch.LastError();
            if (error.empty()) error = "патч не применился";
            break;
        }
    }

    if (applied == m_patches.size()) return true;

    // Откатываем то, что успело встать, в обратном порядке.
    for (std::size_t i = applied; i-- > 0; )
    {
        try
        {
            m_patches[i]->Restore(mem);
        }
        catch (const std::exception& e)
        {
            Log::Error(std::string("Откат патча не удался: ") + e.what());
        }
    }

    SetLastError(m_patches.size() > 1
        ? std::format("Патч {} из {}: {}", applied + 1, m_patches.size(), error)
        : error);
    return false;
}

bool CheatOption::RestoreAll(MemoryAccess& mem)
{
    bool restored = true;

    // В обратном порядке — зеркально применению.
    for (std::size_t i = m_patches.size(); i-- > 0; )
    {
        try
        {
            if (!m_patches[i]->Restore(mem)) restored = false;
        }
        catch (const std::exception& e)
        {
            Log::Error(std::string("Откат патча не удался: ") + e.what());
            restored = false;
        }
    }

    return restored;
}

bool CheatOption::SetEnabled(bool enabled, DWORD pid)
{
    std::lock_guard lock(m_mutex);

    const std::string name = Utils::WStringToUtf8(m_description);

    const bool oneShot = IsOneShot();

    if (!enabled)
    {
        m_autoOffPending = false;
        if (!m_enabled) return true;

        // Разовая запись уже сделана — выключать в памяти нечего.
        if (oneShot)
        {
            m_enabled = false;
            return true;
        }

        MemoryAccess mem(pid);
        bool restored = true;

        if (mem.IsValid())
        {
            restored = RestoreAll(mem);
        }
        else
        {
            // Процесса уже нет — откатывать некуда, но патчи должны забыть
            // своё состояние, иначе в новом процессе считали бы себя
            // уже применёнными.
            for (const auto& patch : m_patches) patch->Reset();
        }

        m_enabled = false;

        if (!restored) Log::Error("Выключено с ошибками: " + name);
        else           Log::Info("Выключено: " + name);

        AudioService::Instance().Play(Sound::CheatDisabled);
        return true;
    }

    if (m_enabled && !oneShot) return true;

    const auto fail = [&](std::string reason)
    {
        Log::Error("Не включилось «" + name + "»: " + reason);
        SetLastError(std::move(reason));
        m_failedAtTicks = NowTicks();
        AudioService::Instance().Play(Sound::CheatFailed);
        return false;
    };

    if (pid == 0) return fail("Игра не запущена");

    MemoryAccess mem(pid);
    if (!mem.IsValid()) return fail("Нет доступа к процессу игры — запустите трейнер от администратора");

    if (!ApplyAll(mem)) return fail(LastError());

    SetLastError({});
    m_failedAtTicks = 0;

    AudioService::Instance().Play(Sound::CheatEnabled);

    // Разовая запись — действие, а не режим: переключатель вспыхивает
    // и гаснет сам. Раньше это работало только из UI и только если автор
    // не забыл дописать autoDisable; с горячей клавиши опция молча
    // оставалась «включённой».
    const int autoOff = m_autoOffMs > 0 ? m_autoOffMs : (oneShot ? DEFAULT_ONE_SHOT_FLASH_MS : 0);

    if (oneShot)
    {
        RestoreAll(mem); // снимает флаги применённости, память не трогает
        Log::Info("Выполнено: " + name);
    }
    else
    {
        Log::Info("Включено: " + name);
    }

    m_enabled = true;

    if (autoOff > 0)
    {
        m_autoOffAt = Clock::now() + std::chrono::milliseconds(autoOff);
        m_autoOffPending = true;
    }

    return true;
}

void CheatOption::Process(DWORD pid, MemoryAccess* mem)
{
    if (m_hotkey.JustPressed())
    {
        Toggle(pid);
    }

    if (!m_enabled) return;

    std::unique_lock lock(m_mutex, std::try_to_lock);
    if (!lock.owns_lock()) return; // UI как раз переключает — не мешаем

    // Автовыключение — здесь, в том же потоке, а не отдельным detached-
    // потоком со sleep, как раньше: тот мог проснуться уже после выхода
    // из программы и обратиться к удалённой опции.
    if (m_autoOffPending && Clock::now() >= m_autoOffAt)
    {
        m_autoOffPending = false;

        if (IsOneShot())
        {
            m_enabled = false; // память уже не наша — просто гасим вспышку
        }
        else
        {
            SetEnabled(false, pid);
            Log::Info("Опция была временной и выключена автоматически");
        }
        return;
    }

    if (mem)
    {
        for (const auto& patch : m_patches)
        {
            patch->Tick(*mem);
        }
    }
}

void CheatOption::OnProcessLost()
{
    std::lock_guard lock(m_mutex);

    for (const auto& patch : m_patches)
    {
        patch->Reset();
    }

    m_enabled = false;
    m_autoOffPending = false;
}
