#include "app/Application.h"

#include "cheats/CheatOptionManager.h"
#include "cheats/ValueFieldRegistry.h"
#include "core/Cheat.h"
#include "platform/AudioService.h"
#include "platform/FileLogger.h"
#include "platform/Logger.h"
#include "platform/Utils.h"
#include "ui/ImGuiConsole.h"
#include "ui/UI.h"

Application::Application() = default;

Application::~Application()
{
    // Гасим звуковой движок явно, а не в деструкторе статика:
    // COM/XAudio2 не любят разрушение на выходе из процесса.
    AudioService::Instance().Shutdown();

    // Консоль умрёт вместе с этим объектом — снимаем приёмник логов.
    Log::SetSink(nullptr);
}

bool Application::Initialize(const wchar_t* targetProcessName)
{
    // Консоль нужна первой: остальные части уже пишут в неё при создании.
    _console = std::make_unique<Console>();
    Log::SetSink(_console.get());

    // Второй приёмник — файл рядом с exe. Консоль по умолчанию скрыта,
    // а разбираться приходится как раз тогда, когда что-то не сработало.
    _fileLog = std::make_unique<FileLogger>();
    Log::AddSink(_fileLog.get());
    Log::Info("Трейнер запущен");

    _process = std::make_unique<Cheat>(targetProcessName);
    _console->SetProcess(_process.get()); // для команд GetPID/status

    // Команда cheats берёт список отсюда: консоль не знает про менеджер.
    _console->SetCheatLister([this]()
    {
        std::vector<std::pair<std::string, bool>> rows;

        for (CheatOption* option : _cheats->GetAllOptions())
        {
            rows.emplace_back(Utils::WStringToUtf8(option->GetDescription()), option->IsEnabled());
        }

        return rows;
    });

    _cheats = std::make_unique<CheatOptionManager>(_process.get());
    _cheats->LoadFromRegistry();

    _view = std::make_unique<MainView>();
    _view->SetToggleHandler(
        [this](const std::string& toggleId, const std::string& optionName, bool currentState, bool previousState)
        {
            _cheats->HandleToggle(toggleId, optionName, currentState, previousState);
        });
    // Поля ввода описаны в том же реестре, что и читы.
    _view->Initialize(_process.get(),
                      ValueFieldRegistry::Instance().All(),
                      _cheats->GetAllOptions());

    // Консоль больше не открывается сама: она перекрывала всю панель.
    // Вызывается клавишей `

    return true;
}

int Application::Run()
{
    _process->Start();
    UI::Render(*_view, *_console);
    _process->Stop();

    _process->DisableAllFunctionMem();
    return 0;
}
