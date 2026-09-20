#include "ui/Window.h"

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
    // Сначала — обработчик владельца (ImGui, ресайз D3D, хиттест).
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

bool Window::Create(const std::wstring& className,
                    const std::wstring& title,
                    int x, int y, int width, int height,
                    HICON icon,
                    HICON iconSmall)
{
    _instance = ::GetModuleHandleW(nullptr);
    _className = className;

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = &Window::WndProcTrampoline;
    wc.hInstance = _instance;
    wc.hIcon = icon;
    wc.hIconSm = iconSmall;
    wc.lpszClassName = _className.c_str();

    if (!::RegisterClassExW(&wc))
    {
        // Класс мог быть зарегистрирован ранее — это не ошибка.
        if (::GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return false;
    }

    // this попадает в WM_NCCREATE и там сохраняется в GWLP_USERDATA.
    ::CreateWindowExW(
        0,
        _className.c_str(),
        title.c_str(),
        WS_OVERLAPPEDWINDOW,
        x, y, width, height,
        nullptr, nullptr, _instance,
        this);

    return _hwnd != nullptr;
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
