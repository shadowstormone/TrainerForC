#include "core/MemoryAccess.h"

#include <TlHelp32.h>

#include <algorithm>
#include <cstring>
#include <cwctype>
#include <format>

#include "core/Memory_Functions.h"
#include "platform/Logger.h"

namespace
{
    // Читать гигантский регион одним new[] нельзя: у игр бывают кучи на
    // гигабайты. Читаем окнами такого размера.
    constexpr std::size_t SCAN_CHUNK = 1u << 20; // 1 МБ

    bool IsReadable(const MEMORY_BASIC_INFORMATION& mbi)
    {
        if (mbi.State != MEM_COMMIT) return false;
        if (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) return false;
        return mbi.Protect != 0;
    }

    bool IsExecutable(const MEMORY_BASIC_INFORMATION& mbi)
    {
        return (mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE
                             | PAGE_EXECUTE_WRITECOPY)) != 0;
    }

    bool EqualsIgnoreCase(std::wstring_view a, std::wstring_view b)
    {
        if (a.size() != b.size()) return false;
        for (std::size_t i = 0; i < a.size(); ++i)
        {
            if (std::towlower(a[i]) != std::towlower(b[i])) return false;
        }
        return true;
    }

    // Обходит модули процесса; visit возвращает true, чтобы остановиться.
    template <typename Visit>
    void ForEachModule(DWORD pid, Visit&& visit)
    {
        // SNAPMODULE32 — чтобы видеть модули 32-битной игры из 64-битного
        // трейнера. Раньше поиск модуля по имени этого флага не ставил и
        // в x86-играх ничего не находил.
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
        if (snapshot == INVALID_HANDLE_VALUE)
        {
            Log::Error(std::format("Не удалось сделать снимок модулей, код {}", GetLastError()));
            return;
        }

        MODULEENTRY32W entry{};
        entry.dwSize = sizeof(entry);

        if (Module32FirstW(snapshot, &entry))
        {
            do
            {
                if (visit(entry)) break;
            } while (Module32NextW(snapshot, &entry));
        }

        CloseHandle(snapshot);
    }
}

MemoryAccess::MemoryAccess(DWORD pid) : m_pid(pid)
{
    if (pid != 0)
    {
        m_handle = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE
                             | PROCESS_QUERY_INFORMATION | SYNCHRONIZE,
                               FALSE, pid);
        if (!IsValid())
        {
            m_handle = nullptr;
            Log::Error(std::format("Не удалось открыть процесс {} (код {}). "
                                   "Если игра запущена от администратора — запустите так же и трейнер.",
                                   pid, GetLastError()));
        }
    }

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

bool MemoryAccess::IsAlive() const
{
    return IsValid() && WaitForSingleObject(m_handle, 0) == WAIT_TIMEOUT;
}

MemoryAccess::ModuleInfo MemoryAccess::MainModule() const
{
    ModuleInfo info;
    if (!IsValid()) return info;

    // Первый модуль в снимке — всегда сам исполняемый файл процесса.
    ForEachModule(m_pid, [&](const MODULEENTRY32W& entry)
    {
        info.base = reinterpret_cast<std::uintptr_t>(entry.modBaseAddr);
        info.size = entry.modBaseSize;
        return true;
    });

    return info;
}

MemoryAccess::ModuleInfo MemoryAccess::Module(std::wstring_view moduleName) const
{
    ModuleInfo info;
    if (!IsValid() || moduleName.empty()) return info;

    ForEachModule(m_pid, [&](const MODULEENTRY32W& entry)
    {
        if (!EqualsIgnoreCase(entry.szModule, moduleName)) return false;

        info.base = reinterpret_cast<std::uintptr_t>(entry.modBaseAddr);
        info.size = entry.modBaseSize;
        return true;
    });

    return info;
}

bool MemoryAccess::Read(std::uintptr_t address, void* out, std::size_t size) const
{
    if (!IsValid() || !out || size == 0) return false;

    SIZE_T read = 0;
    return ReadProcessMemory(m_handle, reinterpret_cast<LPCVOID>(address), out, size, &read)
        && read == size;
}

std::vector<std::uint8_t> MemoryAccess::ReadBytes(std::uintptr_t address, std::size_t size) const
{
    std::vector<std::uint8_t> bytes(size);
    if (!Read(address, bytes.data(), size)) bytes.clear();
    return bytes;
}

std::uintptr_t MemoryAccess::ReadPointer(std::uintptr_t address) const
{
    // Читаем РОВНО столько байт, сколько занимает указатель в цели.
    // Раньше читалось sizeof(uintptr_t) трейнера (8 байт на x64), из-за чего
    // в 32-битных играх в старшую половину попадал мусор и вся цепочка
    // оффсетов вела не туда.
    std::uint64_t raw = 0;
    if (!Read(address, &raw, PointerSize())) return 0;
    return static_cast<std::uintptr_t>(raw);
}

bool MemoryAccess::Write(std::uintptr_t address, const void* source, std::size_t size) const
{
    if (!IsValid() || !source || size == 0 || address == 0) return false;

    LPVOID target = reinterpret_cast<LPVOID>(address);

    DWORD oldProtect = 0;
    const bool unprotected = VirtualProtectEx(m_handle, target, size, PAGE_EXECUTE_READWRITE, &oldProtect) != 0;

    SIZE_T written = 0;
    const bool ok = WriteProcessMemory(m_handle, target, source, size, &written) && written == size;
    const DWORD writeError = GetLastError();

    if (unprotected)
    {
        DWORD ignored = 0;
        VirtualProtectEx(m_handle, target, size, oldProtect, &ignored);
    }

    if (!ok)
    {
        Log::Error(std::format("Запись {} байт по адресу 0x{:X} не удалась (код {})",
                               size, address, writeError));
        return false;
    }

    // Процессор мог уже закэшировать старые инструкции.
    FlushInstructionCache(m_handle, target, size);
    return true;
}

