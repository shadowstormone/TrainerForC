#include "app/main.h"
#include "ui/UI.h"
#include "cheats/CheatOptionManager.h"
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

    // Устанавливаем обработчик для Drawing
    Drawing::SetToggleHandler([&optionManager](const std::string& toggleId, const std::string& optionName, bool currentState, bool previousState)
        {
            optionManager.HandleToggle(toggleId, optionName, currentState, previousState);
        });

    // Запуск программы
    ProcessAttackGame->Start();
    Drawing::Initialize(ProcessAttackGame.get(), offsets, optionManager.GetAllOptions());
	UI::Render();
    ProcessAttackGame->Stop();

    ProcessAttackGame->DisableAllFunctionMem();

    return 0;
}