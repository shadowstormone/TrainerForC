#include "SimpleRendererv2.h"
#include <stdexcept>
#include <sstream>
#include "UI.h"

LRESULT(*SimpleRendererV2::baseProc)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) = nullptr;

void RenderState::Initialize()
{
    optionFont = std::make_unique<Gdiplus::Font>(L"ResourceExtern\\FRIZQT.ttf", 20.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    processInformationFont = std::make_unique<Gdiplus::Font>(L"ResourceExtern\\FRIZQT.ttf", 18.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    optionBrush = std::make_unique<Gdiplus::SolidBrush>(Gdiplus::Color(255, 255, 255, 255));
    enabledOptionBrush = std::make_unique<Gdiplus::SolidBrush>(Gdiplus::Color(255, 0, 220, 0));
    processInfoBrush = std::make_unique<Gdiplus::SolidBrush>(Gdiplus::Color(255, 146, 146, 146));
    processRunningBrush = std::make_unique<Gdiplus::SolidBrush>(Gdiplus::Color(255, 38, 176, 1));
    pidInfoColorBrush = std::make_unique<Gdiplus::SolidBrush>(Gdiplus::Color(255, 12, 176, 197));
}

void RenderState::Cleanup()
{
    // Место для будущей очистки ресурсов, если потребуется
    // В настоящее время управление ресурсами осуществляется через смарт-указатели.
}

SimpleRendererV2::SimpleRendererV2(Cheat* cheat, LPCWSTR title, int width, int height)
    : BaseRender(cheat, title, width, height), isRunning(true)
{
    InitializeLogger(title);
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
    spdlog::info("Rendering thread started");
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
        spdlog::error("Failed to initialize GDI+");
        throw std::runtime_error("Failed to initialize GDI+");
    }
    spdlog::info("GDI+ initialized successfully");
}

void SimpleRendererV2::CleanupGDIPlus() const
{
    Gdiplus::GdiplusShutdown(gdiplusToken);
}

std::string SimpleRendererV2::WStringToUtf8(const std::wstring& wstr)
{
    if (wstr.empty()) return {};

    // Узнаем, сколько байт потребуется для результата
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size_needed <= 0)
    {
        throw std::runtime_error("WideCharToMultiByte failed");
    }

    // Создаем буфер для результата
    std::string utf8str(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8str[0], size_needed, nullptr, nullptr);

    // Удаляем завершающий нулевой символ, т.к. std::string автоматически добавляет его
    utf8str.pop_back();
    return utf8str;
}

void SimpleRendererV2::InitializeLogger(LPCWSTR title)
{
#ifdef _DEBUG
    try
    {
        auto fileLogger = spdlog::basic_logger_mt("file_logger", "logs/simple_renderer.log");
        spdlog::set_default_logger(fileLogger);
        spdlog::set_level(spdlog::level::info);

        spdlog::info("SimpleRendererV2 initialized with title: {}", WStringToUtf8(title));
    }
    catch (const spdlog::spdlog_ex& ex)
    {
        throw std::runtime_error("Failed to initialize logger: " + std::string(ex.what()));
    }
#endif // _DEBUG
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
        spdlog::info("WM_CLOSE received, stopping renderer");
        Stop(); // Останавливаем рендеринг
        DestroyWindow(hWnd); // Закрываем окно
        break;

    case WM_DESTROY:
        spdlog::info("WM_DESTROY received, quitting application");
        PostQuitMessage(0); // Завершаем приложение
        break;

    default:
        break;
    }
    return baseProc(hWnd, uMsg, wParam, lParam);
}

void SimpleRendererV2::RenderLoop()
{
    auto lastTime = std::chrono::steady_clock::now();

    while (isRunning)
    {
        auto currentTime = std::chrono::steady_clock::now();
        auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastTime);

        if (delta.count() >= 16)  // ~60 FPS
        {
            RenderBackground();
            RenderOptions();
            RenderProcessInfo();
            RedrawWindow(_wnd, nullptr, nullptr, RDW_INVALIDATE);
            lastTime = currentTime;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Минимальная нагрузка на процессор
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
    try
    {
        // Получаем название процесса
        std::wstring processName = _cheat->GetProcessName();
        size_t dotPos = processName.find_last_of(L'.');
        if (dotPos != std::wstring::npos)
        {
            processName = processName.substr(0, dotPos);
        }

        // Основная информация о процессе
        std::wstringstream ss;
        bool isRunning = _cheat->isProcessRunning();
        ss << processName << L" " << (isRunning ? L"is running" : L"is not running");

        auto* brush = isRunning ? renderState.processRunningBrush.get() : renderState.processInfoBrush.get();
        auto* pidColorBrush = renderState.pidInfoColorBrush.get();

        graphics->DrawString(ss.str().c_str(), -1, renderState.processInformationFont.get(),
            Gdiplus::PointF(25.0f, static_cast<Gdiplus::REAL>(windowRect.bottom - 70)), brush);

        // PID процесса
        std::wstringstream ssPID;
        if (isRunning)
        {
            int processID = _cheat->GetProcessID();
            if (processID > 0)
            {
                ssPID << L"PID Process: " << processID;
            }
            else
            {
                ssPID << L"PID Process: Error retrieving PID";
                spdlog::warn("Failed to retrieve PID for process: {}", WStringToUtf8(processName));
            }
        }
        else
        {
            ssPID << L"PID Process: N/A";
        }

        graphics->DrawString(ssPID.str().c_str(), -1, renderState.processInformationFont.get(),
            Gdiplus::PointF(25.0f, static_cast<Gdiplus::REAL>(windowRect.bottom - 45)), pidColorBrush);
    }
    catch (const std::exception& ex)
    {
        // Логируем ошибки
        spdlog::error("Exception in RenderProcessInfo: {}", ex.what());
    }
}