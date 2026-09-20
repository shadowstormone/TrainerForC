#include <thread>
#include <atomic>
#include <windows.h>
#include <mmsystem.h>
#include "core/Memory_Functions.h"
#include "core/MemoryAccess.h"
#include "cheats/CheatOption.h"
#include "patches/NopPatch.h"
#include "patches/CavePatch.h"
#include "resource.h"
#include "patches/WriteAddressPatch.h"

#pragma comment(lib, "Winmm.lib")

bool CheatOption::Enable(int pid)
{
    MemoryAccess mem(static_cast<DWORD>(pid));
    if (!mem.IsValid())
        return false;

    for (auto& p : patches)
        p->Hack(mem.Handle());

    std::thread([]() { PlaySound(MAKEINTRESOURCE(IDR_WAVE1), nullptr, SND_RESOURCE | SND_ASYNC | SND_NODEFAULT); }).detach();
    return true;
}

bool CheatOption::Disable(int pid)
{
    MemoryAccess mem(static_cast<DWORD>(pid));
    if (!mem.IsValid())
        return false;

    for (auto& p : patches)
        p->Restore(mem.Handle());

    std::thread([]() { PlaySound(MAKEINTRESOURCE(IDR_WAVE2), nullptr, SND_RESOURCE | SND_ASYNC | SND_NODEFAULT); }).detach();
    return true;
}

bool CheatOption::KeyPressed()
{
    for (int key : m_keys)
    {
        if (!(GetAsyncKeyState(key) & 0x8000))
        {
            return false;
        }
    }
    return true;
}

CheatOption* CheatOption::AddNopPatch(LPCWSTR signature, SIZE_T pSize)
{
    patches.push_back(std::make_unique<NopPatch>(this, signature, pSize));
    return this;
}

CheatOption* CheatOption::AddCavePatch(LPCWSTR signature, PBYTE pBytes, SIZE_T patchSize)
{
    patches.push_back(std::make_unique<CavePatch>(this, signature, pBytes, static_cast<int>(patchSize)));
    return this;
}

CheatOption* CheatOption::AddWriteValuePatch(Cheat* cheatProcess, std::vector<uintptr_t> offsets, int value)
{
    LPCWSTR processName = cheatProcess->GetProcessName();
    patches.push_back(std::make_unique<WriteAddressPatch>(this, processName, offsets, value));
    return this;
}

CheatOption* CheatOption::AddWriteValuePatch(Cheat* cheatProcess, std::vector<uintptr_t> offsets, float value)
{
    LPCWSTR processName = cheatProcess->GetProcessName();
    patches.push_back(std::make_unique<WriteAddressPatch>(this, processName, offsets, value));
    return this;
}

CheatOption* CheatOption::AddWriteValuePatch(Cheat* cheatProcess, std::vector<uintptr_t> offsets, double value)
{
    LPCWSTR processName = cheatProcess->GetProcessName();
    patches.push_back(std::make_unique<WriteAddressPatch>(this, processName, offsets, value));
    return this;
}

void CheatOption::Process(int processId)
{
    static bool keyWasPressed = false; // Флаг, отслеживающий состояние клавиши

    if (KeyPressed())
    {
        if (!keyWasPressed) // Если клавиша была не нажата до этого
        {
            keyWasPressed = true; // Устанавливаем флаг, что клавиша нажата

            if (m_enabled)
            {
                if (Disable(processId))
                {
                    m_enabled = false;
                }
            }
            else
            {
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
                    // Сразу сбрасываем флаг WriteAddressPatch (Restore не требует handle)
                    for (auto& patch : patches)
                    {
                        if (auto* writePatch = dynamic_cast<WriteAddressPatch*>(patch.get()))
                        {
                            writePatch->Restore(nullptr);
                            break;
                        }
                    }
                }
                else if (Enable(processId))
                {
                    m_enabled = true;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Задержка для предотвращения многократного срабатывания
        }
    }
    else
    {
        keyWasPressed = false; // Сбрасываем флаг, если клавиша отпущена
    }
}

