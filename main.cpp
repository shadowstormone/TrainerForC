#include "main.h"
#include "UI.h"
#include "CheatOptionDefinitions.h"
#include "CheatOptionManager.h"
#include "ImGuiConsole.h"

LPCWSTR WindowTitle = L"Test Trainer (+1)";
HWND mainWnd;

std::unordered_map<std::string, FunctionOffset> offsets = {
	{"Set HP", {"Set HP", {0x00240600, 0x4B4}}}
};

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

#ifdef _DEBUG
	if (__argc > 1 && wcscmp(__wargv[1], L"--run-tests") == 0)
	{
		RunTests();
		return 0;
	}
#endif // _DEBUG

	// Процесс атакуемой игры
	Cheat* ProcessAttackGame = new Cheat(L"Tutorial-i386.exe");

	// Создание менеджера опций
	CheatOptionManager optionManager(ProcessAttackGame);

	static Console consoleInstance;
	gConsole = &consoleInstance;

	// Создание и настройка опций с помощью определений
	for (const auto& def : CheatOptionDefinitions::AllOptions)
	{
		CheatOption* option = new CheatOption(NULL, def.name.c_str(), def.keys);

		// Настройка опции на основе ее ID
		switch (def.id)
		{
		case CheatOptionDefinitions::OptionID::CHEAT_TEST_1:
		{
			BYTE patchBytes[] = { 0xC7, 0x83, 0xB4, 0x04, 0x00, 0x00, 0xE8, 0x03, 0x00, 0x00, };
			option->AddCavePatch(L"0x29, 0x83, 0xB4, 0x04, 0x00, 0x00", patchBytes, 10);
			break;
		}
		case CheatOptionDefinitions::OptionID::SET_HP_9999:
		{
			std::vector<uintptr_t> offset = { 0x00240600, 0x4B4 };
			option->AddWriteValuePatch(ProcessAttackGame, offset, 9999);
			break;
		}
		case CheatOptionDefinitions::OptionID::CHEAT_TEST_3:
		{
			BYTE patchBytes[] = { 0xC7, 0x83, 0xB4, 0x04, 0x00, 0x00, 0xE8, 0x03, 0x00, 0x00, };
			option->AddCavePatch(L"0x29, 0x83, 0xB4, 0x04, 0x00, 0x00", patchBytes, 10);
			break;
		}
		case CheatOptionDefinitions::OptionID::FIRST_FUNCTION_NOP:
		{
			option->AddNopPatch(L"0x29, 0x83, 0xB4, 0x04, 0x00, 0x00", 6);
			break;
		}

		}

		// Регистрация менеджера и процесса атакуемой игры
		optionManager.RegisterOption(def.id, option);
		ProcessAttackGame->AddCheatOption(option);
	}

#ifdef _DEBUG
	ProcessAttackGame->ImGuiOpenConsole();
#endif // _DEBUG

	// Установливем обработчик переключения в классе Drawing
	Drawing::SetToggleHandler([&optionManager](const std::string& toggleId, const std::string& optionName, bool currentState, bool previousState)
		{
			optionManager.HandleToggle(toggleId, optionName, currentState, previousState);
		});

	// Запуск программы
	ProcessAttackGame->Start();
	Drawing::Initialize(ProcessAttackGame, offsets, optionManager.GetAllOptions());
	UI::Render();
	ProcessAttackGame->Stop();
	ProcessAttackGame->DisbleAllFunctionMem();

	// Освобождение памяти - получение всех опций из менеджера и их удаление
	auto allOptions = optionManager.GetAllOptions();
	for (auto* option : allOptions)
	{
		delete option;
	}
	delete ProcessAttackGame;

	return 0;
}