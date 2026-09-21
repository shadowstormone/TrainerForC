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

    // Подписи читов — содержимое реестра, а не его механизм: сверять их
    // дословно значит ломать тест при каждом переименовании. Проверяем,
    // что имя вообще проставилось.
    for (const auto& cheat : all)
    {
        EXPECT_FALSE(cheat.name.empty());
        EXPECT_FALSE(cheat.patches.empty());
    }

    ASSERT_EQ(all[0].keys.size(), 1u);
    EXPECT_EQ(all[0].keys[0], VKeys::KEY_NUMPAD1);
    EXPECT_EQ(all[1].keys[0], VKeys::KEY_NUMPAD2);
    EXPECT_EQ(all[2].keys[0], VKeys::KEY_NUMPAD3);
    EXPECT_EQ(all[3].keys[0], VKeys::KEY_NUMPAD4);
}

TEST(CheatRegistry, CavePatchSpecCarriesBytes)
{
    const auto& cheat = CheatRegistry::Instance().All().at(0);
    ASSERT_EQ(cheat.patches.size(), 1u);

    EXPECT_EQ(cheat.patches[0].kind, PatchSpec::Kind::Cave);
    // Длина берётся из самих байт, руками её никто не пишет.
    EXPECT_FALSE(cheat.patches[0].patchBytes.empty());
    EXPECT_EQ(cheat.patches[0].length, cheat.patches[0].patchBytes.size());
    EXPECT_FALSE(cheat.patches[0].signature.empty());
}

TEST(CheatRegistry, WriteValueSpecKeepsOffsetsValueAndAutoDisable)
{
    const auto& cheat = CheatRegistry::Instance().All().at(1);
    ASSERT_EQ(cheat.patches.size(), 1u);

    const auto& spec = cheat.patches[0];
    EXPECT_EQ(spec.kind, PatchSpec::Kind::WriteValue);
    // Цепочка указателей: база модуля + 0x346C10 -> разыменовать -> + 0x800
    EXPECT_FALSE(spec.absoluteAddress);
    EXPECT_EQ(spec.offsets, (std::vector<std::uintptr_t>{ 0x00346C10, 0x800 }));
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