bool MemoryAccess::Free(LPVOID address) const
{
    if (!IsValid() || !address) return false;

    // MEM_RELEASE требует size = 0.
    return VirtualFreeEx(m_handle, address, 0, MEM_RELEASE) != 0;
}

std::size_t MemoryAccess::FindPattern(const std::uint8_t* data, std::size_t dataSize,
                                      const std::vector<std::uint8_t>& pattern,
                                      std::wstring_view mask)
{
    const std::size_t length = (std::min)(pattern.size(), mask.size());
    if (!data || length == 0 || dataSize < length) return SIZE_MAX;

    // Первый точный байт сигнатуры ищем memchr — это на порядок быстрее,
    // чем сверять маску в каждой позиции.
    std::size_t anchor = 0;
    while (anchor < length && mask[anchor] != L'x') ++anchor;

    const std::size_t last = dataSize - length;

    if (anchor == length)
    {
        return 0; // одни джокеры — совпадает где угодно
    }

    const std::uint8_t anchorByte = pattern[anchor];
    std::size_t pos = 0;

    while (pos <= last)
    {
        const void* hit = std::memchr(data + pos + anchor, anchorByte, last - pos + 1);
        if (!hit) return SIZE_MAX;

        pos = static_cast<std::size_t>(static_cast<const std::uint8_t*>(hit) - data) - anchor;

        bool match = true;
        for (std::size_t i = 0; i < length; ++i)
        {
            if (mask[i] == L'x' && data[pos + i] != pattern[i])
            {
                match = false;
                break;
            }
        }

        if (match) return pos;
        ++pos;
    }

    return SIZE_MAX;
}

std::uintptr_t MemoryAccess::ScanSignature(std::uintptr_t start, std::uintptr_t size,
                                           const std::vector<std::uint8_t>& pattern,
                                           std::wstring_view mask,
                                           bool executableOnly) const
{
    const std::size_t length = (std::min)(pattern.size(), mask.size());
    if (!IsValid() || length == 0 || size < length) return 0;

    const std::uintptr_t end = (start + size < start) ? UINTPTR_MAX : start + size;

    std::vector<std::uint8_t> buffer;
    std::uintptr_t cursor = start;

    while (cursor < end)
    {
        MEMORY_BASIC_INFORMATION mbi{};
        if (VirtualQueryEx(m_handle, reinterpret_cast<LPCVOID>(cursor), &mbi, sizeof(mbi)) == 0)
            break; // вышли за пределы адресного пространства

        const std::uintptr_t regionBase = reinterpret_cast<std::uintptr_t>(mbi.BaseAddress);
        const std::uintptr_t regionEnd = (std::min)(end, regionBase + mbi.RegionSize);

        if (regionEnd <= cursor) break; // защита от зацикливания

        if (IsReadable(mbi) && (!executableOnly || IsExecutable(mbi)))
        {
            // Окнами по SCAN_CHUNK с перекрытием length-1: так сигнатура на
            // стыке окон не теряется. Последнее окно региона захватывает
            // начало следующего — совпадение на стыке регионов тоже находится.
            for (std::uintptr_t chunk = cursor; chunk < regionEnd; chunk += SCAN_CHUNK)
            {
                const std::uintptr_t limit = (std::min<std::uintptr_t>)(end, chunk + SCAN_CHUNK + length - 1);
                std::size_t want = static_cast<std::size_t>(limit - chunk);
                if (want < length) break;

                buffer.resize(want);
                SIZE_T got = 0;
                bool ok = ReadProcessMemory(m_handle, reinterpret_cast<LPCVOID>(chunk), buffer.data(), want, &got) != 0;

                // Следующий регион нечитаем — читаем только свой.
                if (!ok && limit > regionEnd)
                {
                    want = static_cast<std::size_t>(regionEnd - chunk);
                    got = 0;
                    ok = want >= length
                      && ReadProcessMemory(m_handle, reinterpret_cast<LPCVOID>(chunk), buffer.data(), want, &got) != 0;
                }

                if (ok && got >= length)
                {
                    const std::size_t found = FindPattern(buffer.data(), got, pattern, mask);
                    if (found != SIZE_MAX) return chunk + found;
                }
            }
        }

        cursor = regionEnd;
    }

    return 0;
}

MemoryAccess::ModuleInfo MemoryAccess::WholeAddressSpace()
{
    SYSTEM_INFO si{};
    GetSystemInfo(&si);

    ModuleInfo range;
    range.base = reinterpret_cast<std::uintptr_t>(si.lpMinimumApplicationAddress);
    range.size = reinterpret_cast<std::uintptr_t>(si.lpMaximumApplicationAddress) - range.base;
    return range;
}

std::uintptr_t MemoryAccess::ResolveChain(std::uintptr_t base, const std::vector<std::uintptr_t>& offsets) const
{
    if (base == 0 || offsets.empty()) return 0;

    std::uintptr_t current = base;
    for (std::size_t i = 0; i + 1 < offsets.size(); ++i)
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
