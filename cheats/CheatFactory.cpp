#include "cheats/CheatFactory.h"

#include "cheats/CheatDefinition.h"
#include "cheats/CheatOption.h"
#include "platform/Utils.h" // Utf8ToWString

std::unique_ptr<CheatOption> CreateCheatFromDefinition(const CheatDefinition& def)
{
    auto option = std::make_unique<CheatOption>(def.name, def.keys, def.module);
    option->SetAutoOff(def.autoOffMs);
    option->SetHint(def.hint);

    for (const auto& spec : def.patches)
    {
        const std::wstring wsig = Utils::Utf8ToWString(spec.signature);

        switch (spec.kind)
        {
        case PatchSpec::Kind::Nop:
            option->AddNopPatch(wsig.c_str(), static_cast<SIZE_T>(spec.length), spec.offset);
            break;

        case PatchSpec::Kind::Cave:
            if (!spec.patchAsm.empty())
            {
                option->AddCavePatchAsm(wsig.c_str(), spec.patchAsm, spec.asmSyntax,
                                        spec.caveMode, spec.preserveRegisters, spec.offset);
            }
            else
            {
                option->AddCavePatch(wsig.c_str(), spec.patchBytes.data(), spec.patchBytes.size(),
                                     spec.caveMode, spec.preserveRegisters, spec.offset);
            }
            break;

        case PatchSpec::Kind::WriteValue:
        case PatchSpec::Kind::Freeze:
            option->AddWriteValuePatch(spec.offsets, spec.value, spec.absoluteAddress,
                                       spec.kind == PatchSpec::Kind::Freeze
                                           ? WriteAddressPatch::Mode::Freeze
                                           : WriteAddressPatch::Mode::OneShot,
                                       spec.module);
            break;
        }
    }

    return option;
}
