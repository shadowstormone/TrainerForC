#pragma once
#include <unordered_map>
#include <functional>
#include <algorithm>
#include <vector>
#include <memory>
#include "Cheat.h"
#include "CheatOptionDefinitions.h"
#include "ImGuiConsole.h"
#include "Drawing.h"
#include "EnumClassHash.h"
#include "Utils.h" // для WStringToUtf8

Cheat* _cheatProcGame = nullptr;

class CheatOptionManager
{
private:
    Cheat* _cheatProcess;
    std::vector<CheatOption*> _orderedOptions; // для UI — сырые указатели в порядке AllOptions
    std::unordered_map<CheatOptionDefinitions::OptionID, std::unique_ptr<CheatOption>, EnumClassHash> _optionsOwner;
    std::unordered_map<std::string, std::function<void(bool, DWORD)>> _toggleHandlers;
    bool _initialized;

public:
    explicit CheatOptionManager(Cheat* cheatProcess)
        : _cheatProcess(cheatProcess), _initialized(false)
    {
        _orderedOptions.resize(CheatOptionDefinitions::AllOptions.size(), nullptr);
    }

    ~CheatOptionManager() = default; // unique_ptr сам удалит опции

    // Добавление опции (manager принимает владение)
    bool AddOption(CheatOptionDefinitions::OptionID id, std::unique_ptr<CheatOption> option) {
        if (!option) return false;

        int index = -1;
        for (size_t i = 0; i < CheatOptionDefinitions::AllOptions.size(); ++i) {
            if (CheatOptionDefinitions::AllOptions[i].id == id) {
                index = static_cast<int>(i);
                break;
            }
        }
        if (index == -1) {
            gConsole->addLog("ERROR", "Опция с ID " + std::to_string(static_cast<int>(id)) + " не найдена в AllOptions");
            return false;
        }

        CheatOption* raw = option.get();

        // Сохраняем владение
        _optionsOwner.emplace(id, std::move(option));

        // Обновляем порядок
        _orderedOptions[index] = raw;

        // Регистрируем опцию в процессе (Process не владеет и не удаляет)
        if (_cheatProcess) {
            _cheatProcess->AddCheatOption(raw);
        }

        // Регистрируем обработчик переключения
        RegisterToggleHandler(id, raw);

        return true;
    }

private:
    void RegisterToggleHandler(CheatOptionDefinitions::OptionID id, CheatOption* option) {
        const auto& definition = CheatOptionDefinitions::GetOptionById(id);
        std::string optionName = Utils::WStringToUtf8(definition.name);

        _toggleHandlers[optionName] = [this, id, definition, option](bool enabled, DWORD processId)
            {
                if (enabled) {
                    gConsole->addLog("INFO", "Переключатель " + Utils::WStringToUtf8(definition.name) + " активирован");
                    option->Enable(processId);
                    option->IsEnabled(true);

                    if (definition.autoDisable) {
                        std::string toggleId = "##toggle_" + Utils::WStringToUtf8(definition.name);
                        Utils::DelayedToggleOff(
                            Drawing::GetToggleStates(),
                            toggleId,
                            definition.autoDisableDelay,
                            [this, definition, toggleId, option, processId]()
                            {
                                option->Disable(processId);
                                option->IsEnabled(false);

                                auto& toggleStates = Drawing::GetToggleStates();
                                if (toggleStates.find(toggleId) != toggleStates.end()) {
                                    toggleStates[toggleId] = false;
                                }
                                gConsole->addLog("INFO", "Опция " + Utils::WStringToUtf8(definition.name) + " была временной и выключена автоматически");
                            });
                    }
                }
                else {
                    gConsole->addLog("INFO", "Опция " + Utils::WStringToUtf8(definition.name) + " выключена");
                    option->Disable(processId);
                    option->IsEnabled(false);
                }
            };
    }

public:
    // Получаем все опции (сырые указатели) в порядке AllOptions
    std::vector<CheatOption*> GetAllOptions() const {
        std::vector<CheatOption*> result;
        for (auto* option : _orderedOptions) {
            if (option != nullptr) result.push_back(option);
        }
        return result;
    }

    // Обработка переключения (вызывается Drawing)
    void HandleToggle(const std::string& toggleId, const std::string& optionName, bool currentState, bool previousState) {
        if (currentState == previousState) return;
        auto it = _toggleHandlers.find(optionName);
        if (it != _toggleHandlers.end()) {
            it->second(currentState, _cheatProcess->GetProcessID());
        }
    }

    // Получение опции по ID
    CheatOption* GetOption(CheatOptionDefinitions::OptionID id) const {
        auto it = _optionsOwner.find(id);
        if (it == _optionsOwner.end()) return nullptr;
        return it->second.get();
    }
};