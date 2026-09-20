#include "patches/NopPatch.h"
#include "cheats/CheatOption.h"

bool NopPatch::Apply(MemoryAccess& mem)
{
    if (!mem.IsValid())
    {
        throw std::invalid_argument("Invalid process handle");
    }

    ULONG_PTR scanSize = mem.IsTargetX64() ? 0x7FFFFFFFFFFFFFFF : 0x7FFFFFFF;

    DWORD_PTR baseAddress = parent->GetModuleName() && wcslen(parent->GetModuleName()) > 0 ? mem.ModuleBase(parent->GetModuleName()) : mem.ProcessBase();
    if (baseAddress == 0)
    {
        throw std::runtime_error("Failed to get process/module base address");
    }

    originalAddress = mem.ScanSignature(baseAddress, scanSize, pattern.data(), mask);
    if (originalAddress == 0)
    {
        throw std::runtime_error("Failed to find signature");
    }

    originalBytes = reinterpret_cast<PBYTE>(mem.Read(originalAddress, patchSize));
    if (originalBytes == NULL)
    {
        throw std::runtime_error("Failed to read original bytes");
    }

    PBYTE patchBytes = new BYTE[patchSize];
    memset(patchBytes, 0x90, patchSize);
    mem.Write(originalAddress, patchBytes, patchSize);

    delete[] patchBytes;
    return true;
}

bool NopPatch::Restore(MemoryAccess& mem)
{
    if (mem.IsValid() && originalAddress && originalAddress != INVALID_HANDLE_VALUE)
    {
        // Проверяем, что процесс еще активен
        DWORD exitCode;
        if (GetExitCodeProcess(mem.Handle(), &exitCode) && exitCode == STILL_ACTIVE)
        {
            // Проверяем доступность памяти
            MEMORY_BASIC_INFORMATION mbi;
            if (VirtualQueryEx(mem.Handle(), (LPCVOID)originalAddress, &mbi, sizeof(mbi)) != 0)
            {
                if (mbi.State == MEM_COMMIT)
                {
                    mem.Write(originalAddress, originalBytes, patchSize);
                }
            }
        }
    }
    return true; // Изменил на true для корректного возврата успеха
}
