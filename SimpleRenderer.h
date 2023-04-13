#pragma once
#include "BaseRenderer.h"

struct RENDER_STATE
{
	HFONT optionFont;				// ����� �������� � ��������
	HFONT processInformationFont;	// ����� ���������� � ��������
	COLORREF optionColor;			// ���� ����� �� ��������
	COLORREF enabledOptionColor;	// ���� �������������� �����
	COLORREF processInfoColor;		// ���� ���������� � ��������
	COLORREF processRunningColor;	// ���� ����� ������� �������
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
	LPCWSTR fontFamily - ��������� ������
	int fontHeight - ������
	bool isItalic - ������
	bool isUnderline - ������������
	bool isStrikesOut - �������������*/
	HFONT SimpleCreateFont(LPCWSTR fontFamily, int fontHeight, bool isItalic, bool isUnderline, bool isStrikesOut);

public:
	SimpleRenderer(Cheat* cheat, LPCWSTR title, int width, int height);

	void RenderLoop(HWND hWnd);
	void Start();
	void Stop();
};