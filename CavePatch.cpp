#include "CavePatch.h"
#include "CheatOption.h"
#include "Memory_Functions.h"
#include <iostream>

#define NMD_ASSEMBLY_IMPLEMENTATION
#include "nmd_assembly.h"


PBYTE CavePatch::CalculateJumpBytes(LPVOID from, LPVOID to, BYTE& outSize, HANDLE hProcess)
{
	PBYTE bytes = nullptr;
	LONG delta = reinterpret_cast<LONG>(to) - reinterpret_cast<LONG>(from);
	bool is64BitProcess = isTargetX64Process(hProcess);

	if (abs(delta) >= INT_MAX)
	{
		std::cerr << "Jump offset too large: " << abs(delta);
		return nullptr;
	}

	if (is64BitProcess)
	{
		bytes = new BYTE[14];
		bytes[0] = 0xFF;
		bytes[1] = 0x25;
		bytes[2] = 0x00;
		bytes[3] = 0x00;
		bytes[4] = 0x00;
		bytes[5] = 0x00;
		memcpy_s(bytes + 6, 8, &to, 8);
		outSize = 14;
	}
	else
	{
		bytes = new BYTE[5];
		bytes[0] = 0xe9;
		delta -= 5;
		memcpy_s(bytes + 1, 4, &delta, 4);
		outSize = 5;
	}
	return bytes;
}


bool CavePatch::Hack(HANDLE hProcess)
{
	constexpr ULONG MAX_INT_SIZE_32_BIT = 0x7FFFFFFF;
	constexpr ULONG MAX_INT_SIZE_64_BIT = 0x7FFFFFFFFFFF;

	bool is64BitProcess = isTargetX64Process(hProcess);
	ULONG scanSize = is64BitProcess ? MAX_INT_SIZE_64_BIT : MAX_INT_SIZE_32_BIT;
	DWORD_PTR baseAddress = parent->GetModuleName() && wcslen(parent->GetModuleName()) > 0 ? 
		GetModuleBaseAddress(hProcess, parent->GetModuleName()) : GetProcessBaseAddress(hProcess);

	originalAddress = reinterpret_cast<LPVOID>(reinterpret_cast<PBYTE>(ScanSignature(hProcess, baseAddress, scanSize, pattern, mask)) + patchOffset);
	originalBytes = reinterpret_cast<PBYTE>(ReadMem(hProcess, originalAddress, 32));
	allocatedAddress = AllocMem(hProcess, NULL, 4096);

	BYTE jmpSize = 0;
	PBYTE jmpBytes = CalculateJumpBytes(originalAddress, allocatedAddress, jmpSize, hProcess);

	size_t offset = 0;
	size_t buffer = 32;

	do
	{
		size_t length = nmd_x86_ldisasm(originalBytes + offset, buffer, is64BitProcess ? NMD_X86_MODE_64 : NMD_X86_MODE_32);
		originalSize += length;
		offset += length;
	} while (originalSize < jmpSize);

	BYTE backJmpSize = 0;
	PBYTE backJmpBytes = CalculateJumpBytes(reinterpret_cast<PBYTE>(allocatedAddress) + patchSize + originalSize, reinterpret_cast<PBYTE>(originalAddress) + jmpSize, backJmpSize, hProcess);

	size_t caveSize = patchSize + originalSize + backJmpSize;
	PBYTE caveBytes = new BYTE[caveSize];
	memcpy_s(caveBytes, patchSize, patchBytes, patchSize);
	memcpy_s(caveBytes + patchSize, originalSize, originalBytes, originalSize);
	memcpy_s(caveBytes + patchSize + originalSize, backJmpSize, backJmpBytes, backJmpSize);

	WriteMem(hProcess, allocatedAddress, caveBytes, caveSize);
	delete[] caveBytes;

	size_t nops = originalSize - jmpSize;

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
