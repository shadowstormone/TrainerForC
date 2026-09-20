#include "cheats/CheatRegistry.h"

#include <utility>

CheatRegistry& CheatRegistry::Instance()
{
    static CheatRegistry instance;
    return instance;
}

bool CheatRegistry::Register(CheatDefinition def)
{
    Instance()._definitions.push_back(std::move(def));
    return true;
}
