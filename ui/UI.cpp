/**
 * @file UI.cpp
 * @brief Основной файл пользовательского интерфейса для Test Trainer
 * @details Содержит реализацию класса UI и вспомогательных классов для работы с DirectX 11, ImGui и системными ресурсами
 */

#define NOMINMAX
#include "resource.h"
#include "ui/UI.h"
#include "ui/ImGuiThemes.h"
#include "ui/ImGuiConsole.h"
#include <imgui_internal.h>
#include <shlobj.h>
#include <KnownFolders.h>
#include <filesystem>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

 /// @brief Статические члены класса UI для работы с DirectX 11
ID3D11Device* UI::pd3dDevice = nullptr;
ID3D11DeviceContext* UI::pd3dDeviceContext = nullptr;
IDXGISwapChain* UI::pSwapChain = nullptr;
ID3D11RenderTargetView* UI::pMainRenderTargetView = nullptr;
HMODULE UI::hCurrentModule = nullptr;
HWND hwnd;

/**
 * @brief Загружает текстуру из ресурсов приложения
 * @param device Указатель на устройство DirectX 11
 * @param context Указатель на контекст устройства DirectX 11
 * @param hModule Модуль приложения
 * @param resourceId Идентификатор ресурса
 * @param resourceType Тип ресурса (например, "PNG")
 * @return Указатель на шейдерный ресурс представления или nullptr в случае ошибки
 */
static ID3D11ShaderResourceView* LoadTextureFromResource(ID3D11Device* device, ID3D11DeviceContext* context,
    HMODULE hModule, int resourceId, const char* resourceType)
{
    // Конвертация ANSI строки в Unicode
    wchar_t wResourceType[64];
    if (MultiByteToWideChar(CP_UTF8, 0, resourceType, -1, wResourceType, sizeof(wResourceType) / sizeof(wResourceType[0])) == 0)
    {
        return nullptr;
    }

    // Поиск и загрузка ресурса
    HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(resourceId), wResourceType);
    if (!hResource)
    {
        return nullptr;
    }

    HGLOBAL hLoadedResource = LoadResource(hModule, hResource);
    if (!hLoadedResource)
    {
        return nullptr;
    }

    void* pResourceData = LockResource(hLoadedResource);
    DWORD resourceSize = SizeofResource(hModule, hResource);
    if (!pResourceData || resourceSize == 0)
    {
        return nullptr;
    }

    // Загрузка изображения с помощью stb_image
    int width, height, channels;
    unsigned char* imageData = stbi_load_from_memory(static_cast<unsigned char*>(pResourceData), resourceSize, &width, &height, &channels, 4);

    if (!imageData)
    {
        return nullptr;
    }

    // Создание DirectX текстуры
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

/**
 * @namespace UIConstants
 * @brief Константы пользовательского интерфейса
 */
namespace UIConstants
{
    constexpr float FADE_DURATION = 1.5f;           ///< Продолжительность анимации затухания в секундах
    constexpr float DEFAULT_FONT_SIZE = 13.0f;      ///< Размер шрифта по умолчанию
    constexpr UINT BUFFER_COUNT = 2;                ///< Количество буферов обмена
    constexpr UINT REFRESH_RATE = 60;               ///< Частота обновления экрана

    /**
     * @struct ResolutionScale
     * @brief Структура для определения масштаба интерфейса в зависимости от разрешения
     */
    constexpr struct ResolutionScale
    {
        int height;     ///< Высота разрешения экрана
        float scale;    ///< Коэффициент масштабирования
    } RESOLUTION_SCALES[] = {
        {2160, 2.1f}, // 4K (Ultra HD)
        {1440, 1.9f}, // 2K (QHD)
        {1200, 1.7f}, // WUXGA
        {1080, 1.5f}, // Full HD
        {900,  1.3f}, // HD+
        {0,    1.1f}  // По умолчанию для низких разрешений
    };
}

namespace
{
    /**
     * @struct FadeAnimation
     * @brief Структура для управления анимацией затухания
     */
    struct FadeAnimation
    {
        float currentAlpha = 0.0f; ///< Текущий уровень прозрачности
        bool isFirstFrame = true; ///< Флаг первого кадра
        std::chrono::steady_clock::time_point startTime; ///< Время начала анимации

