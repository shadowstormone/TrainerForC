#include "BaseRenderer.h"
#include "main.h"
#include "resource.h"

LRESULT BaseRender::BaseWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		switch (wParam)
		{
		case ButtonClickExit:
			exit(0);
			break;
		}
		break;
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

	wcx.cbSize = sizeof wcx;										// Размер этой структуры, в байтах.
	wcx.hInstance = hInstance;										// Дескриптор экземпляра, который содержит оконную процедуру для класса.
	wcx.hCursor = LoadCursor(NULL, IDC_ARROW);						// Дескриптор курсора класса.
	wcx.style = CS_HREDRAW | CS_VREDRAW;							// Устанавливает стиль(и) класса.
	wcx.hbrBackground = NULL;									    // Дескриптор кисти фона класса.
	wcx.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON2));	// Дескриптор значка класса.
	wcx.lpfnWndProc = BaseWindowProc;								// Указатель на оконную процедуру.
	wcx.lpszClassName = L"Trainer By ShadowStorm";					// Символьная строка, она задает имя класса окна
	wcx.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON2));	// Дескриптор маленького значка, который связан с классом окна.

	RegisterClassEx(&wcx);

	// Параметры для CreateWindowEx объясняются:
	// WS_EX_APPWINDOW: необязательный стиль расширенного окна.
	// wcx.lpszClassName: имя приложения.
	// title: текст, который отображается в строке заголовка
	// WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_SIZEBOX & ~WS_CAPTION & 
	// ~WS_THICKFRAME & ~WS_EX_DLGMODALFRAME & ~WS_EX_CLIENTEDGE &
	// ~WS_EX_STATICEDGE, : тип создаваемого окна
	// (GetSystemMetrics(SM_CXSCREEN) - width) / 2, (GetSystemMetrics(SM_CXSCREEN) - height): начальное положение (x, y)
	// width, height: начальный размер (ширина, длина)
	// NULL: родитель этого окна
	// NULL: у этого приложения нет строки меню
	// hInstance: первый параметр из WinMain
	// NULL: не используется в данном приложении
	_wnd = CreateWindowExW(
		WS_EX_APPWINDOW,
		wcx.lpszClassName,
		title,
		WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_SIZEBOX & ~WS_CAPTION &
		~WS_THICKFRAME & ~WS_EX_DLGMODALFRAME & ~WS_EX_CLIENTEDGE & ~WS_EX_STATICEDGE,
		(GetSystemMetrics(SM_CXSCREEN) - width) / 2,	// Ширина
		(GetSystemMetrics(SM_CXSCREEN) - height) / 5,	// Длина
		width,											// Начальный размер (ширина)
		height,											// Начальный размер (длина)
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