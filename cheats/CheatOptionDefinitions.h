#pragma once
#include <string>
#include <vector>
#include <variant>
#include <cstdint>
#include <stdexcept>
#include <string_view>
#include <cstddef>

#include "platform/VKeys.h"
#include "patches/PatchLibrary.h"

namespace CheatOptionDefinitions
{
    using WriteValueVariant = std::variant<int, float, double>;

    struct WriteValueParams
    {
        std::vector<std::uintptr_t> offsets;
        WriteValueVariant value;
    };

    // Для совместимости с твоим AddCavePatch(LPCWSTR, PBYTE, SIZE_T)
    // храним signature как utf-8 string, и указатель на patchData (static lifetime)
    struct CavePatchParams
    {
        std::string signature;        // AOB как utf-8
        const uint8_t* patchData;     // указатель на данные из PatchLibrary (static lifetime)
        std::size_t patchSize;        // число байт для записи (alignment)
    };

    struct NopParams
    {
        std::string signature;
        int length;
    };

    using OptionAction = std::variant<std::monostate, WriteValueParams, CavePatchParams, NopParams>;

    enum class OptionID
    {
        CHEAT_TEST_1,
        SET_HP_9999,
        CHEAT_TEST_3,
        FIRST_FUNCTION_NOP
    };

    struct OptionDefinition
    {
        OptionID id;
        std::wstring name;
        std::vector<int> keys;
        OptionAction action;
        bool autoDisable = false;
        int autoDisableDelay = 0;
    };

    // Макрос-хелпер для компактного создания CavePatchParams с PatchLibrary
#define MAKE_CAVE(sig, patch_arr, size_const) \
    CavePatchParams{ std::string(sig), reinterpret_cast<const uint8_t*>((patch_arr).data()), static_cast<std::size_t>(size_const) }

    inline const std::vector<OptionDefinition> AllOptions = {
        {
            OptionID::CHEAT_TEST_1,
            L"[Numpad 1] - Cheat Test 1",
            { VKeys::KEY_NUMPAD1 },
            MAKE_CAVE(PatchLibrary::SIG_CHEAT_TEST_3, PatchLibrary::PATCH_CHEAT_TEST_3, PatchLibrary::SIZE_CHEAT_TEST_3),
            false
        },
        {
            OptionID::SET_HP_9999,
            L"[Numpad 2] - Set 9999 HP",
            { VKeys::KEY_NUMPAD2 },
            WriteValueParams{ std::vector<std::uintptr_t>{ 0x00240600, 0x4B4 }, static_cast<int>(9999) },
            true, 450
        },
        {
            OptionID::CHEAT_TEST_3,
            L"[Numpad 3] - Cheat Test 3",
            { VKeys::KEY_NUMPAD3 },
            MAKE_CAVE(PatchLibrary::SIG_CHEAT_TEST_3, PatchLibrary::PATCH_CHEAT_TEST_3, PatchLibrary::SIZE_CHEAT_TEST_3),
            false
        },
        {
            OptionID::FIRST_FUNCTION_NOP,
            L"[Numpad 4] - First Function(Nop)",
            { VKeys::KEY_NUMPAD4 },
            NopParams{static_cast<std::string>(PatchLibrary::SIG_CHEAT_TEST_3), 6},
            false
        }

        // Добавляй другие опции здесь...
    };

#undef MAKE_CAVE

    inline const OptionDefinition& GetOptionById(OptionID id)
    {
        for (const auto& def : AllOptions)
        {
            if (def.id == id)
            {
                return def;
            }
        }
        throw std::runtime_error("Option ID not found");
    }

} // namespace CheatOptionDefinitions