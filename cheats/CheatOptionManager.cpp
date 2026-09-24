#include "cheats/CheatOptionManager.h"

#include <exception>

#include "cheats/CheatFactory.h"
#include "cheats/CheatOption.h"
#include "cheats/CheatRegistry.h"
#include "core/Cheat.h"
#include "platform/AudioService.h"
#include "platform/Logger.h"
#include "platform/Utils.h"

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
        auto option = CreateCheatFromDefinition(definition);
        if (!option)
        {
            Log::Error("Не удалось создать опцию: " + Utils::WStringToUtf8(definition.name));
            continue;
        }

        // Регистрируем опцию в процессе (Cheat не владеет и не удаляет)
        if (_cheatProcess) _cheatProcess->AddCheatOption(option.get());

        _options.push_back(std::move(option));
    }
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

void CheatOptionManager::SetEnabled(CheatOption* option, bool enabled)
{
    if (!option) return;

    if (!_cheatProcess)
    {
        option->SetEnabled(enabled, 0);
        return;
    }

    // Само включение — в фоновом потоке, окно тем временем рисует индикатор.
    option->BeginPending();
    _cheatProcess->Post([option, enabled, process = _cheatProcess]()
    {
        option->SetEnabled(enabled, process->GetProcessID());
        option->EndPending();
    });
}

CheatOption* CheatOptionManager::GetOption(std::size_t index) const
{
    if (index >= _options.size()) return nullptr;
    return _options[index].get();
}

int CheatOptionManager::DisableAll()
{
    if (!_cheatProcess) return 0;

    const DWORD processId = _cheatProcess->GetProcessID();
    if (processId == 0) return 0;

    // Откат на выходе — это не пользовательское действие, а уборка.
    // Без этого на закрытии играла бы очередь щелчков: по одному на
    // каждый включённый чит.
    struct MuteWhileRestoring
    {
        bool wasEnabled;

        MuteWhileRestoring() : wasEnabled(AudioService::Instance().IsEnabled())
        {
            AudioService::Instance().SetEnabled(false);
        }

        ~MuteWhileRestoring() { AudioService::Instance().SetEnabled(wasEnabled); }
    } mute;

    int restored = 0;

    for (const std::unique_ptr<CheatOption>& option : _options)
    {
        if (!option || !option->IsEnabled() || option->IsOneShot()) continue;

        // Ошибка отката одной опции не должна мешать откатить остальные:
        // выйти, оставив игру частично пропатченной, хуже всего.
        try
        {
            if (option->SetEnabled(false, processId)) ++restored;
        }
        catch (const std::exception& e)
        {
            Log::Error(std::string("Не удалось откатить опцию: ") + e.what());
        }
    }

    return restored;
}
