#pragma once
#include <Windows.h>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "cheats/CheatOptionDefinitions.h"
#include "platform/EnumClassHash.h"

class Cheat;
class CheatOption;

// Владеет опциями чита и связывает их с UI-переключателями.
// Реализация — в CheatOptionManager.cpp, поэтому заголовок не тянет
// за собой ImGui, Drawing и Utils.
class CheatOptionManager
{
    Cheat* _cheatProcess;
    std::vector<CheatOption*> _orderedOptions; // для UI — сырые указатели в порядке AllOptions
    std::unordered_map<CheatOptionDefinitions::OptionID, std::unique_ptr<CheatOption>, EnumClassHash> _optionsOwner;
    std::unordered_map<std::string, std::function<void(bool, DWORD)>> _toggleHandlers;

    void RegisterToggleHandler(CheatOptionDefinitions::OptionID id, CheatOption* option);

public:
    explicit CheatOptionManager(Cheat* cheatProcess);

    // Объявлен здесь, определён в .cpp: unique_ptr<CheatOption> требует
    // полного типа в точке уничтожения.
    ~CheatOptionManager();

    CheatOptionManager(const CheatOptionManager&) = delete;
    CheatOptionManager& operator=(const CheatOptionManager&) = delete;

    // Добавление опции (manager принимает владение)
    bool AddOption(CheatOptionDefinitions::OptionID id, std::unique_ptr<CheatOption> option);

    // Все опции (сырые указатели) в порядке AllOptions
    std::vector<CheatOption*> GetAllOptions() const;

    // Обработка переключения (вызывается Drawing)
    void HandleToggle(const std::string& toggleId, const std::string& optionName, bool currentState, bool previousState);

    // Получение опции по ID
    CheatOption* GetOption(CheatOptionDefinitions::OptionID id) const;
};
