#include "SimpleRendererv2.h"

LRESULT(*SimpleRendererv2::baseProc)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

SimpleRendererv2::SimpleRendererv2(Cheat* cheat, LPCWSTR title, int width, int height)
    : BaseRender(cheat, title, width, height), _isRunning(true)
{
    GetClientRect(_wnd, &_windowRect);

    // Инициализация GDI+
    InitializeGDIPlus();

    // Создаем ресурсы GDI+
    _bitmap = new Gdiplus::Bitmap(_windowRect.right, _windowRect.bottom, PixelFormat32bppARGB);
    _graphics = Gdiplus::Graphics::FromImage(_bitmap);
    _graphics->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    _graphics->SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

    _rState.optionFont = new Gdiplus::Font(L"ResourceExtern\\FRIZQT.ttf", 20, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    _rState.processInformationFont = new Gdiplus::Font(L"ResourceExtern\\FRIZQT.ttf", 18, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    _rState.optionBrush = new Gdiplus::SolidBrush(Gdiplus::Color(255, 255, 255, 255));
    _rState.enabledOptionBrush = new Gdiplus::SolidBrush(Gdiplus::Color(255, 0, 220, 0));
    _rState.processInfoBrush = new Gdiplus::SolidBrush(Gdiplus::Color(255, 146, 146, 146));
    _rState.processRunningBrush = new Gdiplus::SolidBrush(Gdiplus::Color(255, 38, 176, 1));

    // Устанавливаем оконную процедуру
    SetWindowLongPtr(_wnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    baseProc = reinterpret_cast<WNDPROC>(GetWindowLongPtr(_wnd, GWLP_WNDPROC));
    SetWindowLongPtr(_wnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(ThisWindowProc));

    // Запускаем поток рендеринга
    _renderThread = std::thread(&SimpleRendererv2::RenderFrame, this);
    BaseRender::Start();
}

SimpleRendererv2::~SimpleRendererv2()
{
    Stop();

    // Удаление ресурсов GDI+
    delete _rState.optionFont;
    delete _rState.processInformationFont;
    delete _rState.optionBrush;
    delete _rState.enabledOptionBrush;
    delete _rState.processInfoBrush;
    delete _rState.processRunningBrush;
    delete _graphics;
    delete _bitmap;

    CleanupGDIPlus();
}

void SimpleRendererv2::InitializeGDIPlus()
{
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    if (Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr) != Gdiplus::Ok)
    {
        throw std::runtime_error("Failed to initialize GDI+");
    }
}

void SimpleRendererv2::CleanupGDIPlus()
{
    ULONG_PTR gdiplusToken = NULL;
    Gdiplus::GdiplusShutdown(gdiplusToken);
}

void SimpleRendererv2::Stop()
{
    _isRunning = false;
    if (_renderThread.joinable())
    {
        _renderThread.join();
    }
}

LRESULT SimpleRendererv2::ThisWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    SimpleRendererv2* pThis = reinterpret_cast<SimpleRendererv2*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (pThis)
    {
        pThis->DynamicWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return baseProc(hWnd, uMsg, wParam, lParam);
}

LRESULT SimpleRendererv2::DynamicWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) const
{
    HDC hdc = nullptr;
    switch (uMsg)
    {
    case WM_LBUTTONDOWN:
        SendMessage(hWnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        break;

    case WM_PAINT:
        PAINTSTRUCT ps;
        hdc = BeginPaint(hWnd, &ps);
        {
            Gdiplus::Graphics graphics(hdc);
            graphics.DrawImage(_bitmap, 0, 0);
        }
        EndPaint(hWnd, &ps);
        break;

    default:
        break;
    }
    return 0;
}

void SimpleRendererv2::RenderFrame()
{
    while (_isRunning)
    {
        // Очистка буфера градиентным фоном
        Gdiplus::LinearGradientBrush gradientBrush(Gdiplus::Rect(0, 0, _windowRect.right, _windowRect.bottom),
            Gdiplus::Color(255, 30, 30, 30),  // Верхний цвет
            Gdiplus::Color(255, 10, 10, 10),  // Нижний цвет
            Gdiplus::LinearGradientModeVertical);

        _graphics->FillRectangle(&gradientBrush, 0, 0, _windowRect.right, _windowRect.bottom);

        // Отрисовка опций
        int deltaY = 25;
        for (const auto& pair : _cheat->GetCheatOptionState())
        {
            Gdiplus::SolidBrush* brush = pair.second ? _rState.enabledOptionBrush : _rState.optionBrush;
            _graphics->DrawString(pair.first, -1, _rState.optionFont, Gdiplus::PointF(25.0f, static_cast<Gdiplus::REAL>(deltaY)), brush);
            deltaY += 25;
        }

        // Обновление информации о процессе
        std::wstring processName = _cheat->GetProcessName();
        size_t dotPos = processName.find_last_of(L'.');
        if (dotPos != std::wstring::npos)
        {
            processName = processName.substr(0, dotPos);
        }

        std::wstringstream ss;
        ss << processName << L" " << (_cheat->isProcessRunning() ? L"is running" : L"is not running");

        // Рисуем информацию о процессе
        _graphics->DrawString(ss.str().c_str(), -1, _rState.processInformationFont,
            Gdiplus::PointF(25.0f, static_cast<Gdiplus::REAL>(_windowRect.bottom - 70)),
            _cheat->isProcessRunning() ? _rState.processRunningBrush : _rState.processInfoBrush);

        // PID процесса
        std::wstringstream ssPID;
        if (_cheat->isProcessRunning())
        {
            ssPID << L"PID Process: " << _cheat->GetProcessID();
        }
        else
        {
            ssPID << L"PID Process: N/A";
        }

        // Использование цвета в зависимости от состояния
        Gdiplus::SolidBrush* pidBrush = _cheat->isProcessRunning() ? _rState.processRunningBrush : _rState.processInfoBrush;

        _graphics->DrawString(ssPID.str().c_str(), -1, _rState.processInformationFont,
            Gdiplus::PointF(25.0f, static_cast<Gdiplus::REAL>(_windowRect.bottom - 45)),
            pidBrush);

        // Обновляем окно
        RedrawWindow(_wnd, nullptr, nullptr, RDW_INVALIDATE);

        // Ограничение FPS (60 кадров в секунду)
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / 60));
    }
}
