#pragma once
#include <cstddef>
#include <memory>
#include <vector>

class Cheat;
class CheatOption;

// Владеет опциями чита. Порядок опций задаётся CheatRegistry.
class CheatOptionManager
{
    Cheat* _cheatProcess;
    std::vector<std::unique_ptr<CheatOption>> _options; // в порядке реестра

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

    // Переключатель в окне: включить/выключить опцию в текущей игре.
    void SetEnabled(CheatOption* option, bool enabled);

    // Выключает все включённые опции и возвращает память игры как было.
    // Вызывается при закрытии трейнера: оставить игру пропатченной после
    // выхода — худшее, что может сделать трейнер.
    // Возвращает, сколько опций пришлось откатить.
    int DisableAll();

    CheatOption* GetOption(std::size_t index) const;
};
