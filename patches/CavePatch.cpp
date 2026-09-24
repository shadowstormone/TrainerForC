#include "patches/CavePatch.h"

#include <cstring>
#include <format>
#include <vector>

#include "cheats/CheatOption.h"
#include "core/Assembler.h"
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

std::vector<BYTE> CavePatch::CalculateJumpBytes(uintptr_t from, uintptr_t to, bool is64Bit)
{
    const std::int64_t delta = static_cast<std::int64_t>(to) - static_cast<std::int64_t>(from + 5);

    if (!is64Bit || (delta >= INT32_MIN && delta <= INT32_MAX))
    {
        // E9 rel32 — прыжок в пределах ±2 ГБ
        std::vector<BYTE> bytes(5);
        bytes[0] = 0xE9;
        const std::uint32_t rel = static_cast<std::uint32_t>(delta);
        std::memcpy(bytes.data() + 1, &rel, sizeof(rel));
        return bytes;
    }

    // FF 25 00000000 + абсолютный адрес — прыжок куда угодно, но 14 байт
    std::vector<BYTE> bytes(14, 0);
    bytes[0] = 0xFF;
    bytes[1] = 0x25;
    const std::uint64_t target = to;
    std::memcpy(bytes.data() + 6, &target, sizeof(target));
    return bytes;
}

void CavePatch::FreeCave(MemoryAccess& mem)
{
    if (allocatedAddress)
    {
        mem.Free(allocatedAddress);
        allocatedAddress = nullptr;
    }
    originalSize = 0;
}

bool CavePatch::Apply(MemoryAccess& mem)
{
    lastError.clear();

    if (!mem.IsValid()) return Fail("Процесс игры недоступен");

    // Уже стоит — второй раз не ставим: иначе потеряли бы настоящий оригинал.
    if (originalSize > 0) return true;

    const bool is64BitProcess = mem.IsTargetX64();

    // Код патча. Если он задан текстом — собираем ИМЕННО СЕЙЧАС, под
    // разрядность цели: одна и та же мнемоника кодируется по-разному.
    std::vector<BYTE> code;
    if (!patchAsm.empty())
    {
        const AssembleResult assembled = Assembler::Assemble(patchAsm, is64BitProcess, 0, asmSyntax);
        if (!assembled.ok)
        {
            return Fail("Ассемблер: " + assembled.error);
        }
        code.assign(assembled.bytes.begin(), assembled.bytes.end());
    }
    else
    {
        code = patchBytes;
    }

    if (code.empty()) return Fail("Патч пуст");

    const uintptr_t site = Locate(mem);
    if (site == 0) return false;

    originalAddress = reinterpret_cast<LPVOID>(site);

    originalBytes = mem.ReadBytes(site, PROBE_SIZE);
    if (originalBytes.empty())
    {
        return Fail("Не удалось прочитать оригинальные байты");
    }

    // Кейв рядом с целью — тогда хватит короткого прыжка и красть придётся
    // меньше инструкций.
    allocatedAddress = mem.AllocNear(site, CAVE_SIZE);
    if (!allocatedAddress)
    {
        return Fail("Не удалось выделить память под кейв");
    }

    const uintptr_t cave = reinterpret_cast<uintptr_t>(allocatedAddress);
    const std::vector<BYTE> jmpBytes = CalculateJumpBytes(site, cave, is64BitProcess);

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
            code.data(), code.size(), is64BitProcess);

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
    const uintptr_t stolenAtCave = cave + prologue.size() + code.size() + epilogue.size();

    // Сколько ЦЕЛЫХ инструкций займёт прыжок — считает Zydis.
    std::vector<BYTE> relocated;
    size_t stolen = 0;

    if (mode == CaveMode::KeepOriginal)
    {
        const RelocationResult reloc = Relocator::Relocate(
            site, originalBytes.data(), originalBytes.size(),
            is64BitProcess, stolenAtCave, jmpBytes.size());

        if (!reloc.ok)
        {
            FreeCave(mem);
            return Fail("Патч невозможен: " + reloc.error);
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
        if (!Relocator::Measure(originalBytes.data(), originalBytes.size(), is64BitProcess,
                                jmpBytes.size(), stolen, error))
        {
            FreeCave(mem);
            return Fail("Патч невозможен: " + error);
        }
    }

    if (stolen == 0 || stolen > PROBE_SIZE || stolen > 0xFF)
    {
        FreeCave(mem);
        return Fail("Не удалось определить границу инструкций на месте патча");
    }

    // --- Сборка кейва ---
    //
    //   push портящихся регистров
    //   код патча
    //   pop  их же
    //   перенесённые оригинальные инструкции   (только в режиме KeepOriginal)
    //   прыжок обратно — на адрес сразу за украденными байтами
    std::vector<BYTE> caveCode;
    caveCode.insert(caveCode.end(), prologue.begin(), prologue.end());
    caveCode.insert(caveCode.end(), code.begin(), code.end());
    caveCode.insert(caveCode.end(), epilogue.begin(), epilogue.end());
    caveCode.insert(caveCode.end(), relocated.begin(), relocated.end());

    const std::vector<BYTE> backJmp = CalculateJumpBytes(cave + caveCode.size(), site + stolen, is64BitProcess);
    caveCode.insert(caveCode.end(), backJmp.begin(), backJmp.end());

    if (caveCode.size() > CAVE_SIZE)
    {
        FreeCave(mem);
        return Fail("Кейв не помещается в выделенный блок");
    }

    if (!mem.Write(allocatedAddress, caveCode.data(), caveCode.size()))
    {
        FreeCave(mem);
        return Fail("Не удалось записать кейв");
    }

    // --- На месте патча: прыжок, остаток добиваем NOP-ами ---
    std::vector<BYTE> siteBytes(stolen, 0x90);
    std::memcpy(siteBytes.data(), jmpBytes.data(), jmpBytes.size());

    if (!mem.Write(site, siteBytes.data(), siteBytes.size()))
    {
        FreeCave(mem);
        return Fail("Не удалось записать прыжок на месте патча");
    }

    originalSize = static_cast<BYTE>(stolen);
    return true;
}

bool CavePatch::Restore(MemoryAccess& mem)
{
    if (originalSize == 0) return true; // не стоит — откатывать нечего

    bool ok = true;

    if (mem.IsAlive())
    {
        // Сначала возвращаем код игры и только потом освобождаем кейв:
        // в обратном порядке игра могла бы прыгнуть в уже освобождённую память.
        if (originalAddress && !originalBytes.empty())
        {
            ok = mem.Write(originalAddress, originalBytes.data(), originalSize);
            if (!ok) Fail("Не удалось вернуть оригинальные байты");
        }

        // Кейв освобождаем, только если код вернулся: иначе прыжок остался
        // бы в пустоту.
        if (ok) FreeCave(mem);
    }
    else
    {
        allocatedAddress = nullptr; // процесса нет — и памяти его нет
    }

    originalSize = 0;
    return ok;
}
