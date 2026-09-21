#include "core/MemoryAccess.h"

#include "core/Memory_Functions.h"

MemoryAccess::MemoryAccess(DWORD pid) : m_pid(pid)
{
    if (pid != 0)
        m_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

    if (IsValid())
        m_targetIsX64 = isTargetX64Process(m_handle);
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
    if (!IsValid()) return 0;

    // Читаем РОВНО столько байт, сколько занимает указатель в цели.
    // Раньше читалось sizeof(uintptr_t) трейнера (8 байт на x64), из-за чего
    // в 32-битных играх в старшую половину попадал мусор и вся цепочка
    // оффсетов вела не туда.
    std::uint64_t raw = 0;
    SIZE_T read = 0;

    if (!ReadProcessMemory(m_handle, reinterpret_cast<LPCVOID>(address), &raw, PointerSize(), &read)
        || read != PointerSize())
    {
        return 0;
    }

    return static_cast<uintptr_t>(raw);
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

LPVOID MemoryAccess::AllocNear(std::uintptr_t target, SIZE_T amount) const
{
    if (!IsValid()) return nullptr;

    SYSTEM_INFO si{};
    GetSystemInfo(&si);
    const std::uintptr_t granularity = si.dwAllocationGranularity;

    constexpr std::uintptr_t RANGE = 0x7FFF0000; // чуть меньше 2 ГБ, с запасом
    const std::uintptr_t low  = (target > RANGE) ? (target - RANGE) : 0;
    const std::uintptr_t high = target + RANGE;

    const auto tryAt = [&](std::uintptr_t address) -> LPVOID
    {
        const std::uintptr_t aligned = (address + granularity - 1) & ~(granularity - 1);
        if (aligned < low || aligned >= high) return nullptr;

        return VirtualAllocEx(m_handle, reinterpret_cast<LPVOID>(aligned), amount,
                              MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    };

    // Вверх от цели: перепрыгиваем занятые участки по их реальному размеру,
    // а не шагаем по 64 КБ через всё адресное пространство.
    for (std::uintptr_t probe = target; probe < high; )
    {
        MEMORY_BASIC_INFORMATION mbi{};
        if (VirtualQueryEx(m_handle, reinterpret_cast<LPCVOID>(probe), &mbi, sizeof(mbi)) == 0) break;

        if (mbi.State == MEM_FREE && mbi.RegionSize >= amount)
        {
            if (LPVOID got = tryAt(reinterpret_cast<std::uintptr_t>(mbi.BaseAddress))) return got;
        }

        probe = reinterpret_cast<std::uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
    }

    // Вниз от цели.
    for (std::uintptr_t probe = target; probe > low; )
    {
        MEMORY_BASIC_INFORMATION mbi{};
        if (VirtualQueryEx(m_handle, reinterpret_cast<LPCVOID>(probe), &mbi, sizeof(mbi)) == 0) break;

        const std::uintptr_t regionBase = reinterpret_cast<std::uintptr_t>(mbi.BaseAddress);

        if (mbi.State == MEM_FREE && mbi.RegionSize >= amount)
        {
            // Ближе к цели — значит выше по региону.
            const std::uintptr_t last = regionBase + mbi.RegionSize - amount;
            if (LPVOID got = tryAt(last > regionBase ? (last & ~(granularity - 1)) : regionBase)) return got;
            if (LPVOID got = tryAt(regionBase)) return got;
        }

        if (regionBase < granularity) break;
        probe = regionBase - 1;
    }

    // Рядом не нашлось — берём где угодно, прыжок тогда будет длинным.
    return VirtualAllocEx(m_handle, nullptr, amount, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
}
