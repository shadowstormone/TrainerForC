#include "Include.h"

LRESULT BaseRender::BaseWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_DESTROY:
		PostQuitMessage(S_OK);
		break;

	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return 0;
}

void BaseRender::CreateBaseWindow(LPCWSTR title, int width, int height)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);
	WNDCLASSEX wcx = { 0 };
	HWND mainWnd = NULL;

	wcx.cbSize = sizeof wcx;
	wcx.hInstance = hInstance;
	wcx.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcx.style = CS_HREDRAW | CS_VREDRAW;
	wcx.hbrBackground = NULL;
	wcx.hIcon = LoadIcon(NULL, MAKEINTRESOURCE(IDI_APPLICATION));
	wcx.lpfnWndProc = BaseWindowProc;
	wcx.lpszClassName = L"Trainer By ShadowStorm";

	RegisterClassEx(&wcx);

	_wnd = CreateWindowEx(
		WS_EX_APPWINDOW,
		wcx.lpszClassName,
		title,
		WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_SIZEBOX & ~WS_CAPTION & 
		~WS_THICKFRAME & ~WS_EX_DLGMODALFRAME & ~WS_EX_CLIENTEDGE & ~WS_EX_STATICEDGE,
		(GetSystemMetrics(SM_CXSCREEN) - width) / 2, // Длина
		(GetSystemMetrics(SM_CXSCREEN) - height) / 5, //Высота
		width,
		height,
		NULL,
		NULL,
		hInstance,
		NULL);

	ShowWindow(_wnd, SW_SHOWDEFAULT);
	UpdateWindow(_wnd);
	SetForegroundWindow(_wnd);
}

void BaseRender::StartMessageCycle()
{
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

BaseRender::BaseRender(Cheat* cheat, LPCWSTR title, int width, int height) : _cheat(cheat)
{
	CreateBaseWindow(title, width, height);
}