#pragma once
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
    PBYTE patchBytes = nullptr;
    BYTE originalSize = 0;
    int patchOffset = 0;

    CaveMode mode = CaveMode::ReplaceOriginal;

    // Оборачивать патч в push/pop тех регистров, в которые он пишет.
    // По умолчанию включено: затереть чужой регистр — самая дорогая ошибка
    // в кейвах, падает не сразу и не там.
    bool preserveRegisters = true;

public:
    CavePatch(CheatOption* parentInstance, LPCWSTR signature, PBYTE pBytes, int pSize,
              CaveMode caveMode = CaveMode::ReplaceOriginal,
              bool preserveClobberedRegisters = true)
        : Patch(parentInstance, signature, pSize)
        , mode(caveMode)
        , preserveRegisters(preserveClobberedRegisters)
    {
        patchBytes = new BYTE[pSize];
        memcpy(patchBytes, pBytes, pSize);
    }

    ~CavePatch()
    {
        delete[] patchBytes;
    }

    static PBYTE CalculateJumpBytes(LPVOID from, LPVOID to, BYTE& outSize);

    bool Apply(MemoryAccess& mem) override;
    bool Restore(MemoryAccess& mem) override;
};
