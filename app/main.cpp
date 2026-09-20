#include "app/main.h"
#include "ui/UI.h"
#include "cheats/CheatOptionManager.h"
#include "platform/AudioService.h"
#include "ui/ImGuiConsole.h"

std::unordered_map<std::string, FunctionOffset> offsets = {
	{"Set HP", {"Set HP", {0x00240600, 0x4B4}}},
    {"Set MP", {"Set MP", {0x00240600, 0x4B4}}}
};

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

#ifdef _DEBUG
    if (__argc > 1 && wcscmp(__wargv[1], L"--run-tests") == 0)
    {
        return RunTests();
    }
#endif // _DEBUG

    // Процесс атакуемой игры
    auto ProcessAttackGame = std::make_unique<Cheat>(L"Tutorial-x86_64.exe");

    // Менеджер опций (он будет владеть опциями)
    CheatOptionManager optionManager(ProcessAttackGame.get());

    static Console consoleInstance;
    gConsole = &consoleInstance;

    // Создаём опции из реестра читов (cheats/registry/*.cpp). Manager владеет.
    optionManager.LoadFromRegistry();

#ifdef _DEBUG
    ProcessAttackGame->ImGuiOpenConsole();
#endif // _DEBUG

    // Содержимое главного окна — обычный объект, а не набор статиков
    MainView view;

    // Связываем переключатели UI с менеджером опций
    view.SetToggleHandler([&optionManager](const std::string& toggleId, const std::string& optionName, bool currentState, bool previousState)
        {
            optionManager.HandleToggle(toggleId, optionName, currentState, previousState);
        });

    // Запуск программы
    ProcessAttackGame->Start();
    view.Initialize(ProcessAttackGame.get(), offsets, optionManager.GetAllOptions());
	UI::Render(view);
    ProcessAttackGame->Stop();

    ProcessAttackGame->DisableAllFunctionMem();

    // Гасим звуковой движок явно, а не в деструкторе статика:
    // COM/XAudio2 не любят разрушение на выходе из процесса.
    AudioService::Instance().Shutdown();

    return 0;
}