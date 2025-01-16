#define NOMINMAX
#include "resource.h"
#include "UI.h"
#include "ImGuiThemes.h"
#include <shlobj.h>
#include <KnownFolders.h>
#include <filesystem>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

ID3D11Device* UI::pd3dDevice = nullptr;
ID3D11DeviceContext* UI::pd3dDeviceContext = nullptr;
IDXGISwapChain* UI::pSwapChain = nullptr;
ID3D11RenderTargetView* UI::pMainRenderTargetView = nullptr;
HMODULE UI::hCurrentModule = nullptr;

namespace {
    constexpr float FADE_DURATION = 1.5f; // Длительность анимации в секундах
    float currentAlpha = 0.0f;
    bool isFirstFrame = true;
    std::chrono::steady_clock::time_point startTime;
}

static ID3D11ShaderResourceView* LoadTextureFromResource(ID3D11Device* device, ID3D11DeviceContext* context, HMODULE hModule, int resourceId, const char* resourceType)
{
    // Преобразуем ANSI строку `resourceType` в Unicode
    wchar_t wResourceType[64];
    MultiByteToWideChar(CP_UTF8, 0, resourceType, -1, wResourceType, sizeof(wResourceType) / sizeof(wResourceType[0]));

    // Найти ресурс
    HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(resourceId), wResourceType);
    if (!hResource)
        return nullptr;

    // Загрузить ресурс
    HGLOBAL hLoadedResource = LoadResource(hModule, hResource);
    if (!hLoadedResource)
        return nullptr;

    // Заблокировать ресурс для доступа к данным
    void* pResourceData = LockResource(hLoadedResource);
    DWORD resourceSize = SizeofResource(hModule, hResource);
    if (!pResourceData || resourceSize == 0)
        return nullptr;

    // Загрузить изображение из памяти с помощью stb_image
    int width, height, channels;
    unsigned char* imageData = stbi_load_from_memory((unsigned char*)pResourceData, resourceSize, &width, &height, &channels, 4);
    if (!imageData)
        return nullptr;

    // Создать текстуру DirectX
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = imageData;
    initData.SysMemPitch = width * 4;

    ID3D11Texture2D* texture = nullptr;
    ID3D11ShaderResourceView* textureView = nullptr;
    if (SUCCEEDED(device->CreateTexture2D(&desc, &initData, &texture)))
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = desc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        device->CreateShaderResourceView(texture, &srvDesc, &textureView);
        texture->Release();
    }

    stbi_image_free(imageData);
    return textureView;
}

bool UI::CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    const UINT createDeviceFlags = 0;

    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &pSwapChain, &pd3dDevice, &featureLevel, &pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void UI::CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (pBackBuffer != nullptr)
    {
        pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &pMainRenderTargetView);
        pBackBuffer->Release();
    }
}

void UI::CleanupRenderTarget()
{
    if (pMainRenderTargetView)
    {
        pMainRenderTargetView->Release();
        pMainRenderTargetView = nullptr;
    }
}

void UI::CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (pSwapChain)
    {
        pSwapChain->Release();
        pSwapChain = nullptr;
    }

    if (pd3dDeviceContext)
    {
        pd3dDeviceContext->Release();
        pd3dDeviceContext = nullptr;
    }

    if (pd3dDevice)
    {
        pd3dDevice->Release();
        pd3dDevice = nullptr;
    }
}

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0 // From Windows SDK 8.1+ headers
#endif

LRESULT WINAPI UI::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;

    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;

    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;

    case WM_DPICHANGED:
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DpiEnableScaleViewports)
        {
            const RECT* suggested_rect = (RECT*)lParam;
            ::SetWindowPos(hWnd, nullptr, suggested_rect->left, suggested_rect->top, suggested_rect->right - suggested_rect->left, suggested_rect->bottom - suggested_rect->top, SWP_NOZORDER | SWP_NOACTIVATE);
        }
        break;

    default:
        break;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}

std::string UI::GetFontPath()
{
    PWSTR path = nullptr;
    std::string fontPath;

    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &path)))
    {
        // Конвертируем WCHAR* в string
        int size = WideCharToMultiByte(CP_UTF8, 0, path, -1, nullptr, 0, nullptr, nullptr);
        std::string localAppData(size, 0);
        WideCharToMultiByte(CP_UTF8, 0, path, -1, &localAppData[0], size, nullptr, nullptr);

        // Удаляем завершающий нуль
        localAppData.pop_back();

        fontPath = localAppData + "\\Microsoft\\Windows\\Fonts\\FRIZQT.ttf";
        CoTaskMemFree(path);
    }

    return fontPath;
}

