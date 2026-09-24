#pragma once
#include <Windows.h>

// Низкоуровневые функции поиска процесса. Всё, что касается памяти цели,
// живёт в MemoryAccess: здесь остались только операции, которым процесс
// ещё не нужен открытым.

// PID процесса по имени exe (без учёта регистра). 0 — не запущен.
DWORD GetProcessIdByProcessName(LPCWSTR processName);

// true — цель 64-битная.
bool isTargetX64Process(HANDLE hProcess);
