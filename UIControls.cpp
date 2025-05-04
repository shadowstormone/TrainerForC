#include "UIControls.h"

bool UIControls::AnimatedToggleSwitch(const char* id, bool* v, const ImVec2& size, float animationSpeed)
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec2 p = ImGui::GetCursorScreenPos();
    float height = size.y;
    float width = size.x;
    float radius = height * 0.5f;

    // Уникальные анимации для каждого переключателя
    static std::map<std::string, float> animationProgressMap;
    float& animationProgress = animationProgressMap[std::string(id)];

    // Невидимая кнопка
    ImGui::InvisibleButton(id, size);
    bool clicked = ImGui::IsItemClicked();
    if (clicked)
    {
        *v = !*v;
    }

    // Плавная анимация
    float targetProgress = *v ? 1.0f : 0.0f;
    animationProgress += (targetProgress - animationProgress) * (animationSpeed * ImGui::GetIO().DeltaTime * 60.0f);
    animationProgress = Utils::Clamp(animationProgress, 0.0f, 1.0f);

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