#include "main.h"
#include "Cheat.h"

HWND mainWnd;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	Cheat* cheat = new Cheat(L"ff7remake_.exe"); //Процесс атакуемой игры

	/*std::vector<int> option1keys = { 0x61 };
	CheatOption* option1 = new CheatOption(NULL, L"[Numpad 1] First  Function(Nop)", option1keys);
	option1->AddNopPatch(L"0xFF, 0x48, 0x68", 3);*/

	std::vector<int> optionkeys = { 0x60 };
	CheatOption* option = new CheatOption(NULL, L"[Numpad 0] - Good Mode", optionkeys);
	BYTE bytes[] = { 0x8B, 0x7C, 0x08, 0x34 }; // Патч байты
	option->AddCavePatch(L"0x89, 0x7C, 0x01, 0x30, 0x48, 0x8B, 0x5C, 0x24, 0x30", bytes, 4); //оригинальные байты

	std::vector<int> optionkeys1 = { 0x61 };
	CheatOption* option1 = new CheatOption(NULL, L"[Numpad 1] - Infinite Money", optionkeys1);
	BYTE bytes1[] = { 0xBB, 0x9F, 0x86, 0x01, 0x00 }; // Патч байты
	option1->AddCavePatch(L"0x89, 0x5A, 0x0C, 0x48, 0x8B, 0x5C, 0x24, 0x30, 0x48, 0x83", bytes1, 5); //оригинальные байты

	std::vector<int> optionkeys2 = { 0x62 };
	CheatOption* option2 = new CheatOption(NULL, L"[Numpad 2] - Infinite Used Items", optionkeys2);
	BYTE bytes2[] = { 0xB8, 0x63, 0x00, 0x00, 0x00 }; // Патч байты
	option2->AddCavePatch(L"0x89, 0x42, 0x0C, 0xC7, 0x43, 0x08, 0xFF, 0xFF, 0xFF, 0xFF", bytes2, 5); //оригинальные байты

	std::vector<int> optionkeys3 = { 0x63 };
	CheatOption* option3 = new CheatOption(NULL, L"[Numpad 3] - Infinite Mana", optionkeys3);
	BYTE bytes3[] = { 0x8B, 0x7C, 0x08, 0x3C }; // Патч байты
	option3->AddCavePatch(L"0x89, 0x7C, 0x08, 0x38, 0x48, 0x8B, 0x5C, 0x24, 0x30", bytes3, 4); //оригинальные байты

	std::vector<int> optionkeys4 = { 0x64 };
	CheatOption* option4 = new CheatOption(NULL, L"[Numpad 4] - Infinite Limit", optionkeys4);
	BYTE bytes4[] = { 0x53, 0x48, 0xBB, 0x00, 0x00, 0xFA, 0x44, 0x00, 0x00, 0x00, 0x00, 0x66, 0x48, 0x0F, 0x6E, 0xCB, 0x5B, 0xF3, 0x0F, 0x11, 0x4C, 0x08, 0x24 }; // Патч байты
	option4->AddCavePatch(L"0xF3, 0x0F, 0x10, 0x44, 0x08, 0x24", bytes4, 23); //оригинальные байты

	std::vector<int> optionkeys5 = { 0x65 };
	CheatOption* option5 = new CheatOption(NULL, L"[Numpad 5] - Infinite ABS", optionkeys5);
	BYTE bytes5[] = { 0x53, 0x48, 0xBB, 0x00, 0x00, 0xFA, 0x44, 0x00, 0x00, 0x00, 0x00, 0x66, 0x48, 0x0F, 0x6E, 0xF3, 0x5B }; // Патч байты
	option5->AddCavePatch(L"0xF3, 0x0F, 0x11, 0x74, 0x08, 0x44", bytes5, 17); //оригинальные байты

	cheat->AddCheatOption(option);
	cheat->AddCheatOption(option1);
	cheat->AddCheatOption(option2);
	cheat->AddCheatOption(option3);
	cheat->AddCheatOption(option4);
	cheat->AddCheatOption(option5);

	cheat->Start();
	BaseRender* renderer = new SimpleRenderer(cheat, WindowTitle, W_WIDTH, W_HEIGHT);
	renderer->Start();
	cheat->Stop();

	delete option;
	delete option1;
	delete option2;
	delete option3;
	delete option4;
	delete option5;
	delete cheat;

	return 0;
}