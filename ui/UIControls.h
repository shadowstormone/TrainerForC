#pragma once
#include <imgui.h>
#include <string>

#include "platform/Utils.h"

namespace UIControls
{
    // Числовой регулятор: < значение >
    //
    // Стрелки меняют значение шагом step и повторяются при удержании,
    // само поле остаётся редактируемым — вводить 999999 стрелками никто
    // не станет. Тип значения — любой из ImGuiDataType (int, float, double).
    bool ValueStepper(const char* id, ImGuiDataType type, void* value, double step, float width);

    // Для int — прежняя форма с границами.
    bool ValueStepper(const char* id, int* value, int step, int minValue, int maxValue, float width);

    // Переключатель с анимацией.
    bool AnimatedToggleSwitch(const char* id, bool* v, const ImVec2& size = ImVec2(38, 18), float animationSpeed = 0.1f);

    // Кнопка того же размера, что и переключатель, — для разовых действий
    // («записать 9999»). lit — подсветить, пока действие «горит».
    bool ActionPill(const char* id, const char* label, bool lit, const ImVec2& size = ImVec2(38, 18));

    // Клавиша в рамке, как на клавиатуре: [Ctrl+Num 1].
    // width > 0 — ширина рамки (чтобы столбец был ровным).
    void KeyCap(const char* text, float width = 0.0f);

    // Круглый значок «!» — ошибка; подсказка с причиной при наведении.
    // Возвращает true, если на него навели.
    bool ErrorBadge(const char* id, const std::string& tooltip, float pulse = 0.0f);

    // Точка состояния с мягким свечением. pulse 0..1 — фаза пульсации.
    void StatusDot(const ImVec2& center, float radius, ImU32 color, float pulse = 0.0f);

    // Цвет между a и b.
    ImVec4 Mix(const ImVec4& a, const ImVec4& b, float t);

    // Сглаживание к цели, не зависящее от частоты кадров.
    float Approach(float current, float target, float speed);
}
