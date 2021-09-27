#include "Memory_Functions.h"
#include "CheatOption.h"
#include "NopPatch.h"
#include "CavePatch.h"
#include <playsoundapi.h>
#pragma comment(lib, "Winmm.lib")

bool CheatOption::Enable(int pid)
{
	HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, false, pid);

	if (hProc)
	{
		for (Patch* p : patches) 
		{
			p->Hack(hProc);
		}
		CloseHandle(hProc);
		PlaySoundW(L"ResourceExtern\\activ.wav", NULL, SND_FILENAME | SND_ASYNC);
		return true;
	}
	return false;
}

bool CheatOption::Disable(int pid)
{
	HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, false, pid);

	if (hProc)
	{
		for (Patch* p : patches)
		{
			p->Restore(hProc);
		}
		CloseHandle(hProc);
		PlaySoundW(L"ResourceExtern\\deactiv.wav", NULL, SND_FILENAME | SND_ASYNC);
		return true;
	}
	return false;
}

bool CheatOption::KeyPressed()
{
	for (int key : m_keys) 
	{
		if (GetAsyncKeyState(key) != -32768)
		{
			return false;
		}
		Sleep(10);
	}
	return true;
}

CheatOption* CheatOption::AddNopPatch(LPCWSTR signature, SIZE_T pSize)
{
	patches.push_back(new NopPatch(this, signature, pSize));
	return this;
}

CheatOption* CheatOption::AddCavePatch(LPCWSTR signature, PBYTE pBytes, SIZE_T patchSize)
{
	patches.push_back(new CavePatch(this, signature, pBytes, patchSize));
	return this;
}

void CheatOption::Process(int processId)
{
	if (KeyPressed())
	{
		if (m_enabled)
		{
			Disable(processId);
			m_enabled = false;
		}
		else 
		{
			Enable(processId);
			m_enabled = true;
		}
	}
}
