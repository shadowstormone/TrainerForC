#include "cheats/CheatOptionManager.h"

#include "cheats/CheatDefinition.h"
#include "cheats/CheatFactory.h"
#include "cheats/CheatOption.h"
#include "cheats/CheatRegistry.h"
#include "core/Cheat.h"
#include "platform/Utils.h" // WStringToUtf8, RunAfter
#include "ui/ImGuiConsole.h"

CheatOptionManager::CheatOptionManager(Cheat* cheatProcess)
    : _cheatProcess(cheatProcess)
{
}

CheatOptionManager::~CheatOptionManager() = default; // unique_ptr сам удалит опции

void CheatOptionManager::LoadFromRegistry()
{
    const auto& definitions = CheatRegistry::Instance().All();
    _options.reserve(definitions.size());

    for (const auto& definition : definitions)
    {
        auto option = CreateCheatFromDefinition(definition, _cheatProcess);
        if (!option)
        {
            gConsole->addLog("ERROR", "Не удалось создать опцию: " + Utils::WStringToUtf8(definition.name));
            continue;
        }

        CheatOption* raw = option.get();
        _options.push_back(std::move(option));

        // Регистрируем опцию в процессе (Cheat не владеет и не удаляет)
        if (_cheatProcess)
        {
            _cheatProcess->AddCheatOption(raw);
        }

        RegisterToggleHandler(definition, raw);
    }
}

void CheatOptionManager::RegisterToggleHandler(const CheatDefinition& definition, CheatOption* option)
{
    const std::string optionName = Utils::WStringToUtf8(definition.name);

    _toggleHandlers[optionName] = [definition, option](bool enabled, DWORD processId)
        {
            const std::string name = Utils::WStringToUtf8(definition.name);

            if (enabled)
            {
                if (!option->Enable(processId))
                {
                    // Патч не применился — опция остаётся выключенной.
                    // Лезть в UI не нужно: переключатели рисуются по
                    // IsEnabled(), поэтому интерфейс сам покажет верное
                    // состояние на следующем кадре.
                    option->IsEnabled(false);
                    gConsole->addLog("ERROR", "Не удалось применить " + name + " — опция выключена");
                    return;
                }

                gConsole->addLog("INFO", "Переключатель " + name + " активирован");
                option->IsEnabled(true);

                if (definition.autoDisable)
                {
                    Utils::RunAfter(definition.autoDisableDelay,
                        [option, processId, name]()
                        {
                            option->Disable(processId);
                            option->IsEnabled(false);
                            gConsole->addLog("INFO", "Опция " + name + " была временной и выключена автоматически");
                        });
                }
            }
            else
            {
                gConsole->addLog("INFO", "Опция " + name + " выключена");
                option->Disable(processId);
                option->IsEnabled(false);
            }
        };
}

std::vector<CheatOption*> CheatOptionManager::GetAllOptions() const
{
    std::vector<CheatOption*> result;
    result.reserve(_options.size());
    for (const auto& option : _options)
    {
        if (option) result.push_back(option.get());
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

CheatOption* CheatOptionManager::GetOption(std::size_t index) const
{
    if (index >= _options.size()) return nullptr;
    return _options[index].get();
}
