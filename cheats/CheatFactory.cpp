#include "cheats/CheatFactory.h"

#include <variant>

#include "cheats/CheatDefinition.h"
#include "cheats/CheatOption.h"
#include "core/Cheat.h"
#include "platform/Utils.h" // Utf8ToWString

std::unique_ptr<CheatOption> CreateCheatFromDefinition(const CheatDefinition& def, Cheat* game)
{
    // Имя берётся из определения, которое живёт в реестре всё время работы
    // программы, поэтому c_str() остаётся валидным.
    auto option = std::make_unique<CheatOption>(nullptr, def.name.c_str(), def.keys);

    for (const auto& spec : def.patches)
    {
        switch (spec.kind)
        {
        case PatchSpec::Kind::Nop:
        {
            const std::wstring wsig = Utils::Utf8ToWString(spec.signature);
            option->AddNopPatch(wsig.c_str(), static_cast<SIZE_T>(spec.length));
            break;
        }

        case PatchSpec::Kind::Cave:
        {
            const std::wstring wsig = Utils::Utf8ToWString(spec.signature);
            if (!spec.patchAsm.empty())
            {
                option->AddCavePatchAsm(wsig.c_str(), spec.patchAsm,
                                        spec.caveMode, spec.preserveRegisters);
                break;
            }

            // Байты живут в определении чита, а оно — в реестре, то есть всё
            // время работы программы.
            auto* bytes = reinterpret_cast<PBYTE>(const_cast<std::uint8_t*>(spec.patchBytes.data()));
            option->AddCavePatch(wsig.c_str(), bytes, static_cast<SIZE_T>(spec.patchBytes.size()),
                                 spec.caveMode, spec.preserveRegisters);
            break;
        }

        case PatchSpec::Kind::WriteValue:
        {
            std::visit(
                [&](auto&& v)
                {
                    option->AddWriteValuePatch(game, spec.offsets, v, spec.absoluteAddress);
                },
                spec.value);
            break;
        }
        }
    }

    return option;
}
