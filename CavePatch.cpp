#include "CavePatch.h"
#include "CheatOption.h"
#include "Memory_Functions.h"
#include <iostream>

#define NMD_ASSEMBLY_IMPLEMENTATION
#include "nmd_assembly.h"

PBYTE CavePatch::CalculateJumpBytes(LPVOID from, LPVOID to, BYTE& outSize)
{
	PBYTE bytes = nullptr;
	LONG_PTR delta = reinterpret_cast<LONG_PTR>(to) - reinterpret_cast<LONG_PTR>(from);

	if (abs(delta) < 2147483648)
	{
		bytes = new BYTE[5];
		bytes[0] = 0xE9;
		delta -= 5;
		memcpy_s(bytes + 1, 4, &delta, 4);
		outSize = 5;
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
		outSize = 14;
	}
	/*printf("----------------------------------------------------------------------------------------------\n");
	printf("delta: 0x%p\n", delta);


	printf("Bytes array: ");
	for (int i = 0; i < outSize; i++)
	{
		printf("0x%.2X ", bytes[i]);
	}
	printf("\n");

	printf("from = patternAddress: 0x%p\n", from);
	printf("to = allocatedAddress: 0x%p\n", to);

	printf("bytes: 0x%p\n", bytes);*/
	return bytes;
}

//PBYTE CavePatch::CalculateJumpBytes(LPVOID from, LPVOID to, BYTE& outSize, HANDLE hProcess)
//{
//	bool is64BitProcess = isTargetX64Process(hProcess);
//	std::vector<BYTE> bytes;
//
//	if (is64BitProcess)
//	{
//		bytes.reserve(14);
//		bytes.emplace_back(0xFF);
//		bytes.emplace_back(0x25);
//		bytes.insert(bytes.end(), 4, 0x00);
//		bytes.insert(bytes.end(), reinterpret_cast<PBYTE>(&to), reinterpret_cast<PBYTE>(&to) + sizeof(to));
//		outSize = static_cast<BYTE>(bytes.size());
//	}
//	else
//	{
//		bytes.reserve(5);
//		bytes.emplace_back(0xE9);
//		DWORD offset = reinterpret_cast<DWORD>(to) - (reinterpret_cast<DWORD>(from) + 5);
//		bytes.insert(bytes.end(), reinterpret_cast<PBYTE>(&offset), reinterpret_cast<PBYTE>(&offset) + sizeof(offset));
//		outSize = static_cast<BYTE>(bytes.size());
//	}
//
//	// Allocate a new buffer and copy the bytes
//	PBYTE jumpBytes = new BYTE[outSize];
//	std::copy(bytes.begin(), bytes.end(), jumpBytes);
//
//	// Output the contents of the vector to the console
//	std::cout << "Jump bytes:";
//	for (const auto& byte : bytes)
//	{
//		std::cout << " 0x" << std::hex << static_cast<int>(byte);
//	}
//	std::cout << std::endl;
//
//	return jumpBytes;
//	/*PBYTE bytes = nullptr;
//	LONG delta = reinterpret_cast<LONG>(to) - reinterpret_cast<LONG>(from);
//	bool is64BitProcess = isTargetX64Process(hProcess);
//
//	if (abs(delta) >= INT_MAX)
//	{
//		std::cerr << "Jump offset too large: " << abs(delta);
//		return nullptr;
//	}
//
//	if (is64BitProcess)
//	{
//		bytes = new BYTE[14];
//		bytes[0] = 0xFF;
//		bytes[1] = 0x25;
//		bytes[2] = 0x00;
//		bytes[3] = 0x00;
//		bytes[4] = 0x00;
//		bytes[5] = 0x00;
//		memcpy_s(bytes + 6, 8, &to, 8);
//		outSize = 14;
//	}
//	else
//	{
//		bytes = new BYTE[5];
//		bytes[0] = 0xe9;
//		delta -= 5;
//		memcpy_s(bytes + 1, 4, &delta, 4);
//		outSize = 5;
//	}
//	return bytes;*/
//}

bool CavePatch::Hack(HANDLE hProcess)
{
	bool is64BitProcess = isTargetX64Process(hProcess);
	unsigned __int64 scanSize;
	DWORD_PTR baseAddress;

	if (is64BitProcess)
	{
		scanSize = 0x7FFFFFFFFFFFFFFF;
	}
	else
	{
		scanSize = 0x7FFFFFFF;
	}

	if (parent->GetModuleName() && wcslen(parent->GetModuleName()) > 0)
	{
		baseAddress = GetModuleBaseAddress(hProcess, parent->GetModuleName());
	}
	else
	{
		baseAddress = GetProcessBaseAddress(hProcess);
	}

	patternAddress = ScanSignature(hProcess, baseAddress, scanSize, pattern, mask);
	patchAddress = reinterpret_cast<LPBYTE>(patternAddress) + patchOffset;
	originalAddress = reinterpret_cast<LPVOID>(patchAddress);
	originalBytes = reinterpret_cast<PBYTE>(ReadMem(hProcess, originalAddress, 15));
	allocatedAddress = AllocMem(hProcess, NULL, 4096);

	BYTE jmpSize = 0;
	PBYTE jmpBytes = CalculateJumpBytes(originalAddress, allocatedAddress, jmpSize);

	size_t offset = 0;
	size_t buffer = 64;

	do
	{
		size_t length = nmd_x86_ldisasm(originalBytes + offset, buffer, is64BitProcess ? NMD_X86_MODE_64 : NMD_X86_MODE_32);
		originalSize += length;
		offset += length;
	} while (originalSize < jmpSize);

	if (originalSize > patchSize)
	{
		std::cerr << "Original instructions too large to fit in the cave." << std::endl;
		return false;
	}

	if (jmpSize == originalSize)
	{
		BYTE jumpSize = 0;
		PBYTE jumpBytes = CalculateJumpBytes(originalAddress, allocatedAddress, jumpSize);

		WriteMem(hProcess, originalAddress, jumpBytes, jumpSize);

		delete[] jumpBytes;
	}
	else
	{
		BYTE backJmpSize = 0;
		PBYTE backJmpBytes = CalculateJumpBytes(
			reinterpret_cast<PBYTE>(allocatedAddress) + caveSize + patchSize,
			reinterpret_cast<PBYTE>(originalAddress) + originalSize,
			backJmpSize);

		if (backJmpBytes == nullptr)
		{
			std::cerr << "Failed to allocate memory for the backward jump." << std::endl;
			return false;
		}

		size_t caveSize = patchSize + backJmpSize;
		PBYTE caveBytes = new BYTE[caveSize];

		memcpy_s(caveBytes, patchSize, patchBytes, patchSize);
		memcpy_s(caveBytes + patchSize, backJmpSize, backJmpBytes, backJmpSize);
		WriteMem(hProcess, allocatedAddress, caveBytes, caveSize);

		delete[] caveBytes;
		delete[] backJmpBytes;

		size_t nops = originalSize - jmpSize;
		if (nops > 0)
		{
			PBYTE bytes = new BYTE[originalSize];

			memcpy_s(bytes, jmpSize, jmpBytes, jmpSize);
			memset(bytes + jmpSize, 0x90, nops);
			memcpy_s(bytes + jmpSize + nops, originalSize - jmpSize - nops, originalBytes + jmpSize, originalSize - jmpSize - nops);

			WriteMem(hProcess, originalAddress, bytes, originalSize);

			delete[] bytes;
		}
		else
		{
			WriteMem(hProcess, originalAddress, jmpBytes, jmpSize);
		}
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
