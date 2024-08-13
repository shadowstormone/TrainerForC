#pragma once
#include "BaseRenderer.h"
#include "StarFieldEffect.h"

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
	bool _isRunning;
	StarFieldEffect* starField = NULL;
	std::thread _renderThread;

	static LRESULT (*baseProc)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT ThisWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	LRESULT DynamicWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void RenderFrame();

	/*HFONT SimpleCreateFont
	LPCWSTR fontFamily - ��������� ������
	int fontHeight - ������
	int fontWidth - ������������� ������� ������ � ��������� �� 0 �� 1000
	bool isItalic - ������
	bool isUnderline - ������������
	bool isStrikesOut - �������������*/
	HFONT SimpleCreateFont(LPCWSTR fontFamily, int fontSize, int fontWidth, bool isItalic, bool isUnderline, bool isStrikesOut);

	/*
		FW_DONTCARE		0
		FW_THIN			100
		FW_EXTRALIGHT	200
		FW_ULTRALIGHT	200
		FW_LIGHT		300
		FW_NORMAL		400
		FW_REGULAR		400
		FW_MEDIUM		500
		FW_SEMIBOLD		600
		FW_DEMIBOLD		600
		FW_BOLD			700
		FW_EXTRABOLD	800
		FW_ULTRABOLD	800
		FW_HEAVY		900
		FW_BLACK		1000
	*/


	void SimpleThreadFunc();

public:
	SimpleRenderer(Cheat* cheat, LPCWSTR title, int width, int height);
	~SimpleRenderer();

	void Stop();
};