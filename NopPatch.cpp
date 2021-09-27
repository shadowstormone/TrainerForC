#include "NopPatch.h"
#include "Memory_Functions.h"
#include "CheatOption.h"

bool NopPatch::Hack(HANDLE hProcess)
{
	ULONG scanSize = isTargetX64Process(hProcess) ? 0x7FFFFFFFFFFFFFFF : 0x7FFFFFFF;
	DWORD_PTR baseAddress = parent->GetModuleName() && wcslen(parent->GetModuleName()) > 0 ? GetModuleBaseAddress(hProcess, parent->GetModuleName()) : GetProcessBaseAddress(hProcess);
	originalAddress = ScanSignature(hProcess, baseAddress, scanSize, pattern, mask);
	originalBytes = reinterpret_cast<PBYTE>(ReadMem(hProcess, originalAddress, patchSize));
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
