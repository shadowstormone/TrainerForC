#include <gtest/gtest.h>

#include <cstdint>
#include <variant>
#include <vector>

#include "cheats/CheatRegistry.h"
#include "platform/VKeys.h"

// Реестр наполняется макросами REGISTER_CHEAT в cheats/registry/*.cpp
// через статическую инициализацию — до main(). Эти тесты проверяют,
// что механизм действительно отработал, а не просто скомпилировался.

TEST(CheatRegistry, PopulatedByStaticInit)
{
    EXPECT_EQ(CheatRegistry::Instance().All().size(), 4u);
}

TEST(CheatRegistry, CheatsHaveExpectedNamesAndKeys)
{
    const auto& all = CheatRegistry::Instance().All();
    ASSERT_EQ(all.size(), 4u);

    EXPECT_EQ(all[0].name, L"[Numpad 1] - Cheat Test 1");
    EXPECT_EQ(all[1].name, L"[Numpad 2] - Set 9999 HP");
    EXPECT_EQ(all[2].name, L"[Numpad 3] - Cheat Test 3");
    EXPECT_EQ(all[3].name, L"[Numpad 4] - First Function(Nop)");

    ASSERT_EQ(all[0].keys.size(), 1u);
    EXPECT_EQ(all[0].keys[0], VKeys::KEY_NUMPAD1);
    EXPECT_EQ(all[1].keys[0], VKeys::KEY_NUMPAD2);
    EXPECT_EQ(all[2].keys[0], VKeys::KEY_NUMPAD3);
    EXPECT_EQ(all[3].keys[0], VKeys::KEY_NUMPAD4);
}

TEST(CheatRegistry, CavePatchSpecCarriesStaticBytes)
{
    const auto& cheat = CheatRegistry::Instance().All().at(0);
    ASSERT_EQ(cheat.patches.size(), 1u);

    EXPECT_EQ(cheat.patches[0].kind, PatchSpec::Kind::Cave);
    EXPECT_NE(cheat.patches[0].patchData, nullptr);
    EXPECT_EQ(cheat.patches[0].length, 10u);
    EXPECT_FALSE(cheat.patches[0].signature.empty());
}

TEST(CheatRegistry, WriteValueSpecKeepsOffsetsValueAndAutoDisable)
{
    const auto& cheat = CheatRegistry::Instance().All().at(1);
    ASSERT_EQ(cheat.patches.size(), 1u);

    const auto& spec = cheat.patches[0];
    EXPECT_EQ(spec.kind, PatchSpec::Kind::WriteValue);
    EXPECT_EQ(spec.offsets, (std::vector<std::uintptr_t>{ 0x00240600, 0x4B4 }));
    ASSERT_TRUE(std::holds_alternative<int>(spec.value));
    EXPECT_EQ(std::get<int>(spec.value), 9999);

    EXPECT_TRUE(cheat.autoDisable);
    EXPECT_EQ(cheat.autoDisableDelay, 450);
}

TEST(CheatRegistry, NopSpecKeepsLength)
{
    const auto& cheat = CheatRegistry::Instance().All().at(3);
    ASSERT_EQ(cheat.patches.size(), 1u);

    EXPECT_EQ(cheat.patches[0].kind, PatchSpec::Kind::Nop);
    EXPECT_EQ(cheat.patches[0].length, 6u);
    EXPECT_FALSE(cheat.autoDisable);
}
