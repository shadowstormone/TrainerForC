#include <thread>
#include <atomic>
#include <windows.h>
#include <mmsystem.h>
#include "Memory_Functions.h"
#include "CheatOption.h"
#include "NopPatch.h"
#include "CavePatch.h"
#include "resource.h"

#pragma comment(lib, "Winmm.lib")

bool CheatOption::Enable(int pid)
{
    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

    if (hProc)
    {
        for (Patch* p : patches)
        {
            p->Hack(hProc);
        }
        CloseHandle(hProc);
        // Воспроизведение звука асинхронно после успешного выполнения
        std::thread([]() { PlaySound(MAKEINTRESOURCE(IDR_WAVE1), NULL, SND_RESOURCE | SND_ASYNC); }).detach();
        return true;
    }
    return false;
}

bool CheatOption::Disable(int pid)
{
    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

    if (hProc)
    {
        for (Patch* p : patches)
        {
            p->Restore(hProc);
        }
        CloseHandle(hProc);
        // Воспроизведение звука асинхронно после успешного выполнения
        std::thread([]() { PlaySound(MAKEINTRESOURCE(IDR_WAVE2), NULL, SND_RESOURCE | SND_ASYNC); }).detach();
        return true;
    }
    return false;
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
    patches.push_back(new NopPatch(this, signature, pSize));
    return this;
}

CheatOption* CheatOption::AddCavePatch(LPCWSTR signature, PBYTE pBytes, SIZE_T patchSize)
{
    patches.push_back(new CavePatch(this, signature, pBytes, static_cast<int>(patchSize)));
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
                if (Enable(processId))
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