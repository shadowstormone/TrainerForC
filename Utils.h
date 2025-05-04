#pragma once
#include <string>

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
}