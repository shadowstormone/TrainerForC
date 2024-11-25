#pragma once
#include "BaseRenderer.h"
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

struct RENDER_STATEv2
{
    Gdiplus::Font* optionFont;                  // ����� �������� � ��������
    Gdiplus::Font* processInformationFont;      // ����� ���������� � ��������
    Gdiplus::SolidBrush* optionBrush;           // ���� ����� ����������
    Gdiplus::SolidBrush* enabledOptionBrush;    // ���� �������������� �����
    Gdiplus::SolidBrush* processInfoBrush;      // ���� ���������� � ��������
    Gdiplus::SolidBrush* processRunningBrush;   // ���� ����� ������� �������
};

class SimpleRendererv2 : public BaseRender
{
private:
    Gdiplus::Graphics* _graphics = nullptr;     // GDI+ Graphics ��� ���������
    Gdiplus::Bitmap* _bitmap = nullptr;         // ��������� � �����
    RECT _windowRect;

    RENDER_STATEv2 _rState;
    std::wstring processInfo;
    bool _isRunning;
    std::thread _renderThread;

    static LRESULT(*baseProc)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT ThisWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    LRESULT DynamicWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) const;
    void RenderFrame();
    void InitializeGDIPlus();
    void CleanupGDIPlus();
public:
    ULONG_PTR gdiplusToken;

    SimpleRendererv2(Cheat* cheat, LPCWSTR title, int width, int height);
    ~SimpleRendererv2();
    void Stop();
};