#include "cheats/ValueFieldRegistry.h"

ValueFieldRegistry& ValueFieldRegistry::Instance()
{
    static ValueFieldRegistry registry;
    return registry;
}

bool ValueFieldRegistry::Register(ValueFieldDefinition field)
{
    Instance()._fields.push_back(std::move(field));
    return true;
}
