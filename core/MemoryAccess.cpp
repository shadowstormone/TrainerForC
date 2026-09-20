#include "core/MemoryAccess.h"

#include "core/Memory_Functions.h"

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

// Реализация опирается на функции из Memory_Functions — теперь это
// внутренняя деталь: наружу торчат только методы MemoryAccess.

bool MemoryAccess::IsTargetX64() const
{
    return isTargetX64Process(m_handle);
}

DWORD_PTR MemoryAccess::ProcessBase() const
{
    return GetProcessBaseAddress(m_handle);
}

DWORD_PTR MemoryAccess::ModuleBase(LPCWSTR moduleName) const
{
    return GetModuleBaseAddress(m_handle, moduleName);
}

LPVOID MemoryAccess::Read(LPVOID address, SIZE_T amount) const
{
    return ReadMem(m_handle, address, amount);
}

uintptr_t MemoryAccess::ReadPointer(uintptr_t address) const
{
    return ReadMem(m_handle, address);
}

int MemoryAccess::Write(LPVOID address, LPVOID source, SIZE_T amount) const
{
    return WriteMem(m_handle, address, source, amount);
}

int MemoryAccess::WriteInt(uintptr_t address, int value) const
{
    return WriteMem(m_handle, address, value);
}

LPVOID MemoryAccess::Alloc(LPVOID startAddress, SIZE_T amount) const
{
    return AllocMem(m_handle, startAddress, amount);
}

int MemoryAccess::Free(LPVOID address, SIZE_T amount) const
{
    return FreeMem(m_handle, address, amount);
}

LPVOID MemoryAccess::ScanSignature(ULONG_PTR startAddress, SIZE_T scanSize, PBYTE pattern, std::wstring& mask) const
{
    return ::ScanSignature(m_handle, startAddress, scanSize, pattern, mask);
}

uintptr_t MemoryAccess::ResolveChain(uintptr_t base, const std::vector<uintptr_t>& offsets) const
{
    if (base == 0 || offsets.empty()) return 0;

    uintptr_t current = base;
    for (size_t i = 0; i + 1 < offsets.size(); ++i)
    {
        current += offsets[i];
        current = ReadPointer(current);
        if (current == 0) return 0; // разорванная цепочка указателей
    }

    return current + offsets.back();
}
