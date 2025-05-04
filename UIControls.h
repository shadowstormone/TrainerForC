#pragma once
#include <imgui.h>
#include <map>
#include <string>
#include "Utils.h"

namespace UIControls
{
    // Функция рендера переключателя с анимацией
    bool AnimatedToggleSwitch(const char* id, bool* v, const ImVec2& size = ImVec2(50, 25), float animationSpeed = 0.1f);

    // Константы для элементов интерфейса
    namespace Constants
    {
        constexpr int INPUT_WIDTH = 158;
        constexpr int MIN_VALUE = 1;
        constexpr float LABEL_WIDTH = 150.0f;
        constexpr float TEXT_WIDTH = 315.0f;
    }
}