#include "patches/NopPatch.h"

#include <format>

#include "core/Relocator.h"

bool NopPatch::Apply(MemoryAccess& mem)
{
    lastError.clear();

    if (!mem.IsValid()) return Fail("Процесс игры недоступен");
    if (m_applied) return true;

    const uintptr_t address = Locate(mem);
    if (address == 0) return false;

    // Длина не задана — берём первую инструкцию целиком.
    SIZE_T size = patchSize;
    if (size == 0)
    {
        constexpr std::size_t PROBE = 15; // длиннее x86-инструкция не бывает
        const std::vector<uint8_t> probe = mem.ReadBytes(address, PROBE);
        std::size_t length = 0;
        std::string error;

        if (probe.empty() || !Relocator::Measure(probe.data(), probe.size(), mem.IsTargetX64(), 1, length, error))
        {
            return Fail("Не удалось определить длину инструкции для NOP: " + error);
        }
        size = length;
    }

    originalBytes = mem.ReadBytes(address, size);
    if (originalBytes.empty())
    {
        return Fail("Не удалось прочитать оригинальные байты");
    }

    const std::vector<uint8_t> nops(size, 0x90);
    if (!mem.Write(address, nops.data(), nops.size()))
    {
        originalBytes.clear();
        return Fail("Не удалось записать NOP-ы");
    }

    originalAddress = reinterpret_cast<LPVOID>(address);
    m_applied = true;
    return true;
}

bool NopPatch::Restore(MemoryAccess& mem)
{
    // Нечего откатывать — не ошибка: так бывает, если Apply не дошёл до записи.
    if (!m_applied) return true;

    m_applied = false;

    if (!mem.IsAlive() || !originalAddress || originalBytes.empty())
    {
        return true; // игры уже нет — возвращать некуда
    }

    if (!mem.Write(originalAddress, originalBytes.data(), originalBytes.size()))
    {
        return Fail("Не удалось вернуть оригинальные байты");
    }

    return true;
}
