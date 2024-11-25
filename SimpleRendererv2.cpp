#include "SimpleRendererv2.h"
#include <stdexcept>
#include <sstream>

LRESULT(*SimpleRendererV2::baseProc)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) = nullptr;

void RenderState::Initialize()
{
    optionFont = std::make_unique<Gdiplus::Font>(L"ResourceExtern\\FRIZQT.ttf", 20.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    processInformationFont = std::make_unique<Gdiplus::Font>(L"ResourceExtern\\FRIZQT.ttf", 18.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    optionBrush = std::make_unique<Gdiplus::SolidBrush>(Gdiplus::Color(255, 255, 255, 255));
    enabledOptionBrush = std::make_unique<Gdiplus::SolidBrush>(Gdiplus::Color(255, 0, 220, 0));
    processInfoBrush = std::make_unique<Gdiplus::SolidBrush>(Gdiplus::Color(255, 146, 146, 146));
    processRunningBrush = std::make_unique<Gdiplus::SolidBrush>(Gdiplus::Color(255, 38, 176, 1));
}

void RenderState::Cleanup()
{
    // Cleanup handled by unique_ptr
}

SimpleRendererV2::SimpleRendererV2(Cheat* cheat, LPCWSTR title, int width, int height)
    : BaseRender(cheat, title, width, height), isRunning(true)
{
    GetClientRect(_wnd, &windowRect);

    InitializeGDIPlus();
    bitmap = std::make_unique<Gdiplus::Bitmap>(windowRect.right, windowRect.bottom, PixelFormat32bppARGB);
    graphics = std::make_unique<Gdiplus::Graphics>(bitmap.get());
    graphics->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    graphics->SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

    renderState.Initialize();

    SetWindowLongPtr(_wnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    baseProc = reinterpret_cast<WNDPROC>(GetWindowLongPtr(_wnd, GWLP_WNDPROC));
    SetWindowLongPtr(_wnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WindowProc));

    renderThread = std::thread(&SimpleRendererV2::RenderLoop, this);
    BaseRender::Start();
}

SimpleRendererV2::~SimpleRendererV2()
{
    Stop();
    renderState.Cleanup();
    CleanupGDIPlus();
}

void SimpleRendererV2::InitializeGDIPlus()
{
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    if (Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr) != Gdiplus::Ok)
    {
        throw std::runtime_error("Failed to initialize GDI+");
    }
}

void SimpleRendererV2::CleanupGDIPlus()
{
    Gdiplus::GdiplusShutdown(gdiplusToken);
}

void SimpleRendererV2::Stop()
{
    isRunning = false;
    if (renderThread.joinable())
    {
        renderThread.join();
    }
}

LRESULT SimpleRendererV2::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    auto* pThis = reinterpret_cast<SimpleRendererV2*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (pThis)
    {
        return pThis->HandleWindowMessage(hWnd, uMsg, wParam, lParam);
    }
    return baseProc(hWnd, uMsg, wParam, lParam);
}

LRESULT SimpleRendererV2::HandleWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_LBUTTONDOWN:
        SendMessage(hWnd, WM_NCLBUTTONDOWN, HTCAPTION, 0); // Для перемещения окна
        break;

    case WM_NCHITTEST:
        {
        // Перемещение окна за любое место
        POINT cursorPos;
        GetCursorPos(&cursorPos);
        ScreenToClient(hWnd, &cursorPos);
        return HTCAPTION;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        Gdiplus::Graphics graphics(hdc);
        graphics.DrawImage(bitmap.get(), 0, 0);
        EndPaint(hWnd, &ps);
        break;
    }

    case WM_CLOSE:
        Stop(); // Останавливаем рендеринг
        DestroyWindow(hWnd); // Закрываем окно
        break;

    case WM_DESTROY:
        PostQuitMessage(0); // Завершаем приложение
        break;

    default:
        break;
    }
    return baseProc(hWnd, uMsg, wParam, lParam);
}

void SimpleRendererV2::RenderLoop()
{
    while (isRunning)
    {
        RenderBackground();
        RenderOptions();
        RenderProcessInfo();
        RedrawWindow(_wnd, nullptr, nullptr, RDW_INVALIDATE);
        std::this_thread::sleep_for(std::chrono::milliseconds(16));  // ~60 FPS
    }
}

void SimpleRendererV2::RenderBackground()
{
    Gdiplus::LinearGradientBrush gradientBrush(
        Gdiplus::Rect(0, 0, windowRect.right, windowRect.bottom),
        Gdiplus::Color(255, 30, 30, 30),
        Gdiplus::Color(255, 10, 10, 10),
        Gdiplus::LinearGradientModeVertical);
    graphics->FillRectangle(&gradientBrush, 0, 0, windowRect.right, windowRect.bottom);
}

void SimpleRendererV2::RenderOptions()
{
    float deltaY = 25.0f;
    for (const auto& pair : _cheat->GetCheatOptionState())
    {
        auto* brush = pair.second ? renderState.enabledOptionBrush.get() : renderState.optionBrush.get();
        graphics->DrawString(pair.first, -1, renderState.optionFont.get(),
            Gdiplus::PointF(25.0f, deltaY), brush);
        deltaY += 25.0f;
    }
}

void SimpleRendererV2::RenderProcessInfo()
{
    std::wstring processName = _cheat->GetProcessName();
    size_t dotPos = processName.find_last_of(L'.');
    if (dotPos != std::wstring::npos)
    {
        processName = processName.substr(0, dotPos);
    }

    // Основная информация о процессе
    std::wstringstream ss;
    ss << processName << L" " << (_cheat->isProcessRunning() ? L"is running" : L"is not running");
    auto* brush = _cheat->isProcessRunning() ? renderState.processRunningBrush.get() : renderState.processInfoBrush.get();

    graphics->DrawString(ss.str().c_str(), -1, renderState.processInformationFont.get(),
        Gdiplus::PointF(25.0f, static_cast<Gdiplus::REAL>(windowRect.bottom - 70)), brush);

    // PID процесса
    std::wstringstream ssPID;
    if (_cheat->isProcessRunning())
    {
        int processID = _cheat->GetProcessID();
        if (processID > 0)
        {
            ssPID << L"PID Process: " << processID;
        }
        else
        {
            ssPID << L"PID Process: Error retrieving PID";
        }
    }
    else
    {
        ssPID << L"PID Process: N/A";
    }

    graphics->DrawString(ssPID.str().c_str(), -1, renderState.processInformationFont.get(),
        Gdiplus::PointF(25.0f, static_cast<Gdiplus::REAL>(windowRect.bottom - 45)), brush);
}