#pragma once
#include <Windows.h>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

// Доступ к памяти целевого процесса.
//
// RAII над HANDLE: открывает дескриптор при создании, закрывает при
// уничтожении. Операции чтения/записи/сканирования — методы этого объекта,
// а не свободные функции с HANDLE в первом аргументе: handle теперь нельзя
// забыть закрыть или передать чужой.
//
// Ошибки НЕ показываются окнами: раньше неудачное чтение открывало
// MessageBox — в том числе из фонового потока горячих клавиш, посреди игры.
// Теперь методы возвращают false/0/пусто, а причину пишут в лог.
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
    // Модуль в памяти процесса: где начинается и сколько занимает.
    struct ModuleInfo
    {
        std::uintptr_t base = 0;
        std::size_t    size = 0;

        explicit operator bool() const { return base != 0; }
    };

    explicit MemoryAccess(DWORD pid);
    ~MemoryAccess();

    MemoryAccess(const MemoryAccess&)            = delete;
    MemoryAccess& operator=(const MemoryAccess&) = delete;
    MemoryAccess(MemoryAccess&&)                 = delete;
    MemoryAccess& operator=(MemoryAccess&&)      = delete;

    bool   IsValid() const { return m_handle != nullptr && m_handle != INVALID_HANDLE_VALUE; }
    HANDLE Handle()  const { return m_handle; }
    DWORD  Pid()     const { return m_pid; }

    // Жив ли ещё процесс. Дескриптор может пережить сам процесс — тогда
    // писать в него бессмысленно.
    bool IsAlive() const;

    // --- Сведения о процессе ---
    bool IsTargetX64() const { return m_targetIsX64; }

    // Размер указателя В ЦЕЛЕВОМ процессе: 8 для x64, 4 для x86.
    SIZE_T PointerSize() const { return m_targetIsX64 ? 8u : 4u; }

    // Главный модуль (сам exe) и модуль по имени. Имя сравнивается без
    // учёта регистра: Windows тоже его не различает, а "gameassembly.dll"
    // и "GameAssembly.dll" раньше считались разными модулями.
    ModuleInfo MainModule() const;
    ModuleInfo Module(std::wstring_view moduleName) const;

    // Модуль по имени; пустое имя — главный модуль.
    ModuleInfo ModuleOrMain(std::wstring_view moduleName) const
    {
        return moduleName.empty() ? MainModule() : Module(moduleName);
    }

    DWORD_PTR ProcessBase() const { return MainModule().base; }
    DWORD_PTR ModuleBase(LPCWSTR moduleName) const { return Module(moduleName ? moduleName : L"").base; }

    // --- Чтение и запись ---

    // true — прочитано ровно size байт.
    bool Read(std::uintptr_t address, void* out, std::size_t size) const;

    // Пустой вектор — чтение не удалось.
    std::vector<std::uint8_t> ReadBytes(std::uintptr_t address, std::size_t size) const;

    std::uintptr_t ReadPointer(std::uintptr_t address) const;

    // Запись в любую страницу, в том числе в код.
    //
    // Защита страницы на время записи меняется на PAGE_EXECUTE_READWRITE,
    // а не на PAGE_READWRITE, как раньше: иначе, если игра в этот момент
    // исполняла патчимую функцию, она падала на DEP. После записи
    // сбрасывается кэш инструкций.
    bool Write(std::uintptr_t address, const void* source, std::size_t size) const;
    bool Write(LPVOID address, const void* source, std::size_t size) const
    {
        return Write(reinterpret_cast<std::uintptr_t>(address), source, size);
    }

    template <typename T>
    bool ReadValue(std::uintptr_t address, T& out) const { return Read(address, &out, sizeof(T)); }

    template <typename T>
    bool WriteValue(std::uintptr_t address, const T& value) const { return Write(address, &value, sizeof(T)); }

    // --- Память и поиск ---

    // true — память освобождена.
    bool Free(LPVOID address) const;

    // Выделяет память ПОБЛИЗОСТИ от target — в пределах ±2 ГБ, чтобы до неё
    // доставал короткий 5-байтный прыжок. VirtualAllocEx с nullptr кладёт
    // блок куда угодно, и на x64 он почти всегда оказывается дальше 2 ГБ:
    // тогда нужен 14-байтный прыжок, а значит больше украденных инструкций
    // и выше шанс, что среди них попадётся непереносимая.
    // Если рядом места нет — выделяет где получится (не nullptr).
    LPVOID AllocNear(std::uintptr_t target, SIZE_T amount) const;

    // Ищет сигнатуру в диапазоне [start, start + size). Маска: 'x' — байт
    // должен совпасть, '?' — любой. Возвращает адрес первого совпадения или 0.
    //
    // Нечитаемые и зарезервированные участки пропускаются, а не обрывают
    // поиск; память читается кусками с перекрытием, так что совпадение на
    // стыке регионов тоже находится.
    //
    // executableOnly — смотреть только исполняемую память: для поиска кода
    // по всему процессу (JIT-код Unity/Mono живёт вне модулей).
    std::uintptr_t ScanSignature(std::uintptr_t start, std::uintptr_t size,
                                 const std::vector<std::uint8_t>& pattern,
                                 std::wstring_view mask,
                                 bool executableOnly = false) const;

    // Весь диапазон пользовательских адресов процесса.
    static ModuleInfo WholeAddressSpace();

    // Сравнение с маской в локальном буфере — ядро сканера. Отдельно,
    // чтобы его можно было проверить тестом без чужого процесса.
    // Возвращает смещение первого совпадения или SIZE_MAX.
    static std::size_t FindPattern(const std::uint8_t* data, std::size_t dataSize,
                                   const std::vector<std::uint8_t>& pattern,
                                   std::wstring_view mask);

    // Проходит цепочку "base + offsets[0] -> разыменовать -> ... " и
    // возвращает конечный адрес. 0, если цепочка оборвалась.
    std::uintptr_t ResolveChain(std::uintptr_t base, const std::vector<std::uintptr_t>& offsets) const;
};