        /**
         * @brief Запускает анимацию затухания
         */
        void Start()
        {
            if (isFirstFrame)
            {
                startTime = std::chrono::steady_clock::now();
                currentAlpha = 0.0f;
                isFirstFrame = false;
            }
        }

        /**
         * @brief Обновляет состояние анимации
         */
        void Update()
        {
            auto currentTime = std::chrono::steady_clock::now();
            float elapsedTime = std::chrono::duration<float>(currentTime - startTime).count();
            currentAlpha = std::min(elapsedTime / UIConstants::FADE_DURATION, 1.0f);
        }

        /**
         * @brief Получает текущий уровень прозрачности
         * @return Значение альфа-канала от 0.0 до 1.0
         */
        float getAlpha() const { return currentAlpha; }
    };

    FadeAnimation fadeAnimation; ///< Глобальный объект анимации затухания
}

/**
 * @class TextureManager
 * @brief Класс для управления текстурами интерфейса
 */
class TextureManager
{
private:
    /// @brief Умный указатель на текстуру иконки успеха
    std::unique_ptr<ID3D11ShaderResourceView, void(*)(ID3D11ShaderResourceView*)> successIcon;
    /// @brief Умный указатель на текстуру иконки ошибки
    std::unique_ptr<ID3D11ShaderResourceView, void(*)(ID3D11ShaderResourceView*)> errorIcon;

    /**
     * @brief Освобождает ресурсы текстуры
     * @param texture Указатель на текстуру для освобождения
     */
    static void ReleaseTexture(ID3D11ShaderResourceView* texture)
    {
        if (texture) texture->Release();
    }

public:
    /**
     * @brief Конструктор по умолчанию
     */
    TextureManager() : successIcon(nullptr, ReleaseTexture), errorIcon(nullptr, ReleaseTexture) {}

    /**
     * @brief Загружает текстуры из ресурсов
     * @param device Указатель на устройство DirectX 11
     * @param context Указатель на контекст устройства DirectX 11
     * @param hModule Модуль приложения
     * @return true в случае успешной загрузки, false при ошибке
     */
    bool LoadTextures(ID3D11Device* device, ID3D11DeviceContext* context, HMODULE hModule)
    {
        auto success = LoadTextureFromResource(device, context, hModule, IDB_PNG1, "PNG");
        auto error = LoadTextureFromResource(device, context, hModule, IDB_PNG2, "PNG");

        if (!success || !error)
        {
            return false;
        }

        successIcon.reset(success);
        errorIcon.reset(error);
        return true;
    }

    /**
     * @brief Получает указатель на иконку успеха
     * @return Указатель на текстуру иконки успеха
     */
    ID3D11ShaderResourceView* getSuccessIcon() const { return successIcon.get(); }

    /**
     * @brief Получает указатель на иконку ошибки
     * @return Указатель на текстуру иконки ошибки
     */
    ID3D11ShaderResourceView* getErrorIcon() const { return errorIcon.get(); }
};

/**
 * @class FontManager
 * @brief Класс для управления шрифтами интерфейса
 */
class FontManager
{
private:
    /**
     * @brief Загружает шрифт из ресурсов
     * @param io Объект ImGuiIO
     * @param resourceId Идентификатор ресурса шрифта
     * @param fontSize Размер шрифта
     * @param ranges Диапазоны символов Unicode
     * @param name Имя шрифта
     * @return Указатель на загруженный шрифт или nullptr
     */
    static ImFont* LoadFontFromResource(ImGuiIO& io, int resourceId, float fontSize,
        const ImWchar* ranges, const char* name)
    {
        ImFontConfig config;
        config.SizePixels = fontSize;
        config.PixelSnapH = true;
        config.OversampleH = 1;
        config.OversampleV = 1;
        strcpy_s(config.Name, name);

        return Utils::LoadFontFromResource(io, resourceId, fontSize, &config, ranges);
    }

    /**
     * @brief Загружает шрифт из файла
     * @param io Объект ImGuiIO
     * @param filepath Путь к файлу шрифта
     * @param fontSize Размер шрифта
     * @param ranges Диапазоны символов Unicode
     * @param name Имя шрифта
     * @return Указатель на загруженный шрифт или nullptr
     */
    static ImFont* LoadFontFromFile(ImGuiIO& io, const char* filepath, float fontSize,
        const ImWchar* ranges, const char* name)
    {
        if (!std::filesystem::exists(filepath))
        {
            return nullptr;
        }

        ImFontConfig config;
        config.SizePixels = fontSize;
        config.PixelSnapH = true;
        config.OversampleH = 1;
        config.OversampleV = 1;
        strcpy_s(config.Name, name);

        return io.Fonts->AddFontFromFileTTF(filepath, fontSize, &config, ranges);
    }

