#pragma once
#include <string>
#include <unordered_map>
#include <functional>
#include <wtypes.h>
#include <imgui_impl_win32.h>

namespace Utils
{
    // Шаблон для ограничения значения в диапазоне
    template<typename T>
    inline T Clamp(T value, T min, T max)
    {
        if (value < min) return min;
        if (value > max) return max;
        return value;
    }

    // Конвертация wstring в UTF-8 строку
    std::string WStringToUtf8(const std::wstring& wstr);
    std::string WStringToUtf8(LPCWSTR wstr);
    std::wstring Utf8ToWString(const std::string& str);
    void* LoadResourceToMemory(UINT resourceID, DWORD& sizeOut);
    ImFont* LoadFontFromResource(ImGuiIO& io, UINT resourceID, float fontSize, const ImFontConfig* fontConfig = nullptr, const ImWchar* glyphRanges = nullptr);

}