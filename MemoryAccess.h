#pragma once
#include <Windows.h>

// RAII-обёртка над HANDLE целевого процесса.
// Открывает дескриптор при создании, закрывает при уничтожении.
// Копирование запрещено; move тоже — объект владеет ровно одним handle.
class MemoryAccess
{
    HANDLE m_handle = nullptr;
    DWORD  m_pid    = 0;

public:
    explicit MemoryAccess(DWORD pid);
    ~MemoryAccess();

    MemoryAccess(const MemoryAccess&)            = delete;
    MemoryAccess& operator=(const MemoryAccess&) = delete;
    MemoryAccess(MemoryAccess&&)                 = delete;
    MemoryAccess& operator=(MemoryAccess&&)      = delete;

    bool   IsValid() const { return m_handle != nullptr && m_handle != INVALID_HANDLE_VALUE; }
    HANDLE Handle()  const { return m_handle; }
    DWORD  Pid()     const { return m_pid; }
};
