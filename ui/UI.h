#pragma once
#include "ui/Drawing.h"
#include "core/Cheat.h"

#pragma comment(lib, "d3d11.lib")

// Forward declaration for ImGui Win32 handler
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Forward declarations
class TextureManager;

class UI
{
public:
    // Window dimensions
    static constexpr int WIDTH = 800;
    static constexpr int HEIGHT = 600;

    // Static member variables
    static ID3D11Device* pd3dDevice;
    static ID3D11DeviceContext* pd3dDeviceContext;
    static IDXGISwapChain* pSwapChain;
    static ID3D11RenderTargetView* pMainRenderTargetView;
    static HMODULE hCurrentModule;

    // Main methods
    static void Render();
    static std::string getFontPath();
    static std::pair<int, int> getScreenCenter();

    // DirectX methods
    static bool CreateDeviceD3D(HWND hWnd);
    static void CreateRenderTarget();
    static void CleanupRenderTarget();
    static void CleanupDeviceD3D();

    // Window procedure
    static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    // Private helper methods
    static void RenderLoop(ImGuiIO& io, TextureManager& textureManager);
    static void MakeWindowTopMostTemporary(HWND hWnd, int milliseconds = 2000);
    static void PerformAutoClick();
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