#pragma once
#include <string>
#include <vector>

#include "core/Assembler.h"
#include "patches/Patch.h"

// Что делать с инструкциями, на место которых встал прыжок.
enum class CaveMode
{
    // Патч заменяет их собой — как [ENABLE] в Cheat Engine.
    // Оригинальное поведение в этом месте пропадает.
    ReplaceOriginal,

    // Оригинальные инструкции переносятся в кейв и выполняются после патча.
    // Нужно, когда игре всё ещё нужно то, что они делали: настоящий хук,
    // а не подмена. Адреса внутри них пересчитываются (Relocator).
    KeepOriginal,
};

class CavePatch : public Patch
{
    LPVOID allocatedAddress = nullptr;
    std::vector<BYTE> patchBytes;
    BYTE originalSize = 0;

    // Текст ассемблера, если патч описан им, а не байтами. Собирается
    // при ПРИМЕНЕНИИ, а не здесь: разрядность цели известна только когда
    // процесс открыт, а от неё зависит кодировка.
    std::string patchAsm;
    AsmSyntax asmSyntax = AsmSyntax::Standard;

    CaveMode mode = CaveMode::ReplaceOriginal;

    // Оборачивать патч в push/pop тех регистров, в которые он пишет.
    // По умолчанию включено: затереть чужой регистр — самая дорогая ошибка
    // в кейвах, падает не сразу и не там.
    bool preserveRegisters = true;

    // Освобождает кейв, если он выделен.
    void FreeCave(MemoryAccess& mem);

public:
    CavePatch(CheatOption* parentInstance, LPCWSTR signature, const BYTE* pBytes, SIZE_T pSize,
              CaveMode caveMode = CaveMode::ReplaceOriginal,
              bool preserveClobberedRegisters = true,
              std::ptrdiff_t offset = 0)
        : Patch(parentInstance, signature, pSize, offset)
        , mode(caveMode)
        , preserveRegisters(preserveClobberedRegisters)
    {
        if (pBytes && pSize) patchBytes.assign(pBytes, pBytes + pSize);
    }

    // Патч, описанный текстом ассемблера: "mov [rbx+800], 1000"
    CavePatch(CheatOption* parentInstance, LPCWSTR signature, std::string asmText,
              AsmSyntax syntax = AsmSyntax::Standard,
              CaveMode caveMode = CaveMode::ReplaceOriginal,
              bool preserveClobberedRegisters = true,
              std::ptrdiff_t offset = 0)
        : Patch(parentInstance, signature, 0, offset)
        , patchAsm(std::move(asmText))
        , asmSyntax(syntax)
        , mode(caveMode)
        , preserveRegisters(preserveClobberedRegisters)
    {
    }

    // Байты прыжка from -> to: E9 rel32, если достаёт, иначе FF 25 + адрес.
    // В 32-битной цели E9 достаёт всегда: rel32 там заворачивается по модулю
    // 2^32, а 14-байтная форма с 8-байтным адресом — только для x64.
    static std::vector<BYTE> CalculateJumpBytes(uintptr_t from, uintptr_t to, bool is64Bit = true);

    bool Apply(MemoryAccess& mem) override;
    bool Restore(MemoryAccess& mem) override;

    void Reset() override
    {
        Patch::Reset();
        allocatedAddress = nullptr; // процесса больше нет — освобождать нечего
        originalSize = 0;
    }
};
