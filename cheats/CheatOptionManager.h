#pragma once
#include <Windows.h>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class Cheat;
class CheatOption;
struct CheatDefinition;

// Владеет опциями чита и связывает их с UI-переключателями.
// Порядок опций задаётся CheatRegistry, поэтому отдельные ID не нужны.
class CheatOptionManager
{
    Cheat* _cheatProcess;
    std::vector<std::unique_ptr<CheatOption>> _options; // в порядке реестра
    std::unordered_map<std::string, std::function<void(bool, DWORD)>> _toggleHandlers;

    void RegisterToggleHandler(const CheatDefinition& definition, CheatOption* option);

public:
    explicit CheatOptionManager(Cheat* cheatProcess);

    // Объявлен здесь, определён в .cpp: unique_ptr<CheatOption> требует
    // полного типа в точке уничтожения.
    ~CheatOptionManager();

    CheatOptionManager(const CheatOptionManager&) = delete;
    CheatOptionManager& operator=(const CheatOptionManager&) = delete;

    // Создаёт опции из всех зарегистрированных читов (CheatRegistry).
    void LoadFromRegistry();

    // Все опции (сырые указатели) в порядке реестра
    std::vector<CheatOption*> GetAllOptions() const;

    // Обработка переключения (вызывается Drawing)
    void HandleToggle(const std::string& toggleId, const std::string& optionName, bool currentState, bool previousState);

    CheatOption* GetOption(std::size_t index) const;
};
