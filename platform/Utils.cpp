#include <thread>
#include <chrono>
#include <Windows.h>
#include "platform/Utils.h"
#include <codecvt>

std::string Utils::WStringToUtf8(const std::wstring& wstr)
{
    if (wstr.empty()) return {};

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size_needed <= 0)
    {
        return {};
    }

    std::string utf8str(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8str[0], size_needed, nullptr, nullptr);

    utf8str.pop_back();
    return utf8str;
}

std::string Utils::WStringToUtf8(LPCWSTR wstr)
{
    if (!wstr) return "(null)";
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
    std::string str(size_needed - 1, 0); // -1 чтобы убрать завершающий нуль
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &str[0], size_needed, nullptr, nullptr);
    return str;
}

std::wstring Utils::Utf8ToWString(const std::string& str)
{
    if (str.empty()) return {};

    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    if (size_needed <= 0) return {};

    std::wstring wstr(size_needed, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size_needed);

    // remove terminating null
    if (!wstr.empty() && wstr.back() == L'\0') wstr.pop_back();
    return wstr;
}

void* Utils::LoadResourceToMemory(UINT resourceID, DWORD& sizeOut)
{
    // Находим ресурс
    HRSRC hResource = FindResource(nullptr, MAKEINTRESOURCE(resourceID), RT_RCDATA);
    if (!hResource) return nullptr;

    HGLOBAL hGlobal = LoadResource(nullptr, hResource);
    if (!hGlobal) return nullptr;

    void* pData = LockResource(hGlobal);
    sizeOut = SizeofResource(nullptr, hResource);

    return pData; // Внимание: память остаётся в ресурсах, не освобождайте через delete!
}

ImFont* Utils::LoadFontFromResource(ImGuiIO& io, UINT resourceID, float fontSize, const ImFontConfig* fontConfig, const ImWchar* glyphRanges)
{
    DWORD dataSize = 0;
    void* pData = LoadResourceToMemory(resourceID, dataSize);
    if (!pData || dataSize == 0)
    {
        // Не показываем MessageBox в релизе, только логируем
#ifdef _DEBUG
        MessageBoxA(nullptr, "Failed to load font from resources.", "Font Error", MB_OK | MB_ICONERROR);
#endif
        return nullptr;
    }

    // Копируем данные, потому что ImGui ожидает, что память будет "принадлежать" ему
    void* pCopy = malloc(dataSize);
    if (!pCopy)
    {
#ifdef _DEBUG
        MessageBoxA(nullptr, "Out of memory.", "Font Error", MB_OK | MB_ICONERROR);
#endif
        return nullptr;
    }
    memcpy(pCopy, pData, dataSize);

    // Создаем копию конфигурации, если она передана
    ImFontConfig config;
    if (fontConfig)
    {
        config = *fontConfig;
    }
    else
    {
        // Устанавливаем базовые параметры
        config.SizePixels = fontSize;
        config.PixelSnapH = true;
        config.OversampleH = 1;
        config.OversampleV = 1;
        strcpy_s(config.Name, "Resource Font");
    }

    // ВАЖНО: Устанавливаем флаг, что память принадлежит ImGui
    config.FontDataOwnedByAtlas = true;

    // Загружаем шрифт из памяти
    ImFont* font = io.Fonts->AddFontFromMemoryTTF(pCopy, dataSize, fontSize, &config, glyphRanges);
    if (!font)
    {
        free(pCopy); // Если шрифт не загрузился — освободим память
#ifdef _DEBUG
        MessageBoxA(nullptr, "Failed to create font from memory.", "Font Error", MB_OK | MB_ICONERROR);
#endif
        return nullptr;
    }

    return font;
}

// std::function<void()> onFinish лямбда-обработчик завершения
void Utils::DelayedToggleOff(std::unordered_map<std::string, bool>& toggleStates, const std::string& toggleId, int delayMs, std::function<void()> onFinish)
{
    std::thread([&toggleStates, toggleId, delayMs, onFinish]()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
            toggleStates[toggleId] = false;
            if (onFinish) onFinish();
        }).detach();
}
