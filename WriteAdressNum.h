#pragma once
#include <Windows.h>
#include <vector>
#include "Memory_Functions.h"
#include "Patch.h"

class WriteAddressPatch : public Patch
{
public:
	WriteAddressPatch(CheatOption* parentInstance, LPCWSTR processName, std::vector<uintptr_t> offsets, int value) : 
		Patch(parentInstance, processName, offsets, value), hProcess(nullptr), finalAddress(0) {}

	virtual bool Hack(HANDLE hProcess) override
	{
		if (!isApplied)
		{
			isApplied = true;
		}
		return WriteValueMemory(processName.c_str(), offsets, value);
	}

	virtual bool Restore(HANDLE hProcess) override
	{
		if (isApplied)
		{
			// Ваш код для восстановления патча
			isApplied = false;
		}
		return false;
	}

	bool IsApplied() const
	{
		return isApplied;
	}

	bool WriteValueMemory(LPCWSTR processName, std::vector<uintptr_t> offset, int value);
	~WriteAddressPatch();

private:
	LPCWSTR _processName = NULL;
	int processId = 0;
	HANDLE hProcess = nullptr;
	DWORD_PTR baseAddressProcess = 0;
	uintptr_t finalAddress = 0;
	bool isApplied = false;
};