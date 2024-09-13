#pragma once
#include <string>
#include <Windows.h>
#include <TlHelp32.h>
#include <sstream>
#include <vector>
#include <iterator>
#include <Psapi.h>

int GetProcessIdByWindowName(LPCWSTR className, LPCWSTR windowName);
int GetProcessIdByProcessName(LPCWSTR processName);
int WriteMem(HANDLE hProcess, LPVOID address, LPVOID source, SIZE_T writeAmount);
int WriteMem(HANDLE hProcess, uintptr_t address, int value);
LPVOID ReadMem(HANDLE hProcess, LPVOID address, SIZE_T readAmount);
uintptr_t ReadMem(HANDLE hProcess, uintptr_t address);
LPVOID AllocMem(HANDLE hProcess, LPVOID startAddress, SIZE_T allocationAmount);
int FreeMem(HANDLE hProcess, LPVOID address, SIZE_T amount);
LPVOID ScanSignature(HANDLE hProcess, ULONG_PTR startAddress, SIZE_T scanSize, PBYTE pattern, std::wstring& mask);
bool CheckSignature(PBYTE source, PBYTE pattern, std::wstring& mask);
void ShowErrorMessage(HWND hWnd, LPCWSTR errorMassage);
bool isTargetX64Process(HANDLE hProcess);
DWORD_PTR GetProcessBaseAddress(HANDLE hProcess);
DWORD_PTR GetModuleBaseAddress(HANDLE hProcess, LPCWSTR lpszModuleName);