#pragma once
#include <imgui.h>
#include <map>
#include <string>
#include "platform/Utils.h"

namespace UIControls
{
    // Числовой регулятор: < значение >
    //
    // Стрелки меняют значение шагом step и повторяются при удержании,
    // само поле остаётся редактируемым — вводить 999999 стрелками никто
    // не станет.
    bool ValueStepper(const char* id, int* value, int step, int minValue, int maxValue, float width);

    // Функция рендера переключателя с анимацией
    bool AnimatedToggleSwitch(const char* id, bool* v, const ImVec2& size = ImVec2(38, 18), float animationSpeed = 0.1f);

    // Константы для элементов интерфейса
    namespace Constants
    {
        constexpr int INPUT_WIDTH = 158;
        constexpr int MIN_VALUE = 1;
        constexpr float LABEL_WIDTH = 150.0f;
        constexpr float TEXT_WIDTH = 315.0f;

        // Должна совпадать со значением по умолчанию у AnimatedToggleSwitch.
        constexpr float TOGGLE_WIDTH = 50.0f;
    }
}