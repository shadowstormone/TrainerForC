#include "patches/WriteAddressPatch.h"

#include <format>

#include "cheats/CheatOption.h"
#include "platform/Logger.h"
#include "platform/Utils.h"

std::size_t WriteAddressPatch::ValueSize(const PatchValue& value)
{
    return std::visit([](const auto& v) { return sizeof(v); }, value);
}

std::uintptr_t WriteAddressPatch::ResolveAddress(MemoryAccess& mem)
{
    if (m_offsets.empty()) return 0;

    // Адрес уже абсолютный — база не прибавляется.
    if (m_absolute) return m_offsets.back();

    const std::wstring module = !m_module.empty() ? m_module
                              : m_parent ? m_parent->GetModuleName()
                              : std::wstring();

    if (m_basePid != mem.Pid() || m_base == 0)
    {
        m_base = mem.ModuleOrMain(module).base;
        m_basePid = mem.Pid();
    }
    if (m_base == 0) return 0;

    return mem.ResolveChain(m_base, m_offsets);
}

bool WriteAddressPatch::WriteNow(MemoryAccess& mem, bool logErrors)
{
    const std::uintptr_t address = ResolveAddress(mem);
    if (address == 0)
    {
        m_lastError = "Адрес не вычислился: цепочка указателей оборвалась "
                      "(в игре ещё не загружен нужный объект?)";
        if (logErrors) Log::Error(m_lastError);
        return false;
    }

    const bool ok = std::visit([&](const auto& v) { return mem.WriteValue(address, v); }, m_value);
    if (!ok)
    {
        m_lastError = std::format("Не удалось записать значение по адресу 0x{:X}", address);
        if (logErrors) Log::Error(m_lastError);
        return false;
    }

#ifdef _DEBUG
    if (logErrors) Log::Debug(std::format("Значение записано по адресу 0x{:X}", address));
#endif

    return true;
}

bool WriteAddressPatch::Apply(MemoryAccess& mem)
{
    m_lastError.clear();

    // Процесс больше не открывается здесь: раньше патч игнорировал
    // переданный дескриптор и делал собственный OpenProcess.
    if (!mem.IsValid())
    {
        m_lastError = "Процесс игры недоступен";
        return false;
    }

    if (!WriteNow(mem, true)) return false;

    m_active = (m_mode == Mode::Freeze);
    return true;
}

void WriteAddressPatch::Tick(MemoryAccess& mem)
{
    if (!m_active) return;

    // Молча: заморозка пишет 60 раз в секунду, и на экране загрузки, где
    // объекта ещё нет, лог утонул бы в одинаковых сообщениях.
    WriteNow(mem, false);
}

bool WriteAddressPatch::Restore(MemoryAccess& mem)
{
    // Прежнее значение не сохраняется намеренно: вернуть здоровье к тому,
    // что было при включении заморозки, — не то, чего ждёт игрок.
    (void)mem;
    m_active = false;
    return true;
}
