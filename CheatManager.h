#pragma once
#include <memory>
#include <string>
#include "ICheat.h"
#include <unordered_map>

class CheatManager
{
public:
    void Register(std::shared_ptr<ICheat>);
    void ToggleCheat(const std::string& name);
    void DrawUI();
    void ApplyAll();
    void RevertAll();

private:
    std::unordered_map<std::string, std::shared_ptr<ICheat>> cheats;
};
