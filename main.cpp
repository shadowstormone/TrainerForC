#include "main.h"
#include "UI.h"
#include "CheatOptionDefinitions.h"
#include "CheatOptionManager.h"

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
	CheatOptionManager optionManager(ProcessAttackGame, &console);

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

/*
#include "main.h"
#include "UI.h"
#include "VKeys.h"

LPCWSTR WindowTitle = L"Test Trainer (+1)"; // Определение здесь
HWND mainWnd;

std::unordered_map<std::string, FunctionOffset> offsets = {
	{"Set HP", {"Set HP", {0x00240600, 0x4B4}}}
};

struct NameFunc
{
	static constexpr LPCWSTR
		Name1 = L"[Numpad 1] - Cheat Test",
		Name2 = L"[Numpad 2] - Set 9999 HP",
		Name3 = L"[Numpad 3] - Cheat Test 3",
		Name4 = L"[Numpad 4] - First Function(Nop)";
};

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	std::vector<CheatOption*> VecCheatOptions;

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

#ifdef _DEBUG
	if (__argc > 1 && wcscmp(__wargv[1], L"--run-tests") == 0) // Проверяем аргументы командной строки на наличие флагов тестов
	{
		RunTests();
		return 0;
	}
#endif // _DEBUG

	Cheat* ProcessAttackGame = new Cheat(L"Tutorial-i386.exe"); //Процесс атакуемой игры

	std::vector<int> GoodModeKey = { VKeys::KEY_NUMPAD1 };
	CheatOption* GoodModeOption = new CheatOption(NULL, NameFunc::Name1, GoodModeKey);
	BYTE patchBytes[] = { 0xC7, 0x83, 0xB4, 0x04, 0x00, 0x00, 0xE8, 0x03, 0x00, 0x00, };
	GoodModeOption->AddCavePatch(L"0x29, 0x83, 0xB4, 0x04, 0x00, 0x00", patchBytes, 10); //Байты оригинальной инструкции в памяти

	std::vector<int> GoodModeKey3 = { VKeys::KEY_NUMPAD3 };
	CheatOption* GoodModeOption3 = new CheatOption(NULL, NameFunc::Name3, GoodModeKey3);
	BYTE patchBytes3[] = { 0xC7, 0x83, 0xB4, 0x04, 0x00, 0x00, 0xE8, 0x03, 0x00, 0x00, };
	GoodModeOption3->AddCavePatch(L"0x29, 0x83, 0xB4, 0x04, 0x00, 0x00", patchBytes3, 10); //Байты оригинальной инструкции в памяти

	std::vector<int> nopKeyFn = { VKeys::KEY_NUMPAD4 };
	CheatOption* option1 = new CheatOption(NULL, NameFunc::Name4, nopKeyFn);
	option1->AddNopPatch(L"0x29, 0x83, 0xB4, 0x04, 0x00, 0x00", 6);

	std::vector<int> VriteKey = { VKeys::KEY_NUMPAD2 };
	std::vector<uintptr_t> offset = { 0x00240600, 0x4B4 };
	CheatOption* addr1 = new CheatOption(NULL, NameFunc::Name2, VriteKey);
	addr1->AddWriteValuePatch(ProcessAttackGame, offset, 9999);


	ProcessAttackGame->AddCheatOption(GoodModeOption);
	ProcessAttackGame->AddCheatOption(addr1);
	ProcessAttackGame->AddCheatOption(GoodModeOption3);
	ProcessAttackGame->AddCheatOption(option1);

	// Добавляем экземпляры в вектор
	VecCheatOptions.push_back(GoodModeOption);
	VecCheatOptions.push_back(addr1);
	VecCheatOptions.push_back(GoodModeOption3);
	VecCheatOptions.push_back(option1);

#ifdef _DEBUG
	ProcessAttackGame->ImGuiOpenConsole();
#endif // _DEBUG

	ProcessAttackGame->Start();
	Drawing::Initialize(ProcessAttackGame, offsets, VecCheatOptions);
	UI::Render();
	ProcessAttackGame->Stop();
	ProcessAttackGame->DisbleAllFunctionMem();

	// Освобождение памяти
	delete GoodModeOption;
	delete GoodModeOption3;
	delete option1;
	delete ProcessAttackGame;
	delete addr1;

	return 0;
}*/

/*
	//Cheat* cheat = new Cheat(L"Tutorial-x86_64.exe"); //Процесс атакуемой игры

	std::vector<int> option1keys = { 0x61 };
	CheatOption* option1 = new CheatOption(NULL, L"[Numpad 1] First  Function(Nop)", option1keys);
	option1->AddNopPatch(L"0xFF, 0x48, 0x68", 3);

	std::vector<int> optionkeys1 = { 0x60 };
	CheatOption* option1 = new CheatOption(NULL, L"[Numpad 0] - Infinite Points LS", optionkeys1);
	BYTE bytes1[] = { 0x81, 0xBF, 0xEC, 0x02, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x75, 0x14, 0x0F, 0x1F, 0x40, 0x00, 0x51, 0xB9, 0x00, 0x80, 0x3B, 0x45, 0x66, 0x0F, 0x6E, 0xC1, 0x59, 0xEB, 0x0E, 0x0F, 0x1F, 0x00, 0x51, 0xB9, 0x00, 0x00, 0x00, 0x00, 0x66, 0x0F, 0x6E, 0xC1, 0x59 }; // Патч байты
	option1->AddCavePatch(L"0xF3, 0x0F, 0x11, 0x87, 0xD8, 0x02, 0x00, 0x00, 0x5B", bytes1, 43); //оригинальные байты

	BYTE HackPatchBytes[] = { 0xC7, 0x83, 0xB4, 0x04, 0x00, 0x00, 0xE8, 0x03, 0x00, 0x00 };
	GoodModeOption->AddCavePatch(L"0x29, 0x83, 0xB4, 0x04, 0x00, 0x00", HackPatchBytes, 10); //Байты оригинальной инструкции в памяти

	cheat->AddCheatOption(option1);
	cheat->AddCheatOption(option2);

	ProcessAttackGame->OpenConsole();
	//BaseRender* renderer = new SimpleRendererV2(ProcessAttackGame, WindowTitle, W_WIDTH, W_HEIGHT);
	//renderer->Start();

	delete renderer;
	delete option1;
	delete option2;
*/