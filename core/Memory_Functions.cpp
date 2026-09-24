#include "core/Memory_Functions.h"

#include <TlHelp32.h>

#include <cwchar>

DWORD GetProcessIdByProcessName(LPCWSTR processName)
{
	if (!processName || !*processName) return 0;

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot == INVALID_HANDLE_VALUE) return 0;

	DWORD pid = 0;
	PROCESSENTRY32W pe{};
	pe.dwSize = sizeof(pe);

	if (Process32FirstW(snapshot, &pe))
	{
		do
		{
			// Без учёта регистра: Windows имена файлов не различает, и
			// "tutorial-x86_64.exe" — тот же процесс.
			if (_wcsicmp(processName, pe.szExeFile) == 0)
			{
				pid = pe.th32ProcessID;
				break;
			}
		} while (Process32NextW(snapshot, &pe));
	}

	CloseHandle(snapshot);
	return pid;
}

bool isTargetX64Process(HANDLE hProcess)
{
	if (!hProcess) return false;

	USHORT processMachine = IMAGE_FILE_MACHINE_UNKNOWN;
	USHORT nativeMachine = IMAGE_FILE_MACHINE_UNKNOWN;

	if (!IsWow64Process2(hProcess, &processMachine, &nativeMachine))
	{
#ifdef _WIN64
		return true;
#else
		return false;
#endif
	}

	// UNKNOWN — процесс не под WOW64, то есть родной разрядности системы.
	return processMachine == IMAGE_FILE_MACHINE_UNKNOWN;
}
