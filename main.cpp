#include "main.h"
#include "Cheat.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	Cheat* cheat = new Cheat(L"Tutorial-i386.exe"); //Процесс атакуемой игры

	std::vector<int> option1keys = { 0x61 };
	CheatOption* option1 = new CheatOption(NULL, L"[Numpad 1] First  Function(Nop)", option1keys);
	option1->AddNopPatch(L"D9 9E C0 04 00 00", 6);

	std::vector<int> option2keys = { 0x60 };
	CheatOption* option2 = new CheatOption(NULL, L"[Numpad 0] Test  Function(Cave)", option2keys);
	BYTE bytes[] = { 0x50, 0xB8, 0x00, 0x40, 0x9C, 0x45, 0x66, 0x0F, 0x6E, 0xC8, 0x58 };
	option2->AddCavePatch(L"D9 9E C0 04 00 00", bytes, 6);

	cheat->AddCheatOption(option1);
	cheat->AddCheatOption(option2);

	cheat->Start();
	BaseRender* renderer = new SimpleRenderer(cheat, WindowTitle, W_WIDTH, W_HEIGHT);
	renderer->Start();
	cheat->Stop();

	delete option1;
	delete cheat;

	return 0;
}