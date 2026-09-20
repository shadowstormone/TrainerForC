#include "patches/NopPatch.h"
#include "core/Memory_Functions.h"
#include "cheats/CheatOption.h"

bool NopPatch::Hack(HANDLE hProcess)
{
    if (hProcess == NULL)
    {
        throw std::invalid_argument("Invalid process handle");
    }

    ULONG_PTR scanSize = isTargetX64Process(hProcess) ? 0x7FFFFFFFFFFFFFFF : 0x7FFFFFFF;

    DWORD_PTR baseAddress = parent->GetModuleName() && wcslen(parent->GetModuleName()) > 0 ? GetModuleBaseAddress(hProcess, parent->GetModuleName()) : GetProcessBaseAddress(hProcess);
    if (baseAddress == 0)
    {
        throw std::runtime_error("Failed to get process/module base address");
    }

    originalAddress = ScanSignature(hProcess, baseAddress, scanSize, pattern.data(), mask);
    if (originalAddress == 0)
    {
        throw std::runtime_error("Failed to find signature");
    }

    originalBytes = reinterpret_cast<PBYTE>(ReadMem(hProcess, originalAddress, patchSize));
    if (originalBytes == NULL)
    {
        throw std::runtime_error("Failed to read original bytes");
    }

    PBYTE patchBytes = new BYTE[patchSize];
    memset(patchBytes, 0x90, patchSize);
    WriteMem(hProcess, originalAddress, patchBytes, patchSize);

    delete[] patchBytes;
    return true;
}

bool NopPatch::Restore(HANDLE hProcess)
{
    if (hProcess && originalAddress && originalAddress != INVALID_HANDLE_VALUE)
    {
        // Проверяем, что процесс еще активен
        DWORD exitCode;
        if (GetExitCodeProcess(hProcess, &exitCode) && exitCode == STILL_ACTIVE)
        {
            // Проверяем доступность памяти
            MEMORY_BASIC_INFORMATION mbi;
            if (VirtualQueryEx(hProcess, (LPCVOID)originalAddress, &mbi, sizeof(mbi)) != 0)
            {
                if (mbi.State == MEM_COMMIT)
                {
                    WriteMem(hProcess, originalAddress, originalBytes, patchSize);
                }
            }
        }
    }
    return true; // Изменил на true для корректного возврата успеха
}
