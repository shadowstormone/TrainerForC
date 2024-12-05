#include "main.h"
#include "UI.h"

LPCWSTR WindowTitle = L"Test Trainer (+1)"; // Определение здесь
HWND mainWnd;

struct NameFunc
{
	static constexpr LPCWSTR
		Name1 = L"[Numpad 1] - Cheat Test",
		Name2 = L"[Numpad 2] - Set 9999 HP";
};

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// Проверяем аргументы командной строки на наличие флагов тестов
	if (__argc > 1 && wcscmp(__wargv[1], L"--run-tests") == 0)
	{
		RunTests();
		return 0;
	}

	Cheat* ProcessAttackGame = new Cheat(L"Tutorial-i386.exe"); //Процесс атакуемой игры

	std::vector<int> GoodModeKey = { VKeys::KEY_NUMPAD1 };
	CheatOption* GoodModeOption = new CheatOption(NULL, NameFunc::Name1, GoodModeKey);
	BYTE HackPatchBytes[] = { 0xBE, 0xEB, 0x03, 0x00, 0x00, 0x89, 0xB3, 0xB0, 0x04, 0x00, 0x00, 0x29, 0x83, 0xB0, 0x04, 0x00, 0x00 };
	GoodModeOption->AddCavePatch(L"0x29, 0x83, 0xB0, 0x04, 0x00, 0x00", HackPatchBytes, 17); //Байты оригинальной инструкции в памяти

	std::vector<int> VriteKey = { VKeys::KEY_NUMPAD2 };
	std::vector<uintptr_t> offsets = { 0x00256650, 0x370, 0xAC, 0x4B0 };
	CheatOption* addr1 = new CheatOption(NULL, NameFunc::Name2, VriteKey);
	addr1->AddWriteValuePatch(ProcessAttackGame, offsets, 9999);

	ProcessAttackGame->AddCheatOption(GoodModeOption);
	ProcessAttackGame->AddCheatOption(addr1);

	ProcessAttackGame->Start();

	Drawing::Initialize(ProcessAttackGame);
	UI::Render();
	//ProcessAttackGame->OpenConsole();
	ProcessAttackGame->Stop();

	delete GoodModeOption;
	delete ProcessAttackGame;
	delete addr1;

	return 0;
}

/*
	//Cheat* cheat = new Cheat(L"Tutorial-x86_64.exe"); //Процесс атакуемой игры

	std::vector<int> option1keys = { 0x61 };
	CheatOption* option1 = new CheatOption(NULL, L"[Numpad 1] First  Function(Nop)", option1keys);
	option1->AddNopPatch(L"0xFF, 0x48, 0x68", 3);

	std::vector<int> optionkeys1 = { 0x60 };
	CheatOption* option1 = new CheatOption(NULL, L"[Numpad 0] - Infinite Points LS", optionkeys1);
	BYTE bytes1[] = { 0x81, 0xBF, 0xEC, 0x02, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x75, 0x14, 0x0F, 0x1F, 0x40, 0x00, 0x51, 0xB9, 0x00, 0x80, 0x3B, 0x45, 0x66, 0x0F, 0x6E, 0xC1, 0x59, 0xEB, 0x0E, 0x0F, 0x1F, 0x00, 0x51, 0xB9, 0x00, 0x00, 0x00, 0x00, 0x66, 0x0F, 0x6E, 0xC1, 0x59 }; // Патч байты
	option1->AddCavePatch(L"0xF3, 0x0F, 0x11, 0x87, 0xD8, 0x02, 0x00, 0x00, 0x5B", bytes1, 43); //оригинальные байты

	cheat->AddCheatOption(option1);
	cheat->AddCheatOption(option2);

	ProcessAttackGame->OpenConsole();
	//BaseRender* renderer = new SimpleRendererV2(ProcessAttackGame, WindowTitle, W_WIDTH, W_HEIGHT);
	//renderer->Start();

	delete renderer;
	delete option1;
	delete option2;
*/