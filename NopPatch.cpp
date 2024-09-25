#include "NopPatch.h"
#include "Memory_Functions.h"
#include "CheatOption.h"

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

    originalAddress = ScanSignature(hProcess, baseAddress, scanSize, pattern, mask);
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
	WriteMem(hProcess, originalAddress, originalBytes, patchSize);
	return false;
}
