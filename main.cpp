#include "main.h"
#include "Cheat.h"

HWND mainWnd;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	Cheat* cheat = new Cheat(L"iw3sp.exe"); //Процесс атакуемой игры

	/*std::vector<int> option1keys = { 0x61 };
	CheatOption* option1 = new CheatOption(NULL, L"[Numpad 1] First  Function(Nop)", option1keys);
	option1->AddNopPatch(L"0xFF, 0x48, 0x68", 3);*/

	std::vector<int> optionkeys = { 0x60 };
	CheatOption* option = new CheatOption(NULL, L"[Numpad 0] Rapid Fire(Cave)", optionkeys);
	BYTE bytes[] = { 0xB9, 0x00, 0x00, 0x00, 0x00 };
	option->AddCavePatch(L"0x89, 0x4E, 0x3C, 0x83, 0xBF, 0x04, 0x05, 0x00, 0x00, 0x00", bytes, 5);

	std::vector<int> optionkeys1 = { 0x61 };
	CheatOption* option1 = new CheatOption(NULL, L"[Numpad 1] Infinite Ammo", optionkeys1);
	BYTE bytes1[] = { 0xB8, 0x64, 0x00, 0x00, 0x00 };
	option->AddCavePatch(L"0x89, 0x84, 0x8F, 0x34, 0x03, 0x00, 0x00", bytes1, 5);

	std::vector<int> optionkeys2 = { 0x62 };
	CheatOption* option2 = new CheatOption(NULL, L"[Numpad 2] Maximum Accuracy", optionkeys2);
	option2->AddNopPatch(L"0xD9, 0x91, 0x24, 0x06, 0x00, 0x00", 6);


	cheat->AddCheatOption(option);
	cheat->AddCheatOption(option1);
	cheat->AddCheatOption(option2);

	cheat->Start();
	BaseRender* renderer = new SimpleRenderer(cheat, WindowTitle, W_WIDTH, W_HEIGHT);
	renderer->Start();
	cheat->Stop();

	delete option;
	delete option1;
	delete option2;
	delete cheat;

	return 0;
}