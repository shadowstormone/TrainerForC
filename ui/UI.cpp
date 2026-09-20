/**
 * @file UI.cpp
 * @brief Основной файл пользовательского интерфейса для Test Trainer
 * @details Содержит реализацию класса UI и вспомогательных классов для работы с DirectX 11, ImGui и системными ресурсами
 */

#define NOMINMAX
#include "resource.h"
#include "ui/UI.h"
#include "ui/D3DContext.h"
#include "ui/ImGuiThemes.h"
#include "ui/ImGuiConsole.h"
#include "ui/Window.h"
#include <imgui_internal.h>
#include <shlobj.h>
#include <KnownFolders.h>
#include <filesystem>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif

// Работа с DirectX 11 вынесена в класс D3DContext (ui/D3DContext.h),
// а создание окна и диспетчеризация сообщений — в класс Window (ui/Window.h).

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

        // Окно и контекст DirectX — обычные объекты: освободятся сами
        // при выходе из функции, в том числе при исключении.
        Window window;
        D3DContext d3d;

        const HINSTANCE instance = GetModuleHandle(nullptr);

        WindowDesc desc;
        desc.className = L"TestTrainerWindow";
        desc.title = L"Test Trainer";
        desc.x = posX;
        desc.y = posY;
        desc.width = WIDTH;
        desc.height = HEIGHT;
        desc.icon = LoadIcon(instance, MAKEINTRESOURCE(IDI_ICON2));
        desc.iconSmall = LoadIcon(instance, MAKEINTRESOURCE(IDI_ICON1));
        desc.borderless = true;      // системный заголовок убран, свой рисует ImGui
        desc.resizable = false;      // фиксированный размер
        desc.roundedCorners = true;  // скруглённые углы (Windows 11)

        if (!window.Create(desc))
        {
            throw std::runtime_error("Failed to create window");
        }

        // Кнопки заголовка управляют этим окном
        Drawing::SetWindowHandle(window.Handle());

        // Перетаскивание за полосу заголовка; над кнопками — обычные клики
        window.SetCaptionHitTest([](POINT pt) { return Drawing::IsCaptionPoint(pt); });

        if (!d3d.Create(window.Handle()))
        {
            throw std::runtime_error("Failed to create D3D device");
        }

        // Обработка сообщений окна: сначала ImGui, затем ресайз D3D и DPI.
        window.SetMessageHandler(
            [&d3d](HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& result) -> bool
            {
                if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
                {
                    result = 1;
                    return true;
                }

                switch (msg)
                {
                case WM_SIZE:
                    if (d3d.IsValid() && wParam != SIZE_MINIMIZED)
                    {
                        d3d.Resize(LOWORD(lParam), HIWORD(lParam));
                    }
                    result = 0;
                    return true;

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

                return false; // остальное — стандартная обработка в Window
            });

        window.Show();

        // Инициализация ImGui
		IMGUI_CHECKVERSION();       // Проверка версии ImGui
		ImGui::CreateContext();     // Создание контекста ImGui
		ImGuiIO& io = ImGui::GetIO();       // Получение объекта ImGuiIO
        // Viewports отключены намеренно: интерфейс живёт в одном настоящем
        // окне, у которого есть кнопка в таскбаре, своя иконка и нормальный
        // фокус. Раньше видимым окном был ImGui-viewport (tool window), из-за
        // чего и понадобились topmost/автоклик/раздача иконок вьюпортам.
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.IniFilename = nullptr;       // Отключаем сохранение настроек в ini-файл

		SetModernDarkStyle(); // Установка стиля интерфейса

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

		ImGui_ImplWin32_Init(window.Handle()); // Инициализация ImGui для Win32
		ImGui_ImplDX11_Init(d3d.Device(), d3d.Context()); // Инициализация ImGui для DirectX 11

        // ImGui 1.92: шрифт по умолчанию задаётся через io.FontDefault в SetupFont;
        // ручное обновление контекста шрифта больше не требуется.
        (void)font;

		ImGui_ImplDX11_InvalidateDeviceObjects(); // Очистка объектов устройства ImGui
		ImGui_ImplDX11_CreateDeviceObjects(); // Создание объектов устройства ImGui

        // Загрузка текстур
        TextureManager textureManager;
        if (!textureManager.LoadTextures(d3d.Device(), d3d.Context(), GetModuleHandle(nullptr)))
        {
            MessageBoxA(nullptr, "Failed to load one or more textures.", "Texture Load Error", MB_OK | MB_ICONERROR);
        }

		RenderLoop(window, d3d, io, textureManager); // Запуск основного цикла рендеринга

        // Очистка ресурсов ImGui; окно и D3D освободят себя сами (RAII).
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
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
void UI::RenderLoop(Window& window, D3DContext& d3d, ImGuiIO& io, TextureManager& textureManager)
{
    const ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    bool done = false;

    // Флаги для отслеживания состояния окна
    static int framesRendered = 0;

    while (!done)
    {
        // Обработка сообщений
        if (!window.PumpMessages())
        {
            done = true;
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

        d3d.BeginFrame(clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        d3d.Present(1);

        // Увеличиваем счетчик кадров
        framesRendered++;

#ifndef _WINDLL
        if (!Drawing::isActive())
        {
            break;
        }
#endif
    }
}
