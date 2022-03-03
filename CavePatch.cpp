#include "CavePatch.h"
#include "CheatOption.h"
#include "Memory_Functions.h"

#define NMD_ASSEMBLY_IMPLEMENTATION
#include "nmd_assembly.h"

PBYTE CavePatch::CalculateJumpBytes(LPVOID from, LPVOID to, BYTE* outSize)
{
	PBYTE bytes = NULL;
	LONG delta = reinterpret_cast<LONG>(to) - reinterpret_cast<LONG>(from);

	if (abs(delta) < 2147483648)
	{
		bytes = new BYTE[5];
		bytes[0] = 0xe9;
		delta -= 5;
		memcpy_s(bytes + 1, 4, &delta, 4);
		*outSize = 5;
	}
	else
	{
		bytes = new BYTE[14];
		bytes[0] = 0xFF;
		bytes[1] = 0x25;
		bytes[2] = 0x00;
		bytes[3] = 0x00;
		bytes[4] = 0x00;
		bytes[5] = 0x00;
		memcpy_s(bytes + 6, 8, &to, 8);
		*outSize = 14;
	}

	return bytes;
}

bool CavePatch::Hack(HANDLE hProcess)
{
	bool x64 = isTargetX64Process(hProcess);
	ULONG scanSize = x64 ? 0x7FFFFFFFFFFFFFFF : 0x7FFFFFFF;
	DWORD_PTR baseAddress = parent->GetModuleName() && wcslen(parent->GetModuleName()) > 0 ? 
		GetModuleBaseAddress(hProcess, parent->GetModuleName()) : GetProcessBaseAddress(hProcess);

	originalAddress = reinterpret_cast<LPVOID>(reinterpret_cast<PBYTE>(ScanSignature(hProcess, baseAddress, scanSize, pattern, mask)) + patchOffset);
	originalBytes = reinterpret_cast<PBYTE>(ReadMem(hProcess, originalAddress, 32));
	allocatedAddress = AllocMem(hProcess, NULL, 4096);

	BYTE jmpSize = 0;
	PBYTE jmpBytes = CalculateJumpBytes(originalAddress, allocatedAddress, &jmpSize);

	
	int offset = 0;
	size_t buffer = 32;

	do
	{
		int length = nmd_x86_ldisasm(originalBytes + offset, buffer, x64 ? NMD_X86_MODE_64 : NMD_X86_MODE_32);
		originalSize += length;
		offset += length;
	} while (originalSize < jmpSize);

	BYTE backJmpSize = 0;
	PBYTE backJmpBytes = CalculateJumpBytes(reinterpret_cast<PBYTE>(allocatedAddress) + patchSize + originalSize, reinterpret_cast<PBYTE>(originalAddress) + jmpSize, &backJmpSize);

	int caveSize = patchSize + originalSize + backJmpSize;
	PBYTE caveBytes = new BYTE[caveSize];
	memcpy_s(caveBytes, patchSize, patchBytes, patchSize);
	memcpy_s(caveBytes + patchSize, originalSize, originalBytes, originalSize);
	memcpy_s(caveBytes + patchSize + originalSize, backJmpSize, backJmpBytes, backJmpSize);

	WriteMem(hProcess, allocatedAddress, caveBytes, caveSize);
	delete[] caveBytes;

	int nops = originalSize - jmpSize;

	if (nops)
	{
		PBYTE bytes = new BYTE[originalSize];
		memcpy_s(bytes, jmpSize, jmpBytes, jmpSize);
		memset(bytes + jmpSize, 0x90, nops);
		WriteMem(hProcess, originalAddress, bytes, originalSize);
		delete[] bytes;
	}

	return true;
}

bool CavePatch::Restore(HANDLE hProcess)
{
	WriteMem(hProcess, originalAddress, originalBytes, originalSize);
	FreeMem(hProcess, allocatedAddress, 4096);
	originalSize = 0;
	return true;
}
