#pragma once
#include "BaseRenderer.h"
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

struct RENDER_STATEv2
{
    Gdiplus::Font* optionFont;                  // Шрифт надписей в трейнере
    Gdiplus::Font* processInformationFont;      // Шрифт информации о процессе
    Gdiplus::SolidBrush* optionBrush;           // Цвет опций неактивных
    Gdiplus::SolidBrush* enabledOptionBrush;    // Цвет активированных опций
    Gdiplus::SolidBrush* processInfoBrush;      // Цвет информации о процессе
    Gdiplus::SolidBrush* processRunningBrush;   // Цвет когда процесс активен
};

class SimpleRendererv2 : public BaseRender
{
private:
    Gdiplus::Graphics* _graphics = nullptr;     // GDI+ Graphics для рисования
    Gdiplus::Bitmap* _bitmap = nullptr;         // Рендеринг в буфер
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