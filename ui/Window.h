#pragma once
#include <Windows.h>
#include <functional>
#include <string>

// Параметры создаваемого окна.
struct WindowDesc
{
    std::wstring className;
    std::wstring title;
    int x = CW_USEDEFAULT;
    int y = CW_USEDEFAULT;
    int width = 800;
    int height = 600;
    HICON icon = nullptr;
    HICON iconSmall = nullptr;

    // Убрать системную рамку и заголовок, оставив окно настоящим
    // (таскбар, своя иконка, alt-tab, сворачивание). Полосу заголовка
    // рисует ImGui, а перетаскивание обеспечивает captionHitTest.
    bool borderless = false;

    // Разрешить пользователю менять размер мышью.
    bool resizable = true;

    // Скруглить углы (Windows 11; на более старых просто игнорируется).
    bool roundedCorners = false;
};

// Настоящее Win32-окно: владеет HWND и зарегистрированным классом окна,
// освобождает их в деструкторе. Сообщения приходят в экземпляр
// (указатель хранится в GWLP_USERDATA), поэтому глобальные переменные не нужны.
class Window
{
public:
    // Обработчик владельца окна. Возвращает true, если сообщение обработано
    // (тогда result уходит в систему). Сюда подключается ImGui и ресайз D3D.
    using MessageHandler = std::function<bool(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& result)>;

    // Для borderless: вернуть true, если точка (в клиентских координатах)
    // относится к полосе заголовка — тогда система даст перетащить окно.
    // Над кнопками нужно возвращать false, иначе клики уйдут в перетаскивание.
    using CaptionHitTest = std::function<bool(POINT clientPoint)>;

private:
    HWND _hwnd = nullptr;
    HINSTANCE _instance = nullptr;
    std::wstring _className;
    MessageHandler _handler;
    CaptionHitTest _captionHitTest;
    bool _borderless = false;
    bool _shouldClose = false;

    static LRESULT WINAPI WndProcTrampoline(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void ApplyRoundedCorners();

public:
    Window() = default;
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool Create(const WindowDesc& desc);
    void Destroy();

    void Show(int cmdShow = SW_SHOW);
    void Hide();
    void Minimize();

    // Разбирает очередь сообщений. Возвращает false, когда окно закрывается.
    bool PumpMessages();

    void SetMessageHandler(MessageHandler handler) { _handler = std::move(handler); }
    void SetCaptionHitTest(CaptionHitTest test) { _captionHitTest = std::move(test); }

    HWND Handle() const { return _hwnd; }
    bool ShouldClose() const { return _shouldClose; }
    void RequestClose();
};
