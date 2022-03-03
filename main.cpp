#include "main.h"
#include "Cheat.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	Cheat* cheat = new Cheat(L"bf3.exe"); //Процесс атакуемой игры

	std::vector<int> option1keys = { 0x61 };
	CheatOption* option1 = new CheatOption(NULL, L"[Numpad 1] First  Function(Nop)", option1keys);
	option1->AddNopPatch(L"0xFF, 0x48, 0x68", 3);

	std::vector<int> option2keys = { 0x60 };
	CheatOption* option2 = new CheatOption(NULL, L"[Numpad 0] Test  Function(Cave)", option2keys);
	BYTE bytes[] = { 0xBA, 0x63, 0x00, 0x00, 0x00, 0x89, 0x50, 0x68 };
	option2->AddCavePatch(L"0xFF, 0x48, 0x68", bytes, 8);

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