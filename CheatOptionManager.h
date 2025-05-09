#pragma once
#include <unordered_map>
#include <functional>
#include "Cheat.h"
#include "CheatOptionDefinitions.h"
#include "ImGuiConsole.h"
#include "Drawing.h"
#include <algorithm>

// Класс менеджера для работы с опциями читов
class CheatOptionManager
{
private:
    Cheat * _cheatProcess;
    std::vector<CheatOption*> _orderedOptions;  // Упорядоченный вектор опций (в порядке соответствующем AllOptions)
    std::unordered_map<CheatOptionDefinitions::OptionID, CheatOption*> _optionsMap; // Для быстрого поиска по ID
    std::unordered_map<std::string, std::function<void(bool, DWORD)>> _toggleHandlers;
    Console* _console;
    bool _initialized;

public:
    CheatOptionManager(Cheat* cheatProcess, Console* console)
        : _cheatProcess(cheatProcess), _console(console), _initialized(false)
    {
        // Предварительно выделяем место для всех опций в правильном порядке
        _orderedOptions.resize(CheatOptionDefinitions::AllOptions.size(), nullptr);
    }

    ~CheatOptionManager()
    {
        // Опции будут удалены вызывающей стороной (main.cpp)
    }

    // Инициализация менеджера - вызывается после регистрации всех опций
    void Initialize()
    {
        _initialized = true;

        // Проверяем, что все слоты заполнены
        for (size_t i = 0; i < _orderedOptions.size(); i++)
        {
            if (_orderedOptions[i] == nullptr)
            {
                _console->addLog("ERROR", "Опция не зарегистрирована для индекса " + std::to_string(i));
            }
        }
    }

    // Регистрация опции с использованием конкретного индекса из AllOptions
    void RegisterOption(CheatOptionDefinitions::OptionID id, CheatOption* option)
    {
        // Находим позицию для этой опции в AllOptions
        int index = -1;
        for (size_t i = 0; i < CheatOptionDefinitions::AllOptions.size(); i++)
        {
            if (CheatOptionDefinitions::AllOptions[i].id == id)
            {
                index = static_cast<int>(i);
                break;
            }
        }

        if (index == -1)
        {
            _console->addLog("ERROR", "Опция с ID " + std::to_string(static_cast<int>(id)) + " не найдена в AllOptions");
            return;
        }

        _orderedOptions[index] = option;    // Сохраняем опцию в векторе на правильной позиции
        _optionsMap[id] = option;   // Также сохраняем в map для быстрого доступа по ID

        // Настройка обработчиков переключения на основе идентификатора опции
        const auto& definition = CheatOptionDefinitions::GetOptionById(id);
        std::string optionName = Utils::WStringToUtf8(definition.name);
        _toggleHandlers[optionName] = [this, id, definition, option](bool enabled, DWORD processId)
        {
                // Common logging
                if (enabled)
                {
                    _console->addLog("INFO", "Переключатель " + Utils::WStringToUtf8(definition.name) + " активирован");
                    option->pEnable(processId);
                    option->IsEnabled(true);

                    // Автоматическое отключение, если оно настроено
                    if (definition.autoDisable)
                    {
                        std::string toggleId = "##toggle_" + Utils::WStringToUtf8(definition.name);
                        Utils::DelayedToggleOff(
                            Drawing::GetToggleStates(),
                            toggleId,
                            definition.autoDisableDelay,
                            [this, definition, toggleId, option, processId]()
                            {
                                // Отключаем чит-опцию
                                option->pDisable(processId);
                                option->IsEnabled(false);

                                // Обновляем визуальное состояние переключателя
                                auto& toggleStates = Drawing::GetToggleStates();
                                if (toggleStates.find(toggleId) != toggleStates.end())
                                {
                                    toggleStates[toggleId] = false;
                                }

                                // Логируем действие
                                _console->addLog("INFO", "Опция " + Utils::WStringToUtf8(definition.name) + " была временной и выключена автоматически");
                            });
                    }
                }
                else
                {
                    _console->addLog("INFO", "Опция " + Utils::WStringToUtf8(definition.name) + " выключена");
                    option->pDisable(processId);
                    option->IsEnabled(false);
                }
            };
        }

    // Получаем все зарегистрированные опции в правильном порядке
    std::vector<CheatOption*> GetAllOptions() const
    {
        // Фильтруем nullptr для безопасности
        std::vector<CheatOption*> result;
        for (auto* option : _orderedOptions)
        {
            if (option != nullptr)
            {
                result.push_back(option);
            }
        }
        return result;
    }

    // Обработка взаимодействия при переключении (вызывается из класса Drawing)
    void HandleToggle(const std::string& toggleId, const std::string& optionName, bool currentState, bool previousState)
    {
        // Только обработка изменений состояния
        if (currentState == previousState) return;

        // Поиск и вызов соответствующего обработчика
        auto it = _toggleHandlers.find(optionName);
        if (it != _toggleHandlers.end())
        {
            it->second(currentState, _cheatProcess->GetProcessID());
        }
    }
};