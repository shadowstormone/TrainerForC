#pragma once
#include "BaseRenderer.h"

struct RENDER_STATE
{
	HFONT optionFont;				// Шрифт надписей в трейнере
	HFONT processInformationFont;	// Шрифт информации о процессе
	COLORREF optionColor;			// Цвет опций не активных
	COLORREF enabledOptionColor;	// Цвет активированных опций
	COLORREF processInfoColor;		// Цвет информации о процессе
	COLORREF processRunningColor;	// Цвет когда процесс активен
};

class SimpleRenderer : public BaseRender
{
	HDC _windowDC = NULL;
	HDC _memDC = NULL;
	RECT _windowRect;
	HBITMAP _memBitmap = NULL;
	HGDIOBJ _defMemBmp = NULL;
	BYTE* _memBitmapBits = NULL;
	LONG _memDibSectionSize = 0;
	RENDER_STATE _rState = { 0 };
	std::wstring processInfo;

	static LRESULT (*baseProc)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT ThisWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	LRESULT DynamicWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void RenderFrame();

	/*HFONT SimpleCreateFont
	LPCWSTR fontFamily - Семейство шрифта
	int fontHeight - Размер
	bool isItalic - Курсив
	bool isUnderline - Подчеркнутый
	bool isStrikesOut - Перечеркнутый*/
	HFONT SimpleCreateFont(LPCWSTR fontFamily, int fontHeight, bool isItalic, bool isUnderline, bool isStrikesOut);

public:
	SimpleRenderer(Cheat* cheat, LPCWSTR title, int width, int height);

	void RenderLoop(HWND hWnd);

	void Start();
	void Stop();
};