#include <thread>
#include <chrono>
#include <Windows.h>
#include "Utils.h"
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
