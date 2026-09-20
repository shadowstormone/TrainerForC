#pragma once
#include <Windows.h>
#include <cstdint>
#include <string>
#include <vector>

// Доступ к памяти целевого процесса.
//
// RAII над HANDLE: открывает дескриптор при создании, закрывает при
// уничтожении. Операции чтения/записи/сканирования — методы этого объекта,
// а не свободные функции с HANDLE в первом аргументе: handle теперь нельзя
// забыть закрыть или передать чужой.
//
// Копирование и move запрещены — объект владеет ровно одним handle.
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

    // --- Сведения о процессе ---
    bool      IsTargetX64() const;
    DWORD_PTR ProcessBase() const;
    DWORD_PTR ModuleBase(LPCWSTR moduleName) const;

    // --- Чтение и запись ---
    LPVOID    Read(LPVOID address, SIZE_T amount) const;
    uintptr_t ReadPointer(uintptr_t address) const;
    int       Write(LPVOID address, LPVOID source, SIZE_T amount) const;
    int       WriteInt(uintptr_t address, int value) const;

    // --- Память и поиск ---
    LPVOID Alloc(LPVOID startAddress, SIZE_T amount) const;
    int    Free(LPVOID address, SIZE_T amount) const;
    LPVOID ScanSignature(ULONG_PTR startAddress, SIZE_T scanSize, PBYTE pattern, std::wstring& mask) const;

    // Проходит цепочку "base + offsets[0] -> разыменовать -> ... " и
    // возвращает конечный адрес. 0, если цепочка оборвалась.
    uintptr_t ResolveChain(uintptr_t base, const std::vector<uintptr_t>& offsets) const;
};
