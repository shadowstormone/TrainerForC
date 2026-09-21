#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "cheats/CheatDefinition.h"
#include "patches/Patch.h"

namespace
{
    // Patch прячет разбор сигнатуры — открываем его только для теста.
    class PatternProbe : public Patch
    {
    public:
        using Patch::convertPattern;

        const std::vector<std::uint8_t>& Bytes() const { return pattern; }
        const std::wstring& Mask() const { return mask; }

        bool Apply(MemoryAccess&) override { return false; }
        bool Restore(MemoryAccess&) override { return false; }
    };
}

// --- Сигнатуры ---

TEST(Signature, CheatEngineStyle)
{
    PatternProbe p;
    p.convertPattern(L"29 93 ?? ?? 8B");

    EXPECT_EQ(p.Mask(), L"xx??x");
    EXPECT_EQ(p.Bytes(), (std::vector<std::uint8_t>{ 0x29, 0x93, 0x00, 0x00, 0x8B }));
}

TEST(Signature, StarWildcardWithHexPrefixIsNotLiteralZero)
{
    // Регрессия: "0x**" уходил в разбор числа, wcstoul давал 0, маска
    // ставилась 'x' — и сигнатура требовала здесь байт 0x00.
    PatternProbe p;
    p.convertPattern(L"0x29, 0x93, 0x**, 0x**, 0x8B");

    EXPECT_EQ(p.Mask(), L"xx??x");
    EXPECT_EQ(p.Bytes().size(), 5u);
}

TEST(Signature, AllWildcardSpellingsAccepted)
{
    PatternProbe p;
    p.convertPattern(L"AA ? ?? * ** xx XX BB");

    EXPECT_EQ(p.Mask(), L"x??????x");
    EXPECT_EQ(p.Bytes().front(), 0xAA);
    EXPECT_EQ(p.Bytes().back(), 0xBB);
}

TEST(Signature, BothStylesGiveSameResult)
{
    PatternProbe ce, prefixed;
    ce.convertPattern(L"29 93 ?? ?? 41 B9");
    prefixed.convertPattern(L"0x29, 0x93, 0x**, 0x**, 0x41, 0xB9");

    EXPECT_EQ(ce.Mask(), prefixed.Mask());
    EXPECT_EQ(ce.Bytes(), prefixed.Bytes());
}

// --- Байты патча ---

TEST(ParseBytes, LengthIsCountedAutomatically)
{
    const auto bytes = ParseBytes("48 BE E8 03 00 00 00 00 00 00 48 89 B3 00 08 00 00");

    EXPECT_EQ(bytes.size(), 17u);
    EXPECT_EQ(bytes.front(), 0x48);
    EXPECT_EQ(bytes.back(), 0x00);
    EXPECT_EQ(bytes[1], 0xBE);
}

TEST(ParseBytes, AcceptsCommasAndHexPrefix)
{
    EXPECT_EQ(ParseBytes("0x48, 0xBE, 0xE8"), ParseBytes("48 BE E8"));
}

TEST(Cave, TakesLengthFromTheBytesItself)
{
    const PatchSpec s = Cave("29 93 ?? ??", "48 BE E8 03");

    EXPECT_EQ(s.kind, PatchSpec::Kind::Cave);
    EXPECT_EQ(s.patchBytes.size(), 4u);
    EXPECT_EQ(s.length, 4u); // длину руками больше не пишем
}

// --- Адреса ---

TEST(Address, ModuleChainKeepsOrderAndIsNotAbsolute)
{
    // база + 0x346C10 -> разыменовать -> + 0x800
    const Address a = Address::Module(0x00346C10).Deref(0x800);

    EXPECT_FALSE(a.absolute);
    EXPECT_EQ(a.offsets, (std::vector<std::uintptr_t>{ 0x00346C10, 0x800 }));
}

TEST(Address, MultiLevelChain)
{
    const Address a = Address::Module(0x10).Deref(0x18).Deref(0x800);
    EXPECT_EQ(a.offsets, (std::vector<std::uintptr_t>{ 0x10, 0x18, 0x800 }));
}

TEST(Address, AbsoluteIsMarked)
{
    const Address a = Address::At(0x015F45D0);

    EXPECT_TRUE(a.absolute);
    EXPECT_EQ(a.offsets, (std::vector<std::uintptr_t>{ 0x015F45D0 }));
}

TEST(WriteValue, CarriesChainAndAbsoluteFlagFromAddress)
{
    const PatchSpec chain = WriteValue(Address::Module(0x346C10).Deref(0x800), 9999);
    EXPECT_FALSE(chain.absoluteAddress);
    EXPECT_EQ(chain.offsets.size(), 2u);

    const PatchSpec direct = WriteValue(Address::At(0x1000), 1);
    EXPECT_TRUE(direct.absoluteAddress);
}
