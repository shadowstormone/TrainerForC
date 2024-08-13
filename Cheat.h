#pragma once
#include <Windows.h>
#include <vector>
#include <string>
#include <map>
#include <thread>
#include "CheatOption.h"
#include "Memory_Functions.h"

class Cheat
{
	std::vector<CheatOption*> options;
	std::map<LPCWSTR, bool> m_optionsState;
	LPCWSTR _processName = NULL;
	int processId = 0;

	static DWORD WINAPI ProcessorStarter(void* param) 
	{
		Cheat* that = reinterpret_cast<Cheat*>(param);
		that->ProcessorOptions();
		return 0;
	}

	bool isRunning = false;

	void ProcessorOptions();
public:
	void OpenConsole();

	Cheat(LPCWSTR processName) : _processName(processName) 
	{

	}

	void Start()
	{
		if (!isRunning) 
		{
			isRunning = true;
			DWORD threadId;
			CreateThread(NULL, 0, ProcessorStarter, this, NULL, &threadId);
		}
	}

	void Stop() 
	{
		isRunning = false;
	}

	std::map<LPCWSTR, bool>& GetCheatOptionState()
	{
		return m_optionsState;
	}

	LPCWSTR GetProcessName()
	{
		return _processName;
	}

	bool isProcessRunning()
	{
		return processId;
	}

	int AddCheatOption(CheatOption* option);

	void RemoveCheatOption(int index);
};