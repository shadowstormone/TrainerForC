#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>

#include "cheats/CheatDefinition.h"
#include "core/Relocator.h"
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

// ===================== Перенос инструкций в кейв =====================

TEST(Relocator, StealsWholeInstructionsAndCopiesPlainOnesAsIs)
{
    // Настоящая сигнатура из реестра, disp32 подставлены:
    //   sub [rbx+0x4B4], edx      (6)
    //   mov ecx, [rbx+0x4B4]      (6)
    //   mov r9d, 0xFF             (6)
    //   lea r8, [rbp-0x108]       (7)
    const std::uint8_t code[] = {
        0x29, 0x93, 0xB4, 0x04, 0x00, 0x00,
        0x8B, 0x8B, 0xB4, 0x04, 0x00, 0x00,
        0x41, 0xB9, 0xFF, 0x00, 0x00, 0x00,
        0x4C, 0x8D, 0x85, 0xF8, 0xFE, 0xFF, 0xFF,
    };

    // 14 байт — длинный прыжок x64. Крадём целые инструкции: 6+6+6 = 18.
    const auto r = Relocator::Relocate(0x140001000, code, sizeof(code), true, 0x7FF000000000, 14);

    ASSERT_TRUE(r.ok) << r.error;
    EXPECT_EQ(r.stolen, 18u);            // не 14: инструкции не режутся пополам
    EXPECT_EQ(r.bytes.size(), 18u);

    // Ни RIP-обращений, ни относительных переходов — байты должны совпасть.
    EXPECT_TRUE(std::equal(r.bytes.begin(), r.bytes.end(), code));
}

TEST(Relocator, FixesRipRelativeDisplacement)
{
    // lea rax, [rip+0x10] по адресу 0x1000 -> целится в 0x1017
    const std::uint8_t code[] = { 0x48, 0x8D, 0x05, 0x10, 0x00, 0x00, 0x00 };

    const auto r = Relocator::Relocate(0x1000, code, sizeof(code), true, 0x5000, 5);
    ASSERT_TRUE(r.ok) << r.error;
    ASSERT_EQ(r.bytes.size(), 7u);

    std::int32_t disp = 0;
    std::memcpy(&disp, r.bytes.data() + 3, sizeof(disp));

    // Из кейва до той же цели: 0x1017 - (0x5000 + 7)
    EXPECT_EQ(disp, static_cast<std::int32_t>(0x1017 - (0x5000 + 7)));

    // Прежний длино-дизассемблер оставил бы здесь исходные 0x10
    // и инструкция читала бы чужую память.
    EXPECT_NE(disp, 0x10);
}

TEST(Relocator, FixesNearCallDisplacement)
{
    // call $+5 по адресу 0x1000 -> цель 0x1005
    const std::uint8_t code[] = { 0xE8, 0x00, 0x00, 0x00, 0x00 };

    const auto r = Relocator::Relocate(0x1000, code, sizeof(code), true, 0x5000, 5);
    ASSERT_TRUE(r.ok) << r.error;

    std::int32_t rel = 0;
    std::memcpy(&rel, r.bytes.data() + 1, sizeof(rel));
    EXPECT_EQ(rel, static_cast<std::int32_t>(0x1005 - (0x5000 + 5)));
}

TEST(Relocator, RefusesShortJumpThatCannotReach)
{
    // jmp short +0x10 — адресуется одним байтом, из далёкого кейва не достаёт
    const std::uint8_t code[] = { 0xEB, 0x10, 0x90, 0x90, 0x90, 0x90 };

    const auto r = Relocator::Relocate(0x1000, code, sizeof(code), true, 0x700000000000, 5);

    // Честная ошибка вместо перехода в никуда
    EXPECT_FALSE(r.ok);
    EXPECT_FALSE(r.error.empty());
}

TEST(Relocator, ShortJumpThatStillReachesIsAdjusted)
{
    // Тот же переход, но кейв рядом — уложиться в байт можно
    const std::uint8_t code[] = { 0xEB, 0x10, 0x90, 0x90, 0x90, 0x90 };

    const auto r = Relocator::Relocate(0x1000, code, sizeof(code), true, 0x1040, 2);
    ASSERT_TRUE(r.ok) << r.error;

    const auto rel = static_cast<std::int8_t>(r.bytes[1]);
    EXPECT_EQ(rel, static_cast<std::int8_t>(0x1012 - (0x1040 + 2)));
}

