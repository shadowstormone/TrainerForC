#pragma once
#include <Windows.h>
#include <vector>
#include <string>
#include <iterator>
#include "Memory_Functions.h"

class CheatOption; // Предварительное объявление класса

class Patch
{
protected:
	PBYTE pattern = NULL;
	std::wstring mask;
	LPVOID originalAddress = 0;
	PBYTE originalBytes = NULL;
	SIZE_T patchSize = 0;
	CheatOption* parent = NULL;
	LPBYTE patchAddress = nullptr;
	LPVOID patternAddress = nullptr;

	// Дополнительные атрибуты для конструктора с processName, offset и value
	std::wstring processName;
	std::vector<uintptr_t> offsets;
	int value;

	void convertPattern(LPCWSTR sign)
	{
		std::wstring signature(sign);
		std::wstringstream wss(signature);
		std::vector<BYTE> bytes;
		std::vector<std::wstring> tokens{ std::istream_iterator<std::wstring, wchar_t>(wss),{} };

		for (std::wstring str : tokens)
		{
			if (str.size() == 1 || str._Equal(L"xx") || str._Equal(L"XX"))
			{
				mask.append(L"?");
				bytes.push_back(0);
			}
			else 
			{
				mask.append(L"x");
				BYTE singleByte = static_cast<BYTE>(wcstoul(str.c_str(), NULL, 16)); // Явное преобразование unsigned long в BYTE
				bytes.push_back(singleByte);
			}
		}

		if (pattern)
		{
			delete[] pattern;
		}
		pattern = new BYTE[bytes.size()];
		memcpy_s(pattern, bytes.size(), bytes.data(), bytes.size());
	}

public:
	Patch(CheatOption* parentInstance, LPCWSTR signature, int patchOffset, SIZE_T pSize)
	{
		parent = parentInstance;
		convertPattern(signature);
		patchSize = pSize;
	}

	Patch(CheatOption* parentInstance, LPCWSTR signature, SIZE_T pSize) : 
		Patch(parentInstance, signature, 0 ,pSize)
	{

	}

	Patch(CheatOption* parentInstance, LPCWSTR processName, std::vector<uintptr_t> offsets, int value)
	{
		parent = parentInstance;
		this->processName = processName;
		this->offsets = offsets;
		this->value = value;
	}

	virtual bool Hack(HANDLE hProcess) = 0;
	virtual bool Restore(HANDLE hProcess) = 0;

	CheatOption* GetParent() const
	{
		return parent;
	}

	LPVOID GetOriginalAddress() const
	{
		return originalAddress;
	}

	void SetParent(CheatOption* newParent)
	{
		parent = newParent;
	}
};