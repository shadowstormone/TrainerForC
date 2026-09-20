#include "cheats/CheatOptionFactory.h"
#include "cheats/CheatOption.h"
#include "platform/Utils.h" // Utf8ToWString
#include <variant>

using namespace CheatOptionDefinitions;

std::unique_ptr<CheatOption> CreateOptionFromDefinition(const CheatOptionDefinitions::OptionDefinition& def, Cheat* game)
{

    // Создаём CheatOption (moduleName = nullptr)
    auto option = std::make_unique<CheatOption>(nullptr, def.name.c_str(), def.keys);

    std::visit([&](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<T, std::monostate>)
        {
            // ничего
        }
        else if constexpr (std::is_same_v<T, WriteValueParams>)
        {
            std::visit([&](auto&& val) {
                using V = std::decay_t<decltype(val)>;
                if constexpr (std::is_same_v<V, int>)
                {
                    option->AddWriteValuePatch(game, arg.offsets, val);
                }
                else if constexpr (std::is_same_v<V, float>)
                {
                    option->AddWriteValuePatch(game, arg.offsets, val);
                }
                else if constexpr (std::is_same_v<V, double>)
                {
                    option->AddWriteValuePatch(game, arg.offsets, val);
                }
                }, arg.value);
        }
        else if constexpr (std::is_same_v<T, CavePatchParams>)
        {
            // конвертируем signature в wide
            std::wstring wsig = Utils::Utf8ToWString(arg.signature);

            // patchData указывает на данные из PatchLibrary (static lifetime)
            PBYTE pBytes = reinterpret_cast<PBYTE>(const_cast<uint8_t*>(arg.patchData));
            SIZE_T writeSize = arg.patchSize;

            // ВАЖНО: мы НЕ выдвеляем tmp буферы и НЕ удаляем их — pointer живёт в PatchLibrary
            option->AddCavePatch(wsig.c_str(), pBytes, writeSize);
        }
        else if constexpr (std::is_same_v<T, NopParams>)
        {
            std::wstring wsig = Utils::Utf8ToWString(arg.signature);
            option->AddNopPatch(wsig.c_str(), static_cast<SIZE_T>(arg.length));
        }
        }, def.action);

    return option;
}