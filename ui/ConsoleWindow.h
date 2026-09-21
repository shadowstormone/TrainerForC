#pragma once
#include <Windows.h>

#include <functional>

#include "ui/D3DContext.h"
#include "ui/Window.h"

struct ImGuiContext;
struct ImGuiIO;
class Console;

// Отладочная консоль в СВОЁМ окне операционной системы.
//
// ImGui без multi-viewport рисует все свои окна внутри одного HWND, поэтому
// консоль жила поверх панели трейнера и перекрывала её целиком. Здесь у неё
// собственное окно, собственный swapchain и собственный контекст ImGui: её
// можно двигать, растягивать и уводить на второй монитор.
//
// Второй контекст — это не роскошь, а требование: бэкенды ImGui для Win32 и
// DX11 хранят свои данные в контексте, и один контекст не может обслуживать
// два окна с разными swapchain.
class ConsoleWindow
{
    Window _window;
    D3DContext _d3d;
    ImGuiContext* _imgui = nullptr;
    Console* _console = nullptr;
    bool _visible = false;

    // Выполняет действие с активным контекстом консоли и возвращает
    // предыдущий на место. Забыть вернуть — значит испортить кадр панели.
    void WithContext(const std::function<void()>& action);

public:
    ConsoleWindow() = default;
    ~ConsoleWindow();

    ConsoleWindow(const ConsoleWindow&) = delete;
    ConsoleWindow& operator=(const ConsoleWindow&) = delete;

    // configure получает ImGuiIO нового контекста: туда UI передаёт свой
    // стиль и шрифт, иначе у консоли будет шрифт по умолчанию без кириллицы.
    bool Create(Console& console, HINSTANCE instance,
                const std::function<void(ImGuiIO&)>& configure);

    void Destroy();

    bool IsVisible() const { return _visible; }
    void SetVisible(bool visible);
    void Toggle() { SetVisible(!_visible); }

    // Рисует кадр консоли. Сообщения качает общий насос главного окна:
    // PeekMessage разбирает очередь всего потока, включая это окно.
    void Draw();
};
