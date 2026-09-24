#pragma once
#include <cstddef>
#include <vector>

#include "cheats/CheatDefinition.h"

// Реестр читов. Определения складываются сюда статическими инициализаторами
// (макрос REGISTER_CHEAT) ещё до main(), поэтому чтобы добавить чит,
// достаточно дописать одну запись в любой .cpp в cheats/registry/.
//
// ВАЖНО о порядке: внутри ОДНОГО .cpp читы регистрируются сверху вниз —
// этот порядок гарантирован и определяет порядок в UI. А вот порядок
// инициализации МЕЖДУ разными .cpp стандартом не определён. Поэтому все
// читы держим в одном файле (cheats/registry/GameCheats.cpp); если файлов
// станет несколько, порядок в UI надо будет задавать явно (полем сортировки),
// а не полагаться на порядок линковки.
class CheatRegistry
{
    std::vector<CheatDefinition> _definitions;

    CheatRegistry() = default;

public:
    CheatRegistry(const CheatRegistry&) = delete;
    CheatRegistry& operator=(const CheatRegistry&) = delete;

    // Meyers-синглтон: безопасен во время статической инициализации.
    static CheatRegistry& Instance();

    // Возвращает bool, чтобы результат можно было присвоить статической
    // переменной внутри REGISTER_CHEAT.
    static bool Register(CheatDefinition def);

    const std::vector<CheatDefinition>& All() const { return _definitions; }
    std::size_t Size() const { return _definitions.size(); }
};

#define CHEAT_REGISTRY_CONCAT_INNER(a, b) a##b
#define CHEAT_REGISTRY_CONCAT(a, b) CHEAT_REGISTRY_CONCAT_INNER(a, b)

// Добавление чита одной записью, поля — по именам:
//
//   REGISTER_CHEAT({
//       .name    = L"Set 9999 HP",
//       .keys    = { VKeys::KEY_NUMPAD2 },
//       .patches = { WriteValue(Address::Module(0x240600).Deref(0x4B4), 9999) },
//   })
//
// __VA_ARGS__ нужен из-за запятых внутри фигурных скобок.
#define REGISTER_CHEAT(...)                                                        \
    namespace                                                                      \
    {                                                                              \
        const bool CHEAT_REGISTRY_CONCAT(g_cheatRegistered_, __LINE__) =           \
            CheatRegistry::Register(__VA_ARGS__);                                  \
    }
