#pragma once
#include <Windows.h>
#include <vector>
#include <string>
#include <memory>
#include "patches/Patch.h"   // нужен полный тип для unique_ptr<Patch>
#include "core/Cheat.h"   // нужен для Cheat* в параметрах методов

class Cheat;  // предварительное объявление (на случай кольцевого include)

class CheatOption
{
	//Свойства чита
	LPCWSTR m_moduleName = nullptr;
	LPCWSTR m_description = nullptr;
	std::vector<int> m_keys;

	// Функции чита
	bool m_enabled = false;
	std::vector<std::unique_ptr<Patch>> patches;

	bool KeyPressed();
public:
	bool Enable(int pid);
	bool Disable(int pid);
	CheatOption(LPCWSTR moduleName, LPCWSTR description, const std::vector<int>& keys) 
	{
		m_moduleName = moduleName;
		m_description = description;
		m_keys = keys;
		patches.clear();
	}

	CheatOption(LPCWSTR moduleName, LPCWSTR description)
	{
		m_moduleName = moduleName;
		m_description = description;
		patches.clear();
	}

	CheatOption* AddNopPatch(LPCWSTR signature, SIZE_T pSize);
	CheatOption* AddCavePatch(LPCWSTR signature, PBYTE pBytes, SIZE_T patchSize);
	CheatOption* AddWriteValuePatch(Cheat* cheatProcess, std::vector<uintptr_t> offsets, int value);
	CheatOption* AddWriteValuePatch(Cheat* cheatProcess, std::vector<uintptr_t> offsets, float value);
	CheatOption* AddWriteValuePatch(Cheat* cheatProcess, std::vector<uintptr_t> offsets, double value);
	void Process(int processId);

	bool IsEnabled(bool state)
	{
		return m_enabled = state;
	}

	bool IsEnabled() const
	{
		return m_enabled;
	}

	LPCWSTR GetDescription() const
	{
		return m_description;
	}

	LPCWSTR GetModuleName() const
	{
		return m_moduleName;
	}

	void SetModuleName(LPCWSTR moduleName)
	{
		m_moduleName = moduleName;
	}

};