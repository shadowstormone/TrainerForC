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

    // Быстрая хэш-функция для строк (компилируется во время компиляции)
    static constexpr unsigned int hash(const char* str)
    {
        unsigned int hash = 5381;
        while (*str)
        {
            hash = ((hash << 5) + hash) + (*str++);
        }
        return hash;
    }

    // Конвертация wstring в UTF-8 строку
    std::string WStringToUtf8(const std::wstring& wstr);
    std::string WStringToUtf8(LPCWSTR wstr);
    void* LoadResourceToMemory(UINT resourceID, DWORD& sizeOut);
    ImFont* LoadFontFromResource(ImGuiIO& io, UINT resourceID, float fontSize, const ImFontConfig* fontConfig = nullptr, const ImWchar* glyphRanges = nullptr);

    //void TemporaryToggleOff(std::unordered_map<std::string, bool>& toggleMap, const std::string& toggleId,int delayMs = 250, std::function<void()> onFinish);
    void DelayedToggleOff(std::unordered_map<std::string, bool>& toggleStates, const std::string& toggleId, int delayMs, std::function<void()> onFinish);
}