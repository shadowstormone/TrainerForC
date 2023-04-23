#pragma once
#include <Windows.h>
#include <vector>
#include <string>
#include <playsoundapi.h>

extern class Patch;

class CheatOption
{
	//Свойства чита
	LPCWSTR m_moduleName = NULL;
	LPCWSTR m_description = NULL;
	std::vector<int> m_keys;

	// Функции чита
	bool m_enabled = false;
	std::vector<Patch*> patches;

	bool Enable(int pid);
	bool Disable(int pid);
	bool KeyPressed();

public:
	CheatOption(LPCWSTR moduleName, LPCWSTR description, const std::vector<int>& keys) 
	{
		m_moduleName = moduleName;
		m_description = description;
		m_keys = keys;
		patches.clear();
	}

	CheatOption* AddNopPatch(LPCWSTR signature, SIZE_T pSize);
	CheatOption* AddCavePatch(LPCWSTR signature, PBYTE pBytes, SIZE_T patchSize);

	void Process(int processId);

	bool IsEnabled() 
	{
		return m_enabled;
	}

	LPCWSTR GetDescription() 
	{
		return m_description;
	}

	LPCWSTR GetModuleName() 
	{
		return m_moduleName;
	}
};