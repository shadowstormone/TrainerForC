#include "ui/UIControls.h"

#include <cmath>

bool UIControls::AnimatedToggleSwitch(const char* id, bool* v, const ImVec2& size, float animationSpeed)
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec2 p = ImGui::GetCursorScreenPos();
    float height = size.y;
    float width = size.x;
    float radius = height * 0.5f;

    // Прогресс анимации храним в ImGuiStorage по идентификатору элемента.
    // Раньше это была функциональная static-карта std::map<std::string,float>:
    // она выделяла строку на КАЖДЫЙ кадр ради поиска и росла без конца,
    // потому что записи из неё никогда не удалялись.
    const ImGuiID storageId = ImGui::GetID(id);
    ImGuiStorage* storage = ImGui::GetStateStorage();
    float animationProgress = storage->GetFloat(storageId, *v ? 1.0f : 0.0f);

    // Невидимая кнопка
    ImGui::InvisibleButton(id, size);
    bool clicked = ImGui::IsItemClicked();
    if (clicked)
    {
        *v = !*v;
    }

    // Плавная анимация, не зависящая от частоты кадров.
    //
    // Было: progress += (target - progress) * (speed * dt * 60). При
    // просадке кадров множитель вылетал за единицу — например, после паузы
    // в полсекунды он равен 3, — и анимация перелетала цель, после чего её
    // подрезал clamp. Экспоненциальное сглаживание 1 - e^(-k*dt) всегда
    // остаётся в пределах [0,1) и даёт одинаковую скорость на любом FPS.
    const float target = *v ? 1.0f : 0.0f;
    const float rate = animationSpeed * 60.0f; // скорость в единицах в секунду
    const float t = 1.0f - std::exp(-rate * ImGui::GetIO().DeltaTime);

    animationProgress += (target - animationProgress) * t;
    animationProgress = Utils::Clamp(animationProgress, 0.0f, 1.0f);

    storage->SetFloat(storageId, animationProgress);

    // Easing
    float easeFactor = animationProgress * animationProgress * (3.0f - 2.0f * animationProgress);

    // Цвет
    ImVec4 offColor = style.Colors[ImGuiCol_FrameBg];
    ImVec4 onColor = style.Colors[ImGuiCol_CheckMark];
    ImVec4 currentColor;
    currentColor.x = offColor.x + (onColor.x - offColor.x) * easeFactor;
    currentColor.y = offColor.y + (onColor.y - offColor.y) * easeFactor;
    currentColor.z = offColor.z + (onColor.z - offColor.z) * easeFactor;
    currentColor.w = 1.0f;

    // Фон переключателя
    draw_list->AddRectFilled(ImVec2(p.x, p.y), ImVec2(p.x + width, p.y + height), ImGui::ColorConvertFloat4ToU32(currentColor), radius);

    // Ползунок
    float circle_x = p.x + radius + easeFactor * (width - 2 * radius);
    ImVec4 circleColor = style.Colors[ImGuiCol_Button];
    ImVec4 shadowColor = style.Colors[ImGuiCol_Border];
    shadowColor.w = 0.5f;

    // Тень
    draw_list->AddCircleFilled(ImVec2(circle_x + 1.0f, p.y + radius + 1.0f), radius - 2.0f, ImGui::ColorConvertFloat4ToU32(shadowColor));

    // Ползунок
    draw_list->AddCircleFilled(ImVec2(circle_x, p.y + radius), radius - 2.0f, ImGui::ColorConvertFloat4ToU32(circleColor));

    // Наведение
    if (ImGui::IsItemHovered())
    {
        draw_list->AddCircleFilled(ImVec2(circle_x, p.y + radius), radius - 2.0f, ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_ButtonHovered]));
    }

    return clicked;
}