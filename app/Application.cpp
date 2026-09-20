#include "app/Application.h"

#include "cheats/CheatOptionManager.h"
#include "core/Cheat.h"
#include "platform/AudioService.h"
#include "platform/Logger.h"
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

    _process = std::make_unique<Cheat>(targetProcessName);
    _console->SetProcess(_process.get()); // для команд GetPID/status

    _cheats = std::make_unique<CheatOptionManager>(_process.get());
    _cheats->LoadFromRegistry();

    // Поля ввода значений (кнопка + цепочка оффсетов).
    _offsets = {
        { "Set HP", { "Set HP", { 0x00240600, 0x4B4 } } },
        { "Set MP", { "Set MP", { 0x00240600, 0x4B4 } } },
    };

    _view = std::make_unique<MainView>();
    _view->SetToggleHandler(
        [this](const std::string& toggleId, const std::string& optionName, bool currentState, bool previousState)
        {
            _cheats->HandleToggle(toggleId, optionName, currentState, previousState);
        });
    _view->Initialize(_process.get(), _offsets, _cheats->GetAllOptions());

#ifdef _DEBUG
    _process->ImGuiOpenConsole();
#endif // _DEBUG

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
