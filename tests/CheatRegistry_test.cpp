#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <map>
#include <string>
#include <variant>
#include <vector>

#include "cheats/CheatRegistry.h"
#include "cheats/ValueFieldRegistry.h"
#include "core/Assembler.h"
#include "patches/Patch.h"
#include "platform/KeyNames.h"
#include "platform/Utils.h"

// Реестр наполняется макросами REGISTER_CHEAT в cheats/registry/*.cpp
// через статическую инициализацию — до main().
//
// Эти тесты не сверяют КОНКРЕТНЫЕ читы (иначе ломались бы при каждой
// правке реестра), а проверяют, что всё, что в нём написано, годится:
// сигнатуры разбираются, ассемблер собирается, клавиши не конфликтуют.
// Это та проверка, которую иначе пришлось бы делать запуском игры.

namespace
{
    std::string NameOf(const CheatDefinition& cheat)
    {
        return Utils::WStringToUtf8(cheat.name);
    }
}

TEST(CheatRegistry, PopulatedByStaticInit)
{
    EXPECT_FALSE(CheatRegistry::Instance().All().empty());
}

TEST(RegistryValidation, EveryCheatHasNameAndPatches)
{
    for (const CheatDefinition& cheat : CheatRegistry::Instance().All())
    {
        EXPECT_FALSE(cheat.name.empty());
        EXPECT_FALSE(cheat.patches.empty()) << NameOf(cheat) << ": нет ни одного патча";
        EXPECT_GE(cheat.autoOffMs, 0) << NameOf(cheat);
    }
}

TEST(RegistryValidation, NamesAreUnique)
{
    std::map<std::wstring, int> seen;
    for (const CheatDefinition& cheat : CheatRegistry::Instance().All())
    {
        EXPECT_EQ(++seen[cheat.name], 1) << "Имя повторяется: " << NameOf(cheat);
    }
}

TEST(RegistryValidation, HotkeysDoNotCollide)
{
    // Одна комбинация на два чита — нажатие переключит оба сразу.
    std::map<std::vector<int>, std::string> owners;

    for (const CheatDefinition& cheat : CheatRegistry::Instance().All())
    {
        if (cheat.keys.empty()) continue;

        std::vector<int> keys = cheat.keys;
        std::sort(keys.begin(), keys.end());

        const auto [it, inserted] = owners.emplace(keys, NameOf(cheat));
        EXPECT_TRUE(inserted) << KeyNames::Hotkey(cheat.keys) << " назначена и на «"
                              << it->second << "», и на «" << NameOf(cheat) << "»";
    }
}

TEST(RegistryValidation, SignaturesParse)
{
    for (const CheatDefinition& cheat : CheatRegistry::Instance().All())
    {
        for (const PatchSpec& spec : cheat.patches)
        {
            if (spec.kind != PatchSpec::Kind::Nop && spec.kind != PatchSpec::Kind::Cave) continue;

            std::vector<std::uint8_t> pattern;
            std::wstring mask;
            std::string error;
            const std::wstring text = Utils::Utf8ToWString(spec.signature);

            EXPECT_TRUE(ParseSignature(text.c_str(), pattern, mask, error))
                << NameOf(cheat) << ": " << error;
        }
    }
}

TEST(RegistryValidation, AssemblerPatchesAssemble)
{
    // Разрядность игры тут неизвестна, поэтому годится любая: патч, который
    // не собирается ни под x64, ни под x86, точно написан с ошибкой.
    for (const CheatDefinition& cheat : CheatRegistry::Instance().All())
    {
        for (const PatchSpec& spec : cheat.patches)
        {
            if (spec.patchAsm.empty()) continue;

            const AssembleResult x64 = Assembler::Assemble(spec.patchAsm, true, 0, spec.asmSyntax);
            const AssembleResult x86 = Assembler::Assemble(spec.patchAsm, false, 0, spec.asmSyntax);

            EXPECT_TRUE(x64.ok || x86.ok) << NameOf(cheat) << ": " << x64.error;
        }
    }
}

TEST(RegistryValidation, ByteCavesAreNotEmpty)
{
    for (const CheatDefinition& cheat : CheatRegistry::Instance().All())
    {
        for (const PatchSpec& spec : cheat.patches)
        {
            if (spec.kind == PatchSpec::Kind::Cave && spec.patchAsm.empty())
            {
                EXPECT_FALSE(spec.patchBytes.empty()) << NameOf(cheat) << ": кейв без кода";
            }
        }
    }
}

TEST(RegistryValidation, ValueAddressesHaveOffsets)
{
    for (const CheatDefinition& cheat : CheatRegistry::Instance().All())
    {
        for (const PatchSpec& spec : cheat.patches)
        {
            if (spec.kind == PatchSpec::Kind::WriteValue || spec.kind == PatchSpec::Kind::Freeze)
            {
                EXPECT_FALSE(spec.offsets.empty()) << NameOf(cheat) << ": у адреса нет ни одного оффсета";
            }
        }
    }

    for (const ValueFieldDefinition& field : ValueFieldRegistry::Instance().All())
    {
        EXPECT_FALSE(field.name.empty());
        EXPECT_FALSE(field.offsets.empty()) << Utils::WStringToUtf8(field.name);
    }
}

// --- Сам язык описания читов ---