    /**
     * @brief Загружает шрифт по умолчанию
     * @param io Объект ImGuiIO
     * @param name Имя шрифта
     * @return Указатель на шрифт по умолчанию
     */
    static ImFont* LoadDefaultFont(ImGuiIO& io, const char* name)
    {
        ImFontConfig config;
        strcpy_s(config.Name, name);
        return io.Fonts->AddFontDefault(&config);
    }

public:
    /**
     * @brief Настраивает шрифт с учетом масштабирования
     * @param io Объект ImGuiIO
     * @param scale Коэффициент масштабирования
     * @return Указатель на настроенный шрифт
     */
    static ImFont* SetupFont(ImGuiIO& io, float scale)
    {
        static const ImWchar ranges[] = {
            0x0020, 0x00FF, // Basic Latin + Latin Supplement
            0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
            0x2DE0, 0x2DFF, // Cyrillic Extended-A
            0xA640, 0xA69F, // Cyrillic Extended-B
            0
        };

        const float fontSize = UIConstants::DEFAULT_FONT_SIZE * scale;
        io.Fonts->Clear();

        // Попытка загрузить шрифт из ресурсов
        ImFont* font = LoadFontFromResource(io, IDR_RCDATA1, fontSize, ranges, "Friz Quadrata TT Resource");

        // Резервный вариант - системный шрифт
        if (!font)
        {
            font = LoadFontFromFile(io, "C:\\Windows\\Fonts\\arial.ttf", fontSize, ranges, "Arial Fallback");
        }

        // Последний резерв - шрифт по умолчанию
        if (!font)
        {
            font = LoadDefaultFont(io, "ImGui Default");
        }

        if (font)
        {
            io.FontDefault = font;
            ApplyFontToContext(font, io);
        }

        return font;
    }

private:
    /**
     * @brief Применяет шрифт к контексту ImGui
     * @param font Указатель на шрифт
     * @param io Объект ImGuiIO
     */
    static void ApplyFontToContext(ImFont* font, ImGuiIO& io)
    {
        // ImGui 1.92: шрифты стали масштабируемыми, поля ImFont::FontSize и
        // ImGuiContext::FontBaseSize удалены. Шрифт по умолчанию задаётся через
        // io.FontDefault (см. SetupFont) и применяется ImGui автоматически.
        (void)font;
        (void)io;
    }
};

/**
 * @class DisplayManager
 * @brief Класс для управления параметрами дисплея
 */
class DisplayManager
{
public:
    /**
     * @struct DisplayInfo
     * @brief Структура с информацией о дисплее
     */
    struct DisplayInfo
    {
        int width;     ///< Ширина экрана
        int height;    ///< Высота экрана
        int centerX;   ///< Центр X
        int centerY;   ///< Центр Y
        float scale;   ///< Масштаб интерфейса
    };

    /**
     * @brief Получает информацию о дисплее
     * @return Структура с параметрами дисплея
     */
    static DisplayInfo getDisplayInfo()
    {
        HMONITOR hMonitor = MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
        MONITORINFO mi = {};
        mi.cbSize = sizeof(mi);

        DisplayInfo info = {};

        if (GetMonitorInfo(hMonitor, &mi))
        {
            info.width = mi.rcMonitor.right - mi.rcMonitor.left;
            info.height = mi.rcMonitor.bottom - mi.rcMonitor.top;
            info.centerX = mi.rcMonitor.left + info.width / 2;
            info.centerY = mi.rcMonitor.top + info.height / 2;
            info.scale = CalculateScale(info.height);
        }

        return info;
    }

private:
    /**
     * @brief Вычисляет коэффициент масштабирования на основе высоты экрана
     * @param height Высота экрана в пикселях
     * @return Коэффициент масштабирования
     */
    static float CalculateScale(int height)
    {
        for (const auto& rs : UIConstants::RESOLUTION_SCALES)
        {
            if (height >= rs.height)
            {
                return rs.scale;
            }
        }
        return UIConstants::RESOLUTION_SCALES[
            sizeof(UIConstants::RESOLUTION_SCALES) / sizeof(UIConstants::RESOLUTION_SCALES[0]) - 1
        ].scale;
    }
};

