#include "cheats/CheatOptionManager.h"

#include "cheats/CheatOption.h"
#include "core/Cheat.h"
#include "platform/Utils.h" // WStringToUtf8, DelayedToggleOff
#include "ui/Drawing.h"
#include "ui/ImGuiConsole.h"

CheatOptionManager::CheatOptionManager(Cheat* cheatProcess)
    : _cheatProcess(cheatProcess)
{
    _orderedOptions.resize(CheatOptionDefinitions::AllOptions.size(), nullptr);
}

CheatOptionManager::~CheatOptionManager() = default; // unique_ptr сам удалит опции

bool CheatOptionManager::AddOption(CheatOptionDefinitions::OptionID id, std::unique_ptr<CheatOption> option)
{
    if (!option) return false;

    int index = -1;
    for (size_t i = 0; i < CheatOptionDefinitions::AllOptions.size(); ++i)
    {
        if (CheatOptionDefinitions::AllOptions[i].id == id)
        {
            index = static_cast<int>(i);
            break;
        }
    }
    if (index == -1)
    {
        gConsole->addLog("ERROR", "Опция с ID " + std::to_string(static_cast<int>(id)) + " не найдена в AllOptions");
        return false;
    }

    CheatOption* raw = option.get();

    // Сохраняем владение
    _optionsOwner.emplace(id, std::move(option));

    // Обновляем порядок
    _orderedOptions[index] = raw;

    // Регистрируем опцию в процессе (Process не владеет и не удаляет)
    if (_cheatProcess)
    {
        _cheatProcess->AddCheatOption(raw);
    }

    // Регистрируем обработчик переключения
    RegisterToggleHandler(id, raw);

    return true;
}

void CheatOptionManager::RegisterToggleHandler(CheatOptionDefinitions::OptionID id, CheatOption* option)
{
    const auto& definition = CheatOptionDefinitions::GetOptionById(id);
    std::string optionName = Utils::WStringToUtf8(definition.name);

    _toggleHandlers[optionName] = [this, definition, option](bool enabled, DWORD processId)
        {
            if (enabled)
            {
                gConsole->addLog("INFO", "Переключатель " + Utils::WStringToUtf8(definition.name) + " активирован");
                option->Enable(processId);
                option->IsEnabled(true);

                if (definition.autoDisable)
                {
                    std::string toggleId = "##toggle_" + Utils::WStringToUtf8(definition.name);
                    Utils::DelayedToggleOff(
                        Drawing::GetToggleStates(),
                        toggleId,
                        definition.autoDisableDelay,
                        [definition, toggleId, option, processId]()
                        {
                            option->Disable(processId);
                            option->IsEnabled(false);

                            auto& toggleStates = Drawing::GetToggleStates();
                            if (toggleStates.find(toggleId) != toggleStates.end())
                            {
                                toggleStates[toggleId] = false;
                            }
                            gConsole->addLog("INFO", "Опция " + Utils::WStringToUtf8(definition.name) + " была временной и выключена автоматически");
                        });
                }
            }
            else
            {
                gConsole->addLog("INFO", "Опция " + Utils::WStringToUtf8(definition.name) + " выключена");
                option->Disable(processId);
                option->IsEnabled(false);
            }
        };
}

std::vector<CheatOption*> CheatOptionManager::GetAllOptions() const
{
    std::vector<CheatOption*> result;
    for (auto* option : _orderedOptions)
    {
        if (option != nullptr) result.push_back(option);
    }
    return result;
}

void CheatOptionManager::HandleToggle(const std::string& toggleId, const std::string& optionName, bool currentState, bool previousState)
{
    if (currentState == previousState) return;
    auto it = _toggleHandlers.find(optionName);
    if (it != _toggleHandlers.end())
    {
        it->second(currentState, _cheatProcess->GetProcessID());
    }
}

CheatOption* CheatOptionManager::GetOption(CheatOptionDefinitions::OptionID id) const
{
    auto it = _optionsOwner.find(id);
    if (it == _optionsOwner.end()) return nullptr;
    return it->second.get();
}