TEST(Relocator, WorksIn32BitMode)
{
    // call $+5 в 32-битном режиме
    const std::uint8_t code[] = { 0xE8, 0x00, 0x00, 0x00, 0x00 };

    const auto r = Relocator::Relocate(0x401000, code, sizeof(code), false, 0x500000, 5);
    ASSERT_TRUE(r.ok) << r.error;

    std::int32_t rel = 0;
    std::memcpy(&rel, r.bytes.data() + 1, sizeof(rel));
    EXPECT_EQ(rel, static_cast<std::int32_t>(0x401005 - (0x500000 + 5)));
}

TEST(Relocator, ReportsGarbageInsteadOfLoopingForever)
{
    // Байты, которые не разбираются. Старый цикл на nmd_x86_ldisasm
    // получал длину 0, offset не двигался — и подвешивал интерфейс.
    const std::uint8_t garbage[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

    const auto r = Relocator::Relocate(0x1000, garbage, sizeof(garbage), true, 0x5000, 14);

    EXPECT_FALSE(r.ok);
    EXPECT_FALSE(r.error.empty());
}

// ===================== Регистры, которые портит патч =====================

TEST(ClobberedRegisters, FindsRsiInRealPatch)
{
    // movabs rsi, 1000 ; mov [rbx+0x800], rsi
    // rsi затирается, а он в Windows x64 ABI callee-saved: игра ждёт его целым.
    const std::uint8_t patch[] = {
        0x48, 0xBE, 0xE8, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x48, 0x89, 0xB3, 0x00, 0x08, 0x00, 0x00,
    };

    const auto regs = Relocator::FindClobberedGpRegisters(patch, sizeof(patch), true);

    ASSERT_EQ(regs.size(), 1u);
    EXPECT_EQ(regs[0], 6u); // rsi
}

TEST(ClobberedRegisters, IgnoresRegistersOnlyRead)
{
    // mov [rbx+0x800], rsi — оба регистра только читаются
    const std::uint8_t patch[] = { 0x48, 0x89, 0xB3, 0x00, 0x08, 0x00, 0x00 };

    const auto regs = Relocator::FindClobberedGpRegisters(patch, sizeof(patch), true);
    EXPECT_TRUE(regs.empty());
}

TEST(ClobberedRegisters, TreatsPartialRegisterAsWholeOne)
{
    // mov eax, 1 портит весь rax, а не только младшие 32 бита
    const std::uint8_t patch[] = { 0xB8, 0x01, 0x00, 0x00, 0x00 };

    const auto regs = Relocator::FindClobberedGpRegisters(patch, sizeof(patch), true);
    ASSERT_EQ(regs.size(), 1u);
    EXPECT_EQ(regs[0], 0u); // rax
}

TEST(ClobberedRegisters, FindsExtendedRegisters)
{
    // mov r9d, 0xFF -> r9 (id 9, для push нужен префикс REX.B)
    const std::uint8_t patch[] = { 0x41, 0xB9, 0xFF, 0x00, 0x00, 0x00 };

    const auto regs = Relocator::FindClobberedGpRegisters(patch, sizeof(patch), true);
    ASSERT_EQ(regs.size(), 1u);
    EXPECT_EQ(regs[0], 9u);
}

TEST(Measure, CountsWholeInstructionsWithoutRelocating)
{
    // sub [rbx+0x4B4], edx (6) + mov ecx,[rbx+0x4B4] (6)
    const std::uint8_t code[] = {
        0x29, 0x93, 0xB4, 0x04, 0x00, 0x00,
        0x8B, 0x8B, 0xB4, 0x04, 0x00, 0x00,
    };

    std::size_t bytes = 0;
    std::string error;

    ASSERT_TRUE(Relocator::Measure(code, sizeof(code), true, 5, bytes, error)) << error;
    EXPECT_EQ(bytes, 6u); // одной инструкции уже хватает на 5-байтный прыжок

    ASSERT_TRUE(Relocator::Measure(code, sizeof(code), true, 7, bytes, error)) << error;
    EXPECT_EQ(bytes, 12u); // семь байт -> нужна и вторая инструкция целиком
}
