#pragma once
#include "Cheat.h"

class BaseRender
{
protected:
	HWND _wnd = NULL;
	Cheat* _cheat = NULL;

	static LRESULT BaseWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void CreateBaseWindow(LPCWSTR title, int width, int height);
	void StartMessageCycle();
public:
	BaseRender(Cheat* cheat, LPCWSTR title, int width, int height);
	virtual void Start() { StartMessageCycle(); }
	virtual void Stop() = 0;
};