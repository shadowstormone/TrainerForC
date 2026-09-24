#include "app/Application.h"

#include "cheats/CheatOption.h"
#include "cheats/CheatOptionManager.h"
#include "cheats/ValueFieldRegistry.h"
#include "core/Cheat.h"
#include "platform/AudioService.h"
#include "platform/FileLogger.h"
#include "platform/Logger.h"
#include "platform/Utils.h"

#include <format>
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

#ifdef _DEBUG
    // Второй приёмник — файл рядом с exe. Консоль по умолчанию скрыта,
    // а разбираться приходится как раз тогда, когда что-то не сработало.
    //
    // Только в отладочной сборке: готовому трейнеру незачем оставлять
    // файлы рядом с собой. Обратная сторона — разбирать жалобу на релиз
    // придётся без лога.
    _fileLog = std::make_unique<FileLogger>();
    Log::AddSink(_fileLog.get());
#endif // _DEBUG

    Log::Info("Трейнер запущен");

    _process = std::make_unique<Cheat>(std::wstring(targetProcessName));
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
        [this](CheatOption* option, bool enabled)
        {
            _cheats->SetEnabled(option, enabled);
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

    // Откат — через страж, чтобы он сработал и при выходе по исключению.
    // Порядок важен: сначала останавливается фоновый поток (чтобы горячая
    // клавиша или заморозка не включили что-то заново посреди отката),
    // затем откатываются опции.
    struct RestoreOnExit
    {
        Cheat* process;
        CheatOptionManager* cheats;

        ~RestoreOnExit()
        {
            if (process) process->Stop();
            if (!cheats) return;

            const int restored = cheats->DisableAll();
            if (restored > 0)
            {
                Log::Info(std::format("Откачено опций перед выходом: {}", restored));
            }
        }
    } restoreGuard{ _process.get(), _cheats.get() };

    UI::Render(*_view, *_console);

    return 0;
}
