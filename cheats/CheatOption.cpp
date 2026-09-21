#include <thread>
#include <atomic>
#include <exception>
#include <windows.h>
#include "core/MemoryAccess.h"
#include "cheats/CheatOption.h"
#include "patches/NopPatch.h"
#include "patches/CavePatch.h"
#include "platform/AudioService.h"
#include "resource.h"
#include "patches/WriteAddressPatch.h"

bool CheatOption::Enable(int pid)
{
    MemoryAccess mem(static_cast<DWORD>(pid));
    if (!mem.IsValid())
        return false;

    // Патч может бросить исключение (сигнатура не найдена, инструкция не
    // разобралась) или вернуть false. Раньше результат игнорировался, а
    // исключение улетало прямо в кадр UI — отсюда и "опция активна, хотя
    // ничего не применилось", и падения интерфейса.
    bool applied = true;
    for (auto& p : patches)
    {
        try
        {
            if (!p->Apply(mem)) applied = false;
        }
        catch (const std::exception&)
        {
            applied = false;
        }
    }

    if (!applied) return false;

    AudioService::Instance().Play(Sound::CheatEnabled);
    return true;
}

bool CheatOption::Disable(int pid)
{
    MemoryAccess mem(static_cast<DWORD>(pid));
    if (!mem.IsValid())
        return false;

    bool restored = true;
    for (auto& p : patches)
    {
        try
        {
            if (!p->Restore(mem)) restored = false;
        }
        catch (const std::exception&)
        {
            restored = false;
        }
    }

    AudioService::Instance().Play(Sound::CheatDisabled);
    return restored;
}

CheatOption* CheatOption::AddNopPatch(LPCWSTR signature, SIZE_T pSize)
{
    patches.push_back(std::make_unique<NopPatch>(this, signature, pSize));
    return this;
}

CheatOption* CheatOption::AddCavePatch(LPCWSTR signature, PBYTE pBytes, SIZE_T patchSize,
                                       CaveMode mode, bool preserveRegisters)
{
    patches.push_back(std::make_unique<CavePatch>(
        this, signature, pBytes, static_cast<int>(patchSize), mode, preserveRegisters));
    return this;
}

CheatOption* CheatOption::AddWriteValuePatch(Cheat* cheatProcess, std::vector<uintptr_t> offsets, int value, bool absolute)
{
    LPCWSTR processName = cheatProcess->GetProcessName();
    patches.push_back(std::make_unique<WriteAddressPatch>(this, processName, offsets, value, absolute));
    return this;
}

CheatOption* CheatOption::AddWriteValuePatch(Cheat* cheatProcess, std::vector<uintptr_t> offsets, float value, bool absolute)
{
    LPCWSTR processName = cheatProcess->GetProcessName();
    patches.push_back(std::make_unique<WriteAddressPatch>(this, processName, offsets, value, absolute));
    return this;
}

CheatOption* CheatOption::AddWriteValuePatch(Cheat* cheatProcess, std::vector<uintptr_t> offsets, double value, bool absolute)
{
    LPCWSTR processName = cheatProcess->GetProcessName();
    patches.push_back(std::make_unique<WriteAddressPatch>(this, processName, offsets, value, absolute));
    return this;
}

void CheatOption::Process(int processId)
{
    // Состояние антидребезга живёт в самой комбинации, а не в функциональном
    // статике: раньше флаг был общим для всех опций, и ненажатые опции
    // сбрасывали его каждому тику. Блокирующий sleep тоже убран — он вешал
    // общий поток обработки на 200 мс для всех читов сразу.
    if (!m_hotkey.JustPressed()) return;

    if (m_enabled)
    {
        if (Disable(processId))
        {
            m_enabled = false;
        }
        return;
    }

    bool addressPatchApplied = false;
    for (auto& patch : patches)
    {
        if (auto* writePatch = dynamic_cast<WriteAddressPatch*>(patch.get()))
        {
            if (!writePatch->IsApplied())
            {
                if (Enable(processId))
                {
                    addressPatchApplied = true;
                    break;
                }
            }
        }
    }

    if (addressPatchApplied)
    {
        // WriteAddressPatch — одноразовая запись значения, поэтому сразу
        // снимаем флаг применённости, чтобы горячая клавиша сработала снова.
        MemoryAccess mem(static_cast<DWORD>(processId));
        for (auto& patch : patches)
        {
            if (auto* writePatch = dynamic_cast<WriteAddressPatch*>(patch.get()))
            {
                writePatch->Restore(mem);
                break;
            }
        }
    }
    else if (Enable(processId))
    {
        m_enabled = true;
    }
}
