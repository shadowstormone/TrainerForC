#pragma once
#include "BaseRenderer.h"
#include <gdiplus.h>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <string>

#pragma comment(lib, "gdiplus.lib")

struct RenderState
{
    std::unique_ptr<Gdiplus::Font> optionFont;
    std::unique_ptr<Gdiplus::Font> processInformationFont;
    std::unique_ptr<Gdiplus::SolidBrush> optionBrush;
    std::unique_ptr<Gdiplus::SolidBrush> enabledOptionBrush;
    std::unique_ptr<Gdiplus::SolidBrush> processInfoBrush;
    std::unique_ptr<Gdiplus::SolidBrush> processRunningBrush;

    void Initialize();
    void Cleanup();
};

class SimpleRendererV2 : public BaseRender
{
private:
    std::unique_ptr<Gdiplus::Graphics> graphics;
    std::unique_ptr<Gdiplus::Bitmap> bitmap;
    RECT windowRect{};

    RenderState renderState;
    std::wstring processInfo;
    std::atomic<bool> isRunning{ false };
    std::thread renderThread;

    static LRESULT(*baseProc)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    LRESULT HandleWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void RenderLoop();
    void RenderBackground();
    void RenderOptions();
    void RenderProcessInfo();
    void InitializeGDIPlus();
    void CleanupGDIPlus();

public:
    ULONG_PTR gdiplusToken;

    SimpleRendererV2(Cheat* cheat, LPCWSTR title, int width, int height);
    ~SimpleRendererV2();
    void Stop();
};
