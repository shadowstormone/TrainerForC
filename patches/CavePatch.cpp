#include "patches/CavePatch.h"

#include <cstring>
#include <memory>
#include <stdexcept>
#include <vector>

#include "cheats/CheatOption.h"
#include "core/Relocator.h"
#include "platform/Logger.h"

namespace
{
    constexpr size_t CAVE_SIZE = 4096;

    // Читаем с запасом: одна инструкция бывает до 15 байт, а украсть
    // приходится до нескольких.
    constexpr size_t PROBE_SIZE = 15 * 4;
}

PBYTE CavePatch::CalculateJumpBytes(LPVOID from, LPVOID to, BYTE& outSize)
{
    const uintptr_t delta = reinterpret_cast<uintptr_t>(to) - reinterpret_cast<uintptr_t>(from);
    const uintptr_t normalized_delta = std::abs(static_cast<intptr_t>(delta));

    PBYTE bytes;

    if (normalized_delta < (1ULL << 31))
    {
        // E9 rel32 — короткий прыжок, достаёт в пределах ±2 ГБ
        bytes = new BYTE[5];
        bytes[0] = 0xE9;
        const uint32_t relative_addr = static_cast<uint32_t>(delta - 5);
        std::memcpy(bytes + 1, &relative_addr, sizeof(relative_addr));
        outSize = 5;
    }
    else
    {
        // FF 25 00000000 + абсолютный адрес — прыжок куда угодно, но 14 байт
        bytes = new BYTE[14];
        bytes[0] = 0xFF;
        bytes[1] = 0x25;
        std::memset(bytes + 2, 0, 4);
        const uintptr_t to_address = reinterpret_cast<uintptr_t>(to);
        std::memcpy(bytes + 6, &to_address, sizeof(to_address));
        outSize = 14;
    }

    return bytes;
}

bool CavePatch::Apply(MemoryAccess& mem)
{
    originalSize = 0;

    if (!mem.IsValid()) return false;

    const bool is64BitProcess = mem.IsTargetX64();
    const uintptr_t scanSize = is64BitProcess ? 0x7FFFFFFFFFFFFFFF : 0x7FFFFFFF;

    const LPCWSTR moduleName = parent ? parent->GetModuleName() : nullptr;
    const uintptr_t baseAddress = (moduleName && wcslen(moduleName) > 0)
                                      ? mem.ModuleBase(moduleName)
                                      : mem.ProcessBase();
    if (!baseAddress)
    {
        Log::Error("Не удалось получить базовый адрес процесса");
        return false;
    }

    patternAddress = mem.ScanSignature(baseAddress, scanSize, pattern.data(), mask);
    if (!patternAddress)
    {
        // Раньше отсюда шли дальше с нулём и патчили по адресу patchOffset.
        Log::Error("Сигнатура не найдена — патч не применён");
        return false;
    }

    patchAddress = static_cast<LPBYTE>(patternAddress) + patchOffset;
    originalAddress = reinterpret_cast<LPVOID>(patchAddress);

    originalBytes = static_cast<PBYTE>(mem.Read(originalAddress, PROBE_SIZE));
    if (!originalBytes)
    {
        Log::Error("Не удалось прочитать оригинальные байты");
        return false;
    }

    // Кейв рядом с целью — тогда хватит короткого прыжка и красть придётся
    // меньше инструкций.
    allocatedAddress = mem.AllocNear(reinterpret_cast<uintptr_t>(originalAddress), CAVE_SIZE);
    if (!allocatedAddress)
    {
        Log::Error("Не удалось выделить память под кейв");
        return false;
    }

    const auto freeCave = [&]()
    {
        if (allocatedAddress)
        {
            mem.Free(allocatedAddress, CAVE_SIZE);
            allocatedAddress = nullptr;
        }
        originalSize = 0;
    };

    BYTE jmpSize = 0;
    const std::unique_ptr<BYTE[]> jmpBytes(CalculateJumpBytes(originalAddress, allocatedAddress, jmpSize));

    // Сколько ЦЕЛЫХ инструкций займёт прыжок — считает Zydis.
    // Раньше это делал длино-дизассемблер, который на непонятных байтах
    // возвращал 0: счётчик не двигался и цикл висел вечно.
    const RelocationResult reloc = Relocator::Relocate(
        reinterpret_cast<uintptr_t>(originalAddress),
        originalBytes, PROBE_SIZE,
        is64BitProcess,
        reinterpret_cast<uintptr_t>(allocatedAddress),
        jmpSize);

    if (!reloc.ok)
    {
        freeCave();
        Log::Error("Патч невозможен: " + reloc.error);
        return false;
    }

    originalSize = static_cast<BYTE>(reloc.stolen);

    if (originalSize > PROBE_SIZE)
    {
        freeCave();
        return false;
    }

    // --- Кейв: код пользователя, затем прыжок обратно ---
    //
    // Украденные инструкции сюда НЕ переносятся: патч заменяет их собой,
    // как в [ENABLE]-скрипте Cheat Engine. Возврат идёт на адрес сразу
    // за ними, поэтому красть нужно целые инструкции — иначе вернёмся
    // в середину следующей.
    BYTE backSize = 0;
    const std::unique_ptr<BYTE[]> backJmp(CalculateJumpBytes(
        static_cast<PBYTE>(allocatedAddress) + patchSize,
        static_cast<PBYTE>(originalAddress) + originalSize,
        backSize));

    std::vector<BYTE> cave(patchSize + backSize);
    std::memcpy(cave.data(), patchBytes, patchSize);
    std::memcpy(cave.data() + patchSize, backJmp.get(), backSize);

    if (!mem.Write(allocatedAddress, cave.data(), cave.size()))
    {
        freeCave();
        Log::Error("Не удалось записать кейв");
        return false;
    }

    // --- На месте патча: прыжок, остаток добиваем NOP-ами ---
    std::vector<BYTE> site(originalSize, 0x90);
    std::memcpy(site.data(), jmpBytes.get(), jmpSize);

    if (!mem.Write(originalAddress, site.data(), site.size()))
    {
        freeCave();
        Log::Error("Не удалось записать прыжок на месте патча");
        return false;
    }

    return true;
}

bool CavePatch::Restore(MemoryAccess& mem)
{
    // Возвращаем оригинальные байты
    if (mem.IsValid() && originalAddress && originalAddress != INVALID_HANDLE_VALUE && originalSize > 0)
    {
        DWORD exitCode;
        if (GetExitCodeProcess(mem.Handle(), &exitCode) && exitCode == STILL_ACTIVE)
        {
            MEMORY_BASIC_INFORMATION mbi;
            if (VirtualQueryEx(mem.Handle(), (LPCVOID)originalAddress, &mbi, sizeof(mbi)) != 0)
            {
                if (mbi.State == MEM_COMMIT)
                {
                    mem.Write(originalAddress, originalBytes, originalSize);
                }
            }
        }
    }

    // Освобождаем кейв
    if (mem.IsValid() && allocatedAddress && allocatedAddress != INVALID_HANDLE_VALUE)
    {
        DWORD exitCode;
        if (GetExitCodeProcess(mem.Handle(), &exitCode) && exitCode == STILL_ACTIVE)
        {
            MEMORY_BASIC_INFORMATION mbi;
            if (VirtualQueryEx(mem.Handle(), (LPCVOID)allocatedAddress, &mbi, sizeof(mbi)) != 0)
            {
                if (mbi.State == MEM_COMMIT)
                {
                    mem.Free(allocatedAddress, CAVE_SIZE);
                }
            }
        }

        allocatedAddress = nullptr;
    }

    originalSize = 0;
    return true;
}