/**
 * @brief Создает устройство DirectX 11 и цепочку обмена
 * @param hWnd Дескриптор окна
 * @return true в случае успеха, false при ошибке
 */
bool UI::CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = UIConstants::BUFFER_COUNT;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = UIConstants::REFRESH_RATE;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    const D3D_FEATURE_LEVEL featureLevelArray[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    D3D_FEATURE_LEVEL featureLevel;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        featureLevelArray, ARRAYSIZE(featureLevelArray), D3D11_SDK_VERSION,
        &sd, &pSwapChain, &pd3dDevice, &featureLevel, &pd3dDeviceContext);

    if (FAILED(hr))
    {
        return false;
    }

    CreateRenderTarget();
    return true;
}

/**
 * @brief Создает цель рендеринга
 */
void UI::CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer = nullptr;
    if (SUCCEEDED(pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer))) && pBackBuffer)
    {
        pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &pMainRenderTargetView);
        pBackBuffer->Release();
    }
}

/**
 * @brief Очищает цель рендеринга
 */
void UI::CleanupRenderTarget()
{
    if (pMainRenderTargetView)
    {
        pMainRenderTargetView->Release();
        pMainRenderTargetView = nullptr;
    }
}

/**
 * @brief Очищает ресурсы DirectX 11
 */
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
#define WM_DPICHANGED 0x02E0
#endif

/**
 * @brief Процедура обработки сообщений окна
 * @param hWnd Дескриптор окна
 * @param msg Сообщение
 * @param wParam Параметр сообщения
 * @param lParam Параметр сообщения
 * @return Результат обработки сообщения
 */
LRESULT WINAPI UI::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
    {
        return true;
    }

    switch (msg)
    {
    case WM_SIZE:
        if (pd3dDevice && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            pSwapChain->ResizeBuffers(0, LOWORD(lParam), HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;

    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
        {
            return 0;
        }
        break;

    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;

    case WM_DPICHANGED:
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DpiEnableScaleViewports)
        {
            const RECT* suggested_rect = reinterpret_cast<const RECT*>(lParam);
            ::SetWindowPos(hWnd, nullptr,
                suggested_rect->left, suggested_rect->top,
                suggested_rect->right - suggested_rect->left,
                suggested_rect->bottom - suggested_rect->top,
                SWP_NOZORDER | SWP_NOACTIVATE);
        }
        break;
    }

    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}

/**
 * @brief Получает путь к шрифту в системной папке
 * @return Строка с путем к файлу шрифта
 */
std::string UI::getFontPath()
{
    PWSTR path = nullptr;
    std::string fontPath;

    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &path)))
    {
        // Конвертация WCHAR* в string
        int size = WideCharToMultiByte(CP_UTF8, 0, path, -1, nullptr, 0, nullptr, nullptr);
        if (size > 0)
        {
            std::string localAppData(size - 1, 0); // -1 чтобы исключить null terminator
            WideCharToMultiByte(CP_UTF8, 0, path, -1, &localAppData[0], size, nullptr, nullptr);
            fontPath = localAppData + "\\Microsoft\\Windows\\Fonts\\FRIZQT.ttf";
        }
        CoTaskMemFree(path);
    }

    return fontPath;
}

/**
 * @brief Получает центр экрана
 * @return Пара координат центра экрана (X, Y)
 */
std::pair<int, int> UI::getScreenCenter()
{
    auto displayInfo = DisplayManager::getDisplayInfo();
    return { displayInfo.centerX, displayInfo.centerY };
}

/**
 * @brief Основная функция рендеринга интерфейса
 * @details Инициализирует окно, DirectX 11, ImGui и запускает основной цикл рендеринга
 */
