#pragma once
#include "ui/MainView.h"
#include "core/Cheat.h"

#pragma comment(lib, "d3d11.lib")

// Forward declaration for ImGui Win32 handler
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Forward declarations
class TextureManager;
class D3DContext;
class Window;

// Композиция UI: создаёт окно и контекст DirectX (оба — обычные объекты
// с RAII) и крутит цикл отрисовки. Состояние больше не живёт в статических
// полях и глобальном hwnd — им владеет Render().
class UI
{
public:
    // Размер окна. Окно теперь borderless, поэтому клиентская область
    // равна всему окну и совпадает с размером интерфейса ImGui.
    static constexpr int WIDTH = 500;
    static constexpr int HEIGHT = 555;

    // Main methods
    static void Render(MainView& view);
    static std::string getFontPath();
    static std::pair<int, int> getScreenCenter();

private:
    // Private helper methods
    static void RenderLoop(Window& window, D3DContext& d3d, MainView& view, ImGuiIO& io, TextureManager& textureManager);

    // Prevent instantiation
    UI() = delete;
    ~UI() = delete;
    UI(const UI&) = delete;
    UI& operator=(const UI&) = delete;
};

// External dependencies (assumed to be defined elsewhere)
extern bool showConsole;
extern class Console* gConsole;

// Theme function (assumed to be defined elsewhere)
void SetModernDarkStyle();
