#include "SimpleRenderer.h"

LRESULT(*SimpleRenderer::baseProc)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

LRESULT SimpleRenderer::ThisWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	SimpleRenderer* pThis = reinterpret_cast<SimpleRenderer*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

	if (pThis)
	{
		pThis->DynamicWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return baseProc(hWnd, uMsg, wParam, lParam);
}

LRESULT SimpleRenderer::DynamicWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (uMsg)
	{
	case WM_LBUTTONDOWN:
		SendMessage(hWnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
		break;

	case WM_TIMER:
		if (wParam == 1000)
		{
			this->RenderFrame();
		}
		break;

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		BitBlt(hdc, _windowRect.left, _windowRect.top, _windowRect.right, _windowRect.bottom, _memDC, 0, 0, SRCCOPY);
		EndPaint(hWnd, &ps);
		break;

	default:
		break;
	}
	return 0;
}

void SimpleRenderer::RenderFrame()
{
	ZeroMemory(_memBitmapBits, _memDibSectionSize);
	HGDIOBJ oldFont = SelectObject(_memDC, _rState.optionFont);
	int deltaY = 25;

	for (auto& pair : _cheat->GetCheatOptionState())
	{
		if (pair.second)
		{
			SetTextColor(_memDC, _rState.enabledOptionColor);
		}
		else
		{
			SetTextColor(_memDC, _rState.optionColor);
		}
		TextOut(_memDC, 25, deltaY, pair.first, wcslen(pair.first));
		deltaY += 25;
	}

	SelectObject(_memDC, _rState.processInformationFont);
	processInfo.clear();
	processInfo.append(_cheat->GetProcessName());
	processInfo.append(L" ");

	if (_cheat->isProcessRunning())
	{
		SetTextColor(_memDC, _rState.processRunningColor);
		processInfo.append(L"is running.");
		TextOut(_memDC, 25, _windowRect.bottom - 45, processInfo.c_str(), processInfo.length());
	}
	else
	{
		SetTextColor(_memDC, _rState.processInfoColor);
		processInfo.append(L"is not running.");
		TextOut(_memDC, 25, _windowRect.bottom - 45, processInfo.c_str(), processInfo.length());
	}

	SelectObject(_memDC, oldFont);
	RedrawWindow(_wnd, NULL, NULL, RDW_INVALIDATE);
}

HFONT SimpleRenderer::SimpleCreateFont(LPCWSTR fontFamily, int fontHeight, bool isItalic = false, bool isUnderline = false, bool isStrikesOut = false)
{

	return CreateFont(fontHeight, 0, 0, 0, 0, isItalic, isUnderline, isStrikesOut, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, fontFamily);
}

SimpleRenderer::SimpleRenderer(Cheat* cheat, LPCWSTR title, int width, int height) : BaseRender(cheat, title, width, height)
{
	GetClientRect(_wnd, &_windowRect);
	BITMAPINFO bi;
	ZeroMemory(&bi, sizeof(BITMAPINFO));

	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth = _windowRect.right;
	bi.bmiHeader.biHeight = -_windowRect.bottom;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 32;

	_windowDC = GetDC(_wnd);
	_memDC = CreateCompatibleDC(_windowDC);
	_memBitmap = CreateDIBSection(_memDC, &bi, DIB_RGB_COLORS, reinterpret_cast<LPVOID*>(&_memBitmapBits), NULL, 0);
	_defMemBmp = SelectObject(_memDC, _memBitmap);
	_memDibSectionSize = _windowRect.right * _windowRect.bottom * 4L;

	SetWindowLongPtr(_wnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
	baseProc = reinterpret_cast<WNDPROC>(GetWindowLongPtr(_wnd, GWLP_WNDPROC));
	SetWindowLongPtr(_wnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(ThisWindowProc));
}

void SimpleRenderer::Start()
{
	SetBkMode(_memDC, TRANSPARENT);
	_rState.optionFont = SimpleCreateFont(L"EchoesSans-LightItalic", 20);
	_rState.processInformationFont = SimpleCreateFont(L"Tahoma", 18);
	_rState.optionColor = RGB(255, 255, 255);
	_rState.enabledOptionColor = RGB(0, 220, 0);
	_rState.processInfoColor = RGB(146, 146, 146);
	_rState.processRunningColor = RGB(38, 176, 1); //RED - RGB(250, 50, 50);

	SetTimer(_wnd, 1000, 1000 / 60, NULL);
	BaseRender::Start();
}

void SimpleRenderer::Stop()
{
	KillTimer(_wnd, 1000);
	SelectObject(_memDC, _defMemBmp);
	DeleteObject(_memBitmap);
	DeleteDC(_memDC);
	ReleaseDC(_wnd, _windowDC);
}
