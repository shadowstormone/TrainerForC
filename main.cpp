#include "Include.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	Cheat* cheat = new Cheat(L"Tutorial-i386.exe"); //Процесс атакуемой игры

	std::vector<int> option1keys = { VK_CONTROL, 0x4A };
	CheatOption* option1 = new CheatOption(NULL, L"[CTRL+J] First Function(Nop)", option1keys);
	option1->AddNopPatch(L"29 83 B0 04 00 00", 6);

	std::vector<int> option2keys = { VK_CONTROL, 0x4B };
	CheatOption* option2 = new CheatOption(NULL, L"[CTRL+K] Test Function(Cave)", option2keys);
	BYTE bytes[] = { 0xC7, 0x83, 0xB0, 0x04, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00 };
	option2->AddCavePatch(L"29 83 B0 04 00 00", bytes, 10);

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