TEST(CheatDsl, DesignatedFieldsFillDefinition)
{
    const CheatDefinition cheat{
        .name      = L"Test",
        .keys      = { 0x61 },
        .patches   = { Nop("90 90") },
        .autoOffMs = 300,
        .module    = L"Game.dll",
        .hint      = L"Подсказка",
    };

    EXPECT_EQ(cheat.autoOffMs, 300);
    EXPECT_EQ(cheat.module, L"Game.dll");
    EXPECT_EQ(cheat.hint, L"Подсказка");
    EXPECT_EQ(cheat.patches.size(), 1u);
}

TEST(CheatDsl, WriteValueTypeFollowsLiteral)
{
    EXPECT_TRUE(std::holds_alternative<std::int32_t>(WriteValue(Address::Module(1), 5).value));
    EXPECT_TRUE(std::holds_alternative<float>(WriteValue(Address::Module(1), 5.0f).value));
    EXPECT_TRUE(std::holds_alternative<double>(WriteValue(Address::Module(1), 5.0).value));
    EXPECT_TRUE(std::holds_alternative<std::int64_t>(WriteValue(Address::Module(1), std::int64_t{ 5 }).value));
}

TEST(CheatDsl, AddressChainReadsLikeCheatEngine)
{
    const PatchSpec spec = WriteValue(Address::Module(0x346C10).Deref(0x18).Deref(0x800), 1);
    EXPECT_EQ(spec.offsets, (std::vector<std::uintptr_t>{ 0x346C10, 0x18, 0x800 }));
    EXPECT_FALSE(spec.absoluteAddress);
    EXPECT_TRUE(spec.module.empty());

    const PatchSpec chained = WriteValue(Address::Module(L"UnityPlayer.dll", 0x10).Chain({ 0x20, 0x30 }), 1);
    EXPECT_EQ(chained.offsets, (std::vector<std::uintptr_t>{ 0x10, 0x20, 0x30 }));
    EXPECT_EQ(chained.module, L"UnityPlayer.dll");
}

TEST(CheatDsl, FreezeIsItsOwnKind)
{
    const PatchSpec spec = Freeze(Address::Module(0x100), 1000);
    EXPECT_EQ(spec.kind, PatchSpec::Kind::Freeze);
    EXPECT_EQ(std::get<std::int32_t>(spec.value), 1000);
}

TEST(CheatDsl, NopWithoutLengthMeansOneInstruction)
{
    EXPECT_EQ(Nop("90").length, 0u);
    EXPECT_EQ(Nop("90", 6).length, 6u);
}

TEST(CheatDsl, AtShiftsPatchSite)
{
    EXPECT_EQ(Cave("AA BB", Asm("nop")).At(6).offset, 6);
    EXPECT_EQ(Nop("AA BB").At(2).offset, 2);
}

TEST(CheatDsl, AsmCeSelectsCheatEngineSyntax)
{
    EXPECT_EQ(Cave("AA", Asm("nop")).asmSyntax, AsmSyntax::Standard);
    EXPECT_EQ(Cave("AA", AsmCE("nop")).asmSyntax, AsmSyntax::CheatEngine);
    EXPECT_EQ(CaveKeepOriginal("AA", AsmCE("nop")).caveMode, CaveMode::KeepOriginal);
}

TEST(CheatDsl, ValueFieldTypeFollowsDefault)
{
    EXPECT_TRUE(std::holds_alternative<std::int32_t>(ValueField(L"a", Address::Module(1)).defaultValue));
    EXPECT_TRUE(std::holds_alternative<float>(ValueField(L"a", Address::Module(1), 1.5f).defaultValue));
}

// --- Подсветка чисел в консоли ---

#include "ui/ImGuiConsole.h"

namespace
{
    // Подсвеченные куски сообщения, через запятую.
    std::string Accents(const std::string& text)
    {
        std::string out;
        for (const auto& [part, number] : Console::SplitAccents(text))
        {
            if (!number) continue;
            if (!out.empty()) out += ",";
            out += part;
        }
        return out;
    }
}

TEST(ConsoleAccents, HexAddressesAreHighlightedWhole)
{
    // Регрессия: подсвечивалась только первая цифра — "7", а "FF612340000" нет.
    EXPECT_EQ(Accents("mem 7FF612340000 16"), "7FF612340000,16");
    EXPECT_EQ(Accents("ptr 346C10 800"), "346C10,800");
    EXPECT_EQ(Accents("Адрес: 0x3E8, база 0x7FF6A0000000"), "0x3E8,0x7FF6A0000000");
    EXPECT_EQ(Accents("(exe+1480)"), "1480");
}

TEST(ConsoleAccents, DigitsInsideNamesAreNotNumbers)
{
    EXPECT_EQ(Accents("Tutorial-x86_64.exe"), "");
    EXPECT_EQ(Accents("Num1 и r8d"), "");
    EXPECT_EQ(Accents("1st"), "");
}

TEST(ConsoleAccents, DecimalsAndPlainText)
{
    EXPECT_EQ(Accents("Версия: 1.0, PID 40"), "1.0,40");
    EXPECT_EQ(Accents("Screen resolution: 1920x1080"), "1920,1080");
    EXPECT_EQ(Accents("без чисел"), "");

    std::string joined;
    for (const auto& [part, number] : Console::SplitAccents("ptr 346C10 800")) joined += part;
    EXPECT_EQ(joined, "ptr 346C10 800") << "куски складываются обратно в исходный текст";
}
