#pragma once
#include "ui/Drawing.h"
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
    // Window dimensions
    static constexpr int WIDTH = 800;
    static constexpr int HEIGHT = 600;

    // Main methods
    static void Render();
    static std::string getFontPath();
    static std::pair<int, int> getScreenCenter();

private:
    // Private helper methods
    static void RenderLoop(Window& window, D3DContext& d3d, ImGuiIO& io, TextureManager& textureManager);
    static void MakeWindowTopMostTemporary(HWND hWnd, int milliseconds = 2000);
    static void PerformAutoClick(HWND hWnd);
    static void ApplyIconsToAllViewports(HINSTANCE hInstance);

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
