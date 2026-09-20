#include "ui/Window.h"

#include <dwmapi.h>
#include <windowsx.h> // GET_X_LPARAM / GET_Y_LPARAM

#pragma comment(lib, "dwmapi.lib")

// Значения появились в SDK для Windows 11; объявляем сами, чтобы
// собираться и более старым SDK. На Windows 10 вызов просто вернёт ошибку.
#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif
#ifndef DWMWCP_ROUND
#define DWMWCP_ROUND 2
#endif

Window::~Window()
{
    Destroy();
}

LRESULT WINAPI Window::WndProcTrampoline(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    Window* self = nullptr;

    if (msg == WM_NCCREATE)
    {
        // Указатель на экземпляр передан через CreateWindowEx(..., this).
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = static_cast<Window*>(cs->lpCreateParams);
        if (self)
        {
            self->_hwnd = hWnd;
            ::SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
    }
    else
    {
        self = reinterpret_cast<Window*>(::GetWindowLongPtrW(hWnd, GWLP_USERDATA));
    }

    if (self) return self->HandleMessage(hWnd, msg, wParam, lParam);
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

LRESULT Window::HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Borderless обрабатываем до обработчика владельца: эти два сообщения
    // формируют геометрию окна, ImGui их не касается.
    if (_borderless)
    {
        switch (msg)
        {
        case WM_NCCALCSIZE:
            // Возвращаем 0, НЕ трогая переданный прямоугольник: неклиентская
            // область схлопывается, клиент занимает всё окно, системный
            // заголовок и рамка исчезают.
            if (wParam == TRUE) return 0;
            break;

        case WM_NCHITTEST:
        {
            POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ::ScreenToClient(hWnd, &pt);

            // Полоса заголовка -> система сама тащит окно.
            // Над кнопками и содержимым -> HTCLIENT, иначе клики уедут
            // в перетаскивание.
            if (_captionHitTest && _captionHitTest(pt)) return HTCAPTION;
            return HTCLIENT;
        }
        }
    }

    // Обработчик владельца (ImGui, ресайз D3D).
    if (_handler)
    {
        LRESULT result = 0;
        if (_handler(hWnd, msg, wParam, lParam, result))
        {
            return result;
        }
    }

    switch (msg)
    {
    case WM_SYSCOMMAND:
        // Отключаем системное меню по Alt.
        if ((wParam & 0xfff0) == SC_KEYMENU) return 0;
        break;

    case WM_DESTROY:
        _shouldClose = true;
        _hwnd = nullptr;
        ::PostQuitMessage(0);
        return 0;
    }

    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

void Window::ApplyRoundedCorners()
{
    if (!_hwnd) return;

    const DWORD preference = DWMWCP_ROUND;
    ::DwmSetWindowAttribute(_hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &preference, sizeof(preference));
}

bool Window::Create(const WindowDesc& desc)
{
    _instance = ::GetModuleHandleW(nullptr);
    _className = desc.className;
    _borderless = desc.borderless;

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = &Window::WndProcTrampoline;
    wc.hInstance = _instance;
    wc.hIcon = desc.icon;
    wc.hIconSm = desc.iconSmall;
    wc.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = _className.c_str();

    if (!::RegisterClassExW(&wc))
    {
        // Класс мог быть зарегистрирован ранее — это не ошибка.
        if (::GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return false;
    }

    // Окно остаётся обычным (WS_OVERLAPPEDWINDOW): таскбар, своя иконка,
    // alt-tab и сворачивание работают сами. У неизменяемого по размеру окна
    // убираем рамку-растяжку и кнопку разворота.
    DWORD style = WS_OVERLAPPEDWINDOW;
    if (!desc.resizable)
    {
        style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
    }

    // this попадает в WM_NCCREATE и там сохраняется в GWLP_USERDATA.
    ::CreateWindowExW(
        0,
        _className.c_str(),
        desc.title.c_str(),
        style,
        desc.x, desc.y, desc.width, desc.height,
        nullptr, nullptr, _instance,
        this);

    if (!_hwnd) return false;

    if (desc.roundedCorners) ApplyRoundedCorners();

    if (_borderless)
    {
        // Просим систему пересчитать рамку — тогда WM_NCCALCSIZE применится
        // сразу, ещё до первого показа окна.
        ::SetWindowPos(_hwnd, nullptr, 0, 0, 0, 0,
                       SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    return true;
}

void Window::Show(int cmdShow)
{
    if (_hwnd)
    {
        ::ShowWindow(_hwnd, cmdShow);
        ::UpdateWindow(_hwnd);
    }
}

void Window::Hide()
{
    if (_hwnd) ::ShowWindow(_hwnd, SW_HIDE);
}

void Window::Minimize()
{
    if (_hwnd) ::ShowWindow(_hwnd, SW_MINIMIZE);
}

void Window::RequestClose()
{
    if (_hwnd) ::PostMessageW(_hwnd, WM_CLOSE, 0, 0);
    else _shouldClose = true;
}

bool Window::PumpMessages()
{
    MSG msg;
    while (::PeekMessageW(&msg, nullptr, 0U, 0U, PM_REMOVE))
    {
        ::TranslateMessage(&msg);
        ::DispatchMessageW(&msg);

        if (msg.message == WM_QUIT)
        {
            _shouldClose = true;
            return false;
        }
    }
    return !_shouldClose;
}

void Window::Destroy()
{
    if (_hwnd)
    {
        ::DestroyWindow(_hwnd);
        _hwnd = nullptr;
    }

    if (!_className.empty() && _instance)
    {
        ::UnregisterClassW(_className.c_str(), _instance);
        _className.clear();
    }
}
