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

    // Разрядность цели определяется один раз при открытии процесса.
    // Она нужна не только для дизассемблера: в 32-битной игре указатели
    // занимают 4 байта, а не 8, и цепочки оффсетов надо читать именно так.
    bool m_targetIsX64 = false;

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
    bool      IsTargetX64() const { return m_targetIsX64; }

    // Размер указателя В ЦЕЛЕВОМ процессе: 8 для x64, 4 для x86.
    SIZE_T    PointerSize() const { return m_targetIsX64 ? 8u : 4u; }

    DWORD_PTR ProcessBase() const;
    DWORD_PTR ModuleBase(LPCWSTR moduleName) const;

    // --- Чтение и запись ---
    LPVOID    Read(LPVOID address, SIZE_T amount) const;
    uintptr_t ReadPointer(uintptr_t address) const;

    // true — записано полностью. Именно bool, а не код возврата:
    // нижележащие функции отдают 0 при УСПЕХЕ, и на этом уже один раз
    // сломались кейвы — успех принимался за провал.
    bool      Write(LPVOID address, LPVOID source, SIZE_T amount) const;
    bool      WriteInt(uintptr_t address, int value) const;

    // --- Память и поиск ---
    LPVOID Alloc(LPVOID startAddress, SIZE_T amount) const;

    // true — память освобождена.
    bool   Free(LPVOID address, SIZE_T amount) const;

    // Выделяет память ПОБЛИЗОСТИ от target — в пределах ±2 ГБ, чтобы до неё
    // доставал короткий 5-байтный прыжок. VirtualAllocEx с nullptr кладёт
    // блок куда угодно, и на x64 он почти всегда оказывается дальше 2 ГБ:
    // тогда нужен 14-байтный прыжок, а значит больше украденных инструкций
    // и выше шанс, что среди них попадётся непереносимая.
    // Если рядом места нет — выделяет где получится (не nullptr).
    LPVOID AllocNear(std::uintptr_t target, SIZE_T amount) const;
    LPVOID ScanSignature(ULONG_PTR startAddress, SIZE_T scanSize, PBYTE pattern, std::wstring& mask) const;

    // Проходит цепочку "base + offsets[0] -> разыменовать -> ... " и
    // возвращает конечный адрес. 0, если цепочка оборвалась.
    uintptr_t ResolveChain(uintptr_t base, const std::vector<uintptr_t>& offsets) const;
};
