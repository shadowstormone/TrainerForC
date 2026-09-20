#pragma once
#include <Windows.h>
#include <vector>
#include <string>
#include <iterator>
#include <cstdint>
#include "core/Memory_Functions.h"

class CheatOption;	// Предварительное объявление класса CheatOption
class Patch		// Предварительное объявление класса Patch
{
protected:
	std::vector<uint8_t> pattern;   // владеет байтами паттерна (было: PBYTE + new[])
	std::wstring mask;
	LPVOID originalAddress = nullptr;
	PBYTE  originalBytes   = nullptr; // аллоцируется ReadMem в наследниках; освобождается ~Patch
	SIZE_T patchSize = 0;
	CheatOption* parent = nullptr;
	LPBYTE patchAddress = nullptr;
	LPVOID patternAddress = nullptr;

	// Дополнительные атрибуты для конструктора с processName, offset и value
	std::wstring processName;
	std::vector<uintptr_t> offsets;
	int value = 0;
	float fvalue = 0;
	double dvalue = 0;

	void convertPattern(LPCWSTR sign)
	{
		std::wstring signature(sign);
		std::wstringstream wss(signature);
		std::vector<std::wstring> tokens{ std::istream_iterator<std::wstring, wchar_t>(wss),{} };

		pattern.clear();
		mask.clear();

		for (const std::wstring& str : tokens)
		{
			if (str.size() == 1 || str._Equal(L"xx") || str._Equal(L"XX"))
			{
				mask += L'?';
				pattern.push_back(0);
			}
			else
			{
				mask += L'x';
				pattern.push_back(static_cast<uint8_t>(wcstoul(str.c_str(), nullptr, 16)));
			}
		}
	}

public:
	Patch(){}

	virtual ~Patch()
	{
		delete[] originalBytes;
		originalBytes = nullptr;
	}

	Patch(CheatOption* parentInstance, LPCWSTR signature, int patchOffset, SIZE_T pSize)
	{
		parent = parentInstance;
		convertPattern(signature);
		patchSize = pSize;
	}

	Patch(CheatOption* parentInstance, LPCWSTR signature, SIZE_T pSize) : 
		Patch(parentInstance, signature, 0 ,pSize) {}

	Patch(CheatOption* parentInstance, LPCWSTR processName, std::vector<uintptr_t> offsets, int value)
	{
		parent = parentInstance;
		this->processName = processName;
		this->offsets = offsets;
		this->value = value;
	}

	Patch(CheatOption* parentInstance, LPCWSTR processName, std::vector<uintptr_t> offsets, float value)
	{
		parent = parentInstance;
		this->processName = processName;
		this->offsets = offsets;
		this->fvalue = value;
	}

	Patch(CheatOption* parentInstance, LPCWSTR processName, std::vector<uintptr_t> offsets, double value)
	{
		parent = parentInstance;
		this->processName = processName;
		this->offsets = offsets;
		this->dvalue = value;
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