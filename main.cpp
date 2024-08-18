#include "main.h"
#include "Cheat.h"

LPCWSTR WindowTitle = L"Test Trainer (+1)"; // Определение здесь
HWND mainWnd;

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

	std::vector<int> GoodModeKey = { 0x61 };
	CheatOption* GoodModeOption = new CheatOption(NULL, L"[Numpad 1] - Cheat Test", GoodModeKey);
	BYTE HackPatchBytes[] = { 0xBE, 0xEB, 0x03, 0x00, 0x00, 0x89, 0xB3, 0xB0, 0x04, 0x00, 0x00, 0x29, 0x83, 0xB0, 0x04, 0x00, 0x00 };
	GoodModeOption->AddCavePatch(L"0x29, 0x83, 0xB0, 0x04, 0x00, 0x00", HackPatchBytes, 17); //Байты оригинальной тнструкции в памяти

	std::vector<int> VriteKey = { 0x62 };
	std::vector<uintptr_t> offsets = { 0x00256650,0x370, 0xAC, 0x4B0 };
	CheatOption* addr1 = new CheatOption(NULL, L"[Numpad 2] - Write 9999 for HP", VriteKey);
	addr1->AddWriteValuePatch(L"Tutorial-i386.exe", offsets, 9999);

	ProcessAttackGame->AddCheatOption(GoodModeOption);
	ProcessAttackGame->AddCheatOption(addr1);

	ProcessAttackGame->Start();
	BaseRender* renderer = new SimpleRenderer(ProcessAttackGame, WindowTitle, W_WIDTH, W_HEIGHT);
	renderer->Start();
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

	cheat->OpenConsole();

	delete option1;
	delete option2;
*/