void UI::Render()
{
    try
    {
        auto displayInfo = DisplayManager::getDisplayInfo();

        // Вычисление позиции окна для центрирования на экране
        const int posX = displayInfo.centerX - WIDTH / 2;
        const int posY = displayInfo.centerY - HEIGHT / 2;

        // Инициализация окна
        ImGui_ImplWin32_EnableDpiAwareness();

        static WNDCLASSEX wc = {
			sizeof(WNDCLASSEX),         // Размер структуры
			CS_CLASSDC,                 // Стиль класса окна
			WndProc,                    // Процедура обработки сообщений
			0L,                         // Дополнительные параметры класса
			0L,                         // Размер класса
			GetModuleHandle(nullptr),   // Дескриптор модуля
			nullptr,                    // Иконка класса (nullptr для системной иконки)
			nullptr,                    // Курсор класса (nullptr для системного курсора)
			nullptr,                    // Фон класса (nullptr для системного фона)
			nullptr,                    // Меню класса (nullptr для отсутствия меню)
			_T("Test Trainer"),         // Имя класса окна
			nullptr                     // Стиль класса (nullptr для системного стиля)
        };
        
        ::RegisterClassEx(&wc);

        hwnd = ::CreateWindow(
			wc.lpszClassName,       // Имя класса окна
			_T("Test Trainer"),     // Заголовок окна
			WS_OVERLAPPEDWINDOW,    // Стиль окна
			posX,                   // Позиция X окна
			posY,                   // Позиция Y окна
			50,                     // Ширина окна
			50,                     // Высота окна
			NULL,                   // Родительское окно
			NULL,                   // Меню окна
			wc.hInstance,           // Дескриптор экземпляра приложения
			NULL                    // Дополнительные параметры
        );

        if (!hwnd)
        {
            throw std::runtime_error("Failed to create window");
        }

		MakeWindowTopMostTemporary(hwnd, 5000); // Устанавливаем окно поверх других на 5 секунд

        if (!CreateDeviceD3D(hwnd))
        {
            CleanupDeviceD3D();
            ::UnregisterClass(wc.lpszClassName, wc.hInstance);
            throw std::runtime_error("Failed to create D3D device");
        }

        ::ShowWindow(hwnd, SW_HIDE);
        ::UpdateWindow(hwnd);

        // Инициализация ImGui
		IMGUI_CHECKVERSION();       // Проверка версии ImGui
		ImGui::CreateContext();     // Создание контекста ImGui
		ImGuiIO& io = ImGui::GetIO();       // Получение объекта ImGuiIO
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_ViewportsEnable;
		io.IniFilename = nullptr;       // Отключаем сохранение настроек в ini-файл

		SetModernDarkStyle(); // Установка стиля интерфейса

        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 4.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

#ifdef _DEBUG
        char message[128];
        sprintf_s(message, "Screen Resolution:\nWidth: %d\nHeight: %d",
            displayInfo.width, displayInfo.height);
        MessageBoxA(nullptr, message, "Screen Resolution Info", MB_OK | MB_ICONINFORMATION);
#endif // _DEBUG

        // Настройка шрифта
        ImFont* font = FontManager::SetupFont(io, displayInfo.scale);
        if (!font)
        {
#ifdef _DEBUG
            MessageBoxA(nullptr, "Unable to load any font.", "Font Load Error", MB_OK | MB_ICONERROR);
#else
            gConsole->addLog("ERROR", "Unable to load any font.");
#endif
        }

		ImGui_ImplWin32_Init(hwnd); // Инициализация ImGui для Win32
		ImGui_ImplDX11_Init(pd3dDevice, pd3dDeviceContext); // Инициализация ImGui для DirectX 11

        // ImGui 1.92: шрифт по умолчанию задаётся через io.FontDefault в SetupFont;
        // ручное обновление контекста шрифта больше не требуется.
        (void)font;

		ImGui_ImplDX11_InvalidateDeviceObjects(); // Очистка объектов устройства ImGui
		ImGui_ImplDX11_CreateDeviceObjects(); // Создание объектов устройства ImGui

        // Загрузка текстур
        TextureManager textureManager;
        if (!textureManager.LoadTextures(pd3dDevice, pd3dDeviceContext, GetModuleHandle(nullptr)))
        {
            MessageBoxA(nullptr, "Failed to load one or more textures.", "Texture Load Error", MB_OK | MB_ICONERROR);
        }

		RenderLoop(io, textureManager); // Запуск основного цикла рендеринга

        // Очистка ресурсов
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        CleanupDeviceD3D();
        ::DestroyWindow(hwnd);
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);

    }
    catch (const std::exception& e)
    {
#ifdef _DEBUG
        MessageBoxA(nullptr, e.what(), "Error", MB_OK | MB_ICONERROR);
#else
        if (gConsole) gConsole->addLog("FATAL", std::string("UI::Render exception: ") + e.what());
#endif
    }
}

