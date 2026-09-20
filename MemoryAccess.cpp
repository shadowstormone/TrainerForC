#include "MemoryAccess.h"

MemoryAccess::MemoryAccess(DWORD pid) : m_pid(pid)
{
    if (pid != 0)
        m_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
}

MemoryAccess::~MemoryAccess()
{
    if (IsValid())
    {
        CloseHandle(m_handle);
        m_handle = nullptr;
    }
}
