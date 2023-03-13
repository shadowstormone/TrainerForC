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

	// Выводим значение delta
	printf("----------------------------------------------------------------------------------------------\n");
	printf("delta: 0x%p\n", delta);

	// Вывод в консоль содержимое массива bytes
	printf("Bytes array: ");
	for (int i = 0; i < outSize; i++)
	{
		printf("0x%.2X ", bytes[i]);
	}
	printf("\n");

	// Вывод в консоль адресов from и to
	printf("from = patternAddress: 0x%p\n", from);
	printf("to = allocatedAddress: 0x%p\n", to);

	// Вывод в консоль переменной bytes
	printf("bytes: 0x%p\n", bytes);

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
	ULONG scanSize;
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

	printf("baseAddress: 0x%p\n", baseAddress);
	printf("patternAddress: 0x%p\n", patternAddress);
	printf("patchAddress: 0x%p\n", patchAddress);
	printf("originalAddress: 0x%p\n", originalAddress);
	printf("allocatedAddress: 0x%p\n", allocatedAddress);
	printf("originalBytes: 0x%p\n", originalBytes);

	BYTE jmpSize = 0;
	PBYTE jmpBytes = CalculateJumpBytes(originalAddress, allocatedAddress, jmpSize);

	int offset = 0;
	size_t buffer = 64;

	do
	{
		int length = nmd_x86_ldisasm(originalBytes + offset, buffer, is64BitProcess ? NMD_X86_MODE_64 : NMD_X86_MODE_32);
		originalSize += length;
		offset += length;
	} while (originalSize < jmpSize);

	if (jmpSize == originalSize)
	{
		// If the size of the jump and original instruction are the same, we can patch in place
		BYTE jumpSize = 0;
		PBYTE jumpBytes = CalculateJumpBytes(originalAddress, allocatedAddress, jumpSize);
		WriteMem(hProcess, originalAddress, jumpBytes, jumpSize);
		delete[] jumpBytes;
	}
	else
	{
		// Calculate the backward jump to be inserted after the original code
		BYTE backJmpSize = 0;
		PBYTE backJmpBytes = CalculateJumpBytes(
			reinterpret_cast<PBYTE>(allocatedAddress) + patchSize + originalSize,
			reinterpret_cast<PBYTE>(originalAddress) + jmpSize,backJmpSize);

		if (backJmpBytes == nullptr)
		{
			// Failed to allocate memory for the backward jump, return an error
			std::cerr << "Failed to allocate memory for the backward jump." << std::endl;
			return false;
		}

		// Determine the size of the cave needed to store patch, original, and backward jump
		int caveSize = patchSize + originalSize + backJmpSize;
		PBYTE caveBytes = new BYTE[caveSize];

		// Copy the patch, original, and backward jump into the cave buffer
		memcpy_s(caveBytes, patchSize, patchBytes, patchSize);
		memcpy_s(caveBytes + patchSize, originalSize, originalBytes, originalSize);
		memcpy_s(caveBytes + patchSize + originalSize, backJmpSize, backJmpBytes, backJmpSize);

		// Write the contents of the cave buffer to the allocated memory address
		WriteMem(hProcess, allocatedAddress, caveBytes, caveSize);

		// Free the cave buffer
		delete[] caveBytes;

		// Free the backward jump buffer
		delete[] backJmpBytes;

		// Insert "NOP" instructions after the jump instruction (if necessary)
		size_t nops = originalSize - jmpSize;
		if (nops)
		{
			PBYTE bytes = new BYTE[originalSize];

			// Copy the jump bytes and insert "NOP" instructions in the remaining space
			memcpy_s(bytes, jmpSize, jmpBytes, jmpSize);
			memset(bytes + jmpSize, 0x90, nops);

			// Write the modified bytes to the original memory address
			WriteMem(hProcess, originalAddress, bytes, originalSize);

			// Free the temporary buffer
			delete[] bytes;
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
