#include "patches/CavePatch.h"

#include <cstring>
#include <memory>
#include <format>
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

    // push reg / pop reg. Для r8-r15 нужен префикс REX.B.
    void EmitPush(std::vector<BYTE>& out, BYTE id, bool is64Bit)
    {
        if (is64Bit && id >= 8) { out.push_back(0x41); out.push_back(static_cast<BYTE>(0x50 + (id - 8))); }
        else                     { out.push_back(static_cast<BYTE>(0x50 + id)); }
    }

    void EmitPop(std::vector<BYTE>& out, BYTE id, bool is64Bit)
    {
        if (is64Bit && id >= 8) { out.push_back(0x41); out.push_back(static_cast<BYTE>(0x58 + (id - 8))); }
        else                     { out.push_back(static_cast<BYTE>(0x58 + id)); }
    }
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

    // --- Пролог и эпилог: спасаем регистры, которые портит патч ---
    //
    // Их состав зависит только от самого патча, поэтому размеры известны
    // заранее — а они нужны, чтобы знать, по какому адресу в кейве лягут
    // перенесённые инструкции.
    std::vector<BYTE> prologue;
    std::vector<BYTE> epilogue;

    if (preserveRegisters)
    {
        const auto clobbered = Relocator::FindClobberedGpRegisters(
            patchBytes, static_cast<size_t>(patchSize), is64BitProcess);

        for (const BYTE id : clobbered)
        {
            EmitPush(prologue, id, is64BitProcess);
        }

        // Снимаем в обратном порядке.
        for (auto it = clobbered.rbegin(); it != clobbered.rend(); ++it)
        {
            EmitPop(epilogue, *it, is64BitProcess);
        }

        if (!clobbered.empty())
        {
            Log::Debug(std::format("Кейв сохраняет {} регистр(ов), которые портит патч",
                                   clobbered.size()));
        }
    }

    // Куда в кейве лягут перенесённые оригинальные инструкции.
    const uintptr_t stolenAtCave = reinterpret_cast<uintptr_t>(allocatedAddress)
                                 + prologue.size() + patchSize + epilogue.size();

    // Сколько ЦЕЛЫХ инструкций займёт прыжок — считает Zydis.
    // Раньше это делал длино-дизассемблер, который на непонятных байтах
    // возвращал 0: счётчик не двигался и цикл висел вечно.
    std::vector<BYTE> relocated;
    size_t stolen = 0;

    if (mode == CaveMode::KeepOriginal)
    {
        const RelocationResult reloc = Relocator::Relocate(
            reinterpret_cast<uintptr_t>(originalAddress),
            originalBytes, PROBE_SIZE,
            is64BitProcess, stolenAtCave, jmpSize);

        if (!reloc.ok)
        {
            freeCave();
            Log::Error("Патч невозможен: " + reloc.error);
            return false;
        }

        relocated = reloc.bytes;
        stolen = reloc.stolen;
    }
    else
    {
        // Инструкции выбрасываем — пересчитывать нечего, нужна только
        // граница. Отдельный путь, чтобы не отказывать из-за инструкции,
        // которую мы всё равно не переносим.
        std::string error;
        if (!Relocator::Measure(originalBytes, PROBE_SIZE, is64BitProcess, jmpSize, stolen, error))
        {
            freeCave();
            Log::Error("Патч невозможен: " + error);
            return false;
        }
    }

    if (stolen == 0 || stolen > PROBE_SIZE || stolen > 0xFF)
    {
        freeCave();
        return false;
    }

    originalSize = static_cast<BYTE>(stolen);

    // --- Сборка кейва ---
    //
    //   push портящихся регистров
    //   код патча
    //   pop  их же
    //   перенесённые оригинальные инструкции   (только в режиме KeepOriginal)
    //   прыжок обратно — на адрес сразу за украденными байтами
    std::vector<BYTE> cave;
    cave.insert(cave.end(), prologue.begin(), prologue.end());
    cave.insert(cave.end(), patchBytes, patchBytes + patchSize);
    cave.insert(cave.end(), epilogue.begin(), epilogue.end());
    cave.insert(cave.end(), relocated.begin(), relocated.end());

    BYTE backSize = 0;
    const std::unique_ptr<BYTE[]> backJmp(CalculateJumpBytes(
        static_cast<PBYTE>(allocatedAddress) + cave.size(),
        static_cast<PBYTE>(originalAddress) + originalSize,
        backSize));

    cave.insert(cave.end(), backJmp.get(), backJmp.get() + backSize);

    if (cave.size() > CAVE_SIZE)
    {
        freeCave();
        Log::Error("Кейв не помещается в выделенный блок");
        return false;
    }

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