/**
 * @brief Основной цикл рендеринга
 * @param io Объект ImGuiIO для управления вводом/выводом
 * @param textureManager Менеджер текстур
 */
void UI::RenderLoop(ImGuiIO& io, TextureManager& textureManager)
{
    const ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    bool done = false;

    // Флаги для отслеживания состояния окна
    static bool windowFullyCreated = false;
    static bool autoClickPerformed = false;
    static int framesRendered = 0;

    while (!done)
    {
        // Обработка сообщений
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
            {
                done = true;
            }
        }

        // Проверка клавиши выхода
        if (GetAsyncKeyState(VK_END) & 1)
        {
            done = true;
        }
        if (done) break;

        // Обновление анимации затухания
        fadeAnimation.Start();
        fadeAnimation.Update();

        // Рендеринг кадра
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Отрисовка элементов интерфейса
        if (showConsole)
        {
            gConsole->draw("Debug Console", &showConsole);
        }

        ImGui::GetStyle().Alpha = fadeAnimation.getAlpha();
        Drawing::Draw(textureManager.getSuccessIcon(), textureManager.getErrorIcon());

        ImGui::EndFrame();
        ImGui::Render();

        // Present frame
        const float clear_color_with_alpha[4] = {
            clear_color.x * clear_color.w, clear_color.y * clear_color.w,
            clear_color.z * clear_color.w, clear_color.w
        };

        pd3dDeviceContext->OMSetRenderTargets(1, &pMainRenderTargetView, nullptr);
        pd3dDeviceContext->ClearRenderTargetView(pMainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
			ApplyIconsToAllViewports(GetModuleHandle(nullptr));
            ImGui::RenderPlatformWindowsDefault();

#ifdef _DEBUG
            if (gConsole && framesRendered < 10)
            {
                ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
                int viewport_count = platform_io.Viewports.Size;
                char debug_msg[128];
                sprintf_s(debug_msg, "Viewport count: %d", viewport_count);
                gConsole->addLog("INFO", debug_msg);
            }
#endif // _DEBUG
        }

        pSwapChain->Present(1, 0);

        // Увеличиваем счетчик кадров
        framesRendered++;

        if (framesRendered >= 3 && !autoClickPerformed)
        {
            PerformAutoClick();
            autoClickPerformed = true;
        }

#ifndef _WINDLL
        if (!Drawing::isActive())
        {
            break;
        }
#endif
    }
}

void UI::MakeWindowTopMostTemporary(HWND hWnd, int milliseconds)
{
    // Делаем окно всегда поверх всех
    SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    // Запускаем отдельный поток, который через заданное время снимет TopMost
    std::thread([hWnd, milliseconds]()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
            SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }).detach();
}

void UI::PerformAutoClick()
{
    if (hwnd != nullptr)
    {
        auto displayInfo = DisplayManager::getDisplayInfo();
        POINT originalPos;
        GetCursorPos(&originalPos); // Сохраняем текущую позицию

        // Вычисление позиции окна для центрирования на экране
        const int posX = displayInfo.centerX - WIDTH / 5;
        const int posY = displayInfo.centerY - HEIGHT / 5;

        // Устанавливаем курсор в центр окна
        SetCursorPos(posX, posY);

        // Выполняем клик левой кнопкой мыши
        mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
        mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);

        SetCursorPos(originalPos.x, originalPos.y); // Восстанавливаем позицию
    }
}

void UI::ApplyIconsToAllViewports(HINSTANCE hInstance)
{
    static HICON hIcon = nullptr;
    static HICON hIconSm = nullptr;

    if (!hIcon)
    {
        hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON2));
    }

    if (!hIconSm)
    {
        hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    }

    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();

    for (ImGuiViewport* viewport : platform_io.Viewports)
    {
        if (viewport->PlatformHandle)
        {
            HWND hwnd = (HWND)viewport->PlatformHandle;
            if (hIcon)
            {
                SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            }
            else if (gConsole)
            {
                gConsole->addLog("ERROR", "Failed to load big icon for viewport");
            }
            if (hIconSm)
            {
                SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconSm);
            }
            else if (gConsole)
            {
                gConsole->addLog("ERROR", "Failed to load small icon for viewport");
            }
        }
    }
}
