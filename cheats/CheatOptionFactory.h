#pragma once
#include "cheats/CheatOptionDefinitions.h"
#include "core/Cheat.h"
#include <memory>

std::unique_ptr<CheatOption> CreateOptionFromDefinition(const CheatOptionDefinitions::OptionDefinition& def, Cheat* game);