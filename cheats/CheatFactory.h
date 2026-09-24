#pragma once
#include <memory>

class CheatOption;
struct CheatDefinition;

// Собирает готовую опцию чита из её описания.
// Добавление нового ТИПА патча = новый класс Patch + одна ветка в CheatFactory.cpp.
std::unique_ptr<CheatOption> CreateCheatFromDefinition(const CheatDefinition& def);