void UI::Render()
{
    ImGui_ImplWin32_EnableDpiAwareness();
    const WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, _T("Test Trainer"), nullptr };
    ::RegisterClassEx(&wc);
    const HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("Test Trainer"), WS_OVERLAPPEDWINDOW, 100, 100, 50, 50, NULL, NULL, wc.hInstance, NULL);

    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return;
    }

    ::ShowWindow(hwnd, SW_HIDE);
    ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    //ImGui::StyleColorsDark();
    SetModernDarkStyle();

    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 4.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    const HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO info = {};
    info.cbSize = sizeof(MONITORINFO);
    GetMonitorInfo(monitor, &info);
    const int monitor_height = info.rcMonitor.bottom - info.rcMonitor.top;
    const int monitor_width = GetSystemMetrics(SM_CXSCREEN);

#ifdef _DEBUG
    // Показ сообщения с разрешением экрана
    char message[128];
    sprintf_s(message, "Screen Resolution:\nWidth: %d\nHeight: %d", monitor_width, monitor_height);
    MessageBoxA(nullptr, message, "Screen Resolution Info", MB_OK | MB_ICONINFORMATION);
#endif // _DEBUG

    // Устанавливаем масштаб в зависимости от высоты экрана
    float fScale;
    if (monitor_height > 1440)
    {
        fScale = 2.2f;  // Большой масштаб для разрешений выше 2К монитора
    }
    else if (monitor_height > 1080)
    {
        fScale = 1.9f; // Большой масштаб для разрешений выше 1080
    }
    else
    {
        fScale = 1.3f; // Меньший масштаб для разрешений 1080 и ниже
    }

    // Добавляем диапазон символов кириллицы
    static const ImWchar ranges[] = {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
        0x2DE0, 0x2DFF, // Cyrillic Extended-A
        0xA640, 0xA69F, // Cyrillic Extended-B
        0 };

    ImFontConfig cfg;
    cfg.SizePixels = 13 * fScale; // Применяем масштаб к размеру шрифта
    cfg.PixelSnapH = true;
    cfg.OversampleH = 1;
    cfg.OversampleV = 1;

    std::string fontPath = GetFontPath();   // Путь к шрифту
    if (std::filesystem::exists(fontPath))  // Перед использованием шрифта проверяем его существование
    {
        ImFont* font = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 13 * fScale, &cfg, ranges);   // Загружаем шрифт
        if (!font)
        {
            MessageBoxA(nullptr, "Failed to load font.", "Font Load Error", MB_OK | MB_ICONERROR);
        }
    }
    else
    {
        ImFont* font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\arial.ttf", 13 * fScale, &cfg, ranges); // Загружаем дефолтный шрифт если нужный не найден
        if (!font)
        {
            MessageBoxA(nullptr, "Font file not found.", "Font Load Error", MB_OK | MB_ICONERROR);
        }
    }

    ImGui::GetIO().IniFilename = nullptr;

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(pd3dDevice, pd3dDeviceContext);

    // Загрузка текстур
    static ID3D11ShaderResourceView* successIcon = LoadTextureFromResource(pd3dDevice, pd3dDeviceContext, GetModuleHandle(nullptr), IDB_PNG1, "PNG");
    static ID3D11ShaderResourceView* errorIcon = LoadTextureFromResource(pd3dDevice, pd3dDeviceContext, GetModuleHandle(nullptr), IDB_PNG2, "PNG");
    if (!successIcon || !errorIcon)
    {
        MessageBoxA(nullptr, "Failed to load one or more textures.", "Texture Load Error", MB_OK | MB_ICONERROR);
    }

    const ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    bool bDone = false;

    while (!bDone)
    {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                bDone = true;
        }

        if (GetAsyncKeyState(VK_END) & 1)
            bDone = true;

        if (bDone)
            break;

        // Управление анимацией появления
        if (isFirstFrame)
        {
            startTime = std::chrono::steady_clock::now();
            currentAlpha = 0.0f;
            isFirstFrame = false;
        }

        // Обновление прозрачности
        auto currentTime = std::chrono::steady_clock::now();
        float elapsedTime = std::chrono::duration<float>(currentTime - startTime).count();
        currentAlpha = std::min(elapsedTime / FADE_DURATION, 1.0f);

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        {
            if (showConsole)
            {
                console.draw("Debug Console", &showConsole);
            }
            ImGui::GetStyle().Alpha = currentAlpha;
            Drawing::Draw(successIcon, errorIcon);
        }
        ImGui::EndFrame();

        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        pd3dDeviceContext->OMSetRenderTargets(1, &pMainRenderTargetView, nullptr);
        pd3dDeviceContext->ClearRenderTargetView(pMainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        pSwapChain->Present(1, 0);

        #ifndef _WINDLL
        if (!Drawing::isActive())
            break;
        #endif
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);
}