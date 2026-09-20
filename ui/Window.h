#pragma once
#include <Windows.h>
#include <functional>
#include <string>

// Настоящее Win32-окно: владеет HWND и зарегистрированным классом окна,
// освобождает их в деструкторе. Заменяет глобальный hwnd и статическую
// WndProc из прежнего класса UI.
//
// Сообщения приходят в экземпляр (указатель хранится в GWLP_USERDATA),
// поэтому окон может быть несколько и не нужны глобальные переменные.
class Window
{
public:
    // Обработчик владельца окна. Возвращает true, если сообщение обработано
    // (тогда result уходит в систему). Сюда подключаются ImGui, ресайз D3D
    // и — в дальнейшем — borderless-хиттест.
    using MessageHandler = std::function<bool(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& result)>;

private:
    HWND _hwnd = nullptr;
    HINSTANCE _instance = nullptr;
    std::wstring _className;
    MessageHandler _handler;
    bool _shouldClose = false;

    static LRESULT WINAPI WndProcTrampoline(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

public:
    Window() = default;
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool Create(const std::wstring& className,
                const std::wstring& title,
                int x, int y, int width, int height,
                HICON icon = nullptr,
                HICON iconSmall = nullptr);

    void Destroy();

    void Show(int cmdShow = SW_SHOW);
    void Hide();

    // Разбирает очередь сообщений. Возвращает false, когда окно закрывается.
    bool PumpMessages();

    void SetMessageHandler(MessageHandler handler) { _handler = std::move(handler); }

    HWND Handle() const { return _hwnd; }
    bool ShouldClose() const { return _shouldClose; }
    void RequestClose() { _shouldClose = true; }
};
