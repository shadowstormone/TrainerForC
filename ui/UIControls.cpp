#include "ui/UIControls.h"

#include <algorithm>
#include <cmath>

#include <imgui_internal.h> // GetItemFlags: рисовать отключённое состояние

ImVec4 UIControls::Mix(const ImVec4& a, const ImVec4& b, float t)
{
    return ImVec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t,
                  a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t);
}

float UIControls::Approach(float current, float target, float speed)
{
    // Экспоненциальное сглаживание 1 - e^(-k*dt): всегда в пределах [0,1)
    // и одинаковая скорость на любом FPS.
    const float t = 1.0f - std::exp(-speed * ImGui::GetIO().DeltaTime);
    return current + (target - current) * t;
}

bool UIControls::AnimatedToggleSwitch(const char* id, bool* v, const ImVec2& size, float animationSpeed)
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImGuiStyle& style = ImGui::GetStyle();
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float height = size.y;
    const float width = size.x;
    const float radius = height * 0.5f;

    // Прогресс анимации храним в ImGuiStorage по идентификатору элемента.
    // Раньше это была функциональная static-карта std::map<std::string,float>:
    // она выделяла строку на КАЖДЫЙ кадр ради поиска и росла без конца,
    // потому что записи из неё никогда не удалялись.
    const ImGuiID storageId = ImGui::GetID(id);
    ImGuiStorage* storage = ImGui::GetStateStorage();
    float progress = storage->GetFloat(storageId, *v ? 1.0f : 0.0f);

    ImGui::InvisibleButton(id, size);
    const bool clicked = ImGui::IsItemClicked();
    if (clicked)
    {
        *v = !*v;
    }

    const bool hovered = ImGui::IsItemHovered();
    const bool disabled = (ImGui::GetItemFlags() & ImGuiItemFlags_Disabled) != 0;

    progress = Approach(progress, *v ? 1.0f : 0.0f, animationSpeed * 60.0f);
    progress = Utils::Clamp(progress, 0.0f, 1.0f);
    storage->SetFloat(storageId, progress);

    const float ease = progress * progress * (3.0f - 2.0f * progress);

    // Выключенный — заметная серая дорожка с рамкой (раньше сливалась с
    // фоном), включённый — акцентный цвет.
    ImVec4 offColor = style.Colors[ImGuiCol_FrameBgHovered];
    ImVec4 onColor = style.Colors[ImGuiCol_CheckMark];
    if (hovered && !disabled)
    {
        offColor = Mix(offColor, ImVec4(1, 1, 1, 1), 0.08f);
        onColor = Mix(onColor, ImVec4(1, 1, 1, 1), 0.10f);
    }

    ImVec4 track = Mix(offColor, onColor, ease);
    if (disabled) track.w *= 0.5f;

    draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), ImGui::ColorConvertFloat4ToU32(track), radius);
    draw_list->AddRect(p, ImVec2(p.x + width, p.y + height),
                       ImGui::ColorConvertFloat4ToU32(ImVec4(1, 1, 1, 0.06f * (1.0f - ease))), radius);

    // Ползунок
    const float knobRadius = radius - 2.5f;
    const float cx = p.x + radius + ease * (width - 2.0f * radius);
    const float cy = p.y + radius;

    draw_list->AddCircleFilled(ImVec2(cx, cy + 1.0f), knobRadius, IM_COL32(0, 0, 0, disabled ? 40 : 90));
    draw_list->AddCircleFilled(ImVec2(cx, cy), knobRadius,
                               ImGui::ColorConvertFloat4ToU32(Mix(ImVec4(0.78f, 0.80f, 0.84f, 1.0f),
                                                                  ImVec4(1, 1, 1, 1), ease)));
    return clicked;
}

bool UIControls::ActionPill(const char* id, const char* label, bool lit, const ImVec2& size)
{
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImGuiStyle& style = ImGui::GetStyle();
    const ImVec2 p = ImGui::GetCursorScreenPos();

    const ImGuiID storageId = ImGui::GetID(id);
    ImGuiStorage* storage = ImGui::GetStateStorage();
    float glow = storage->GetFloat(storageId, 0.0f);

    ImGui::InvisibleButton(id, size);
    const bool clicked = ImGui::IsItemClicked();
    const bool hovered = ImGui::IsItemHovered();
    const bool held = ImGui::IsItemActive();
    const bool disabled = (ImGui::GetItemFlags() & ImGuiItemFlags_Disabled) != 0;

    glow = Approach(glow, lit ? 1.0f : 0.0f, lit ? 30.0f : 6.0f);
    storage->SetFloat(storageId, glow);

    ImVec4 bg = style.Colors[ImGuiCol_FrameBgHovered];
    if (hovered && !disabled) bg = Mix(bg, ImVec4(1, 1, 1, 1), 0.08f);
    if (held) bg = Mix(bg, ImVec4(0, 0, 0, 1), 0.15f);
    bg = Mix(bg, style.Colors[ImGuiCol_CheckMark], glow);
    if (disabled) bg.w *= 0.5f;

    const float rounding = size.y * 0.5f;
    draw->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), ImGui::ColorConvertFloat4ToU32(bg), rounding);
    draw->AddRect(p, ImVec2(p.x + size.x, p.y + size.y), IM_COL32(255, 255, 255, 16), rounding);

    const ImVec2 textSize = ImGui::CalcTextSize(label);
    ImVec4 text = style.Colors[disabled ? ImGuiCol_TextDisabled : ImGuiCol_Text];
    draw->AddText(ImVec2(p.x + (size.x - textSize.x) * 0.5f, p.y + (size.y - textSize.y) * 0.5f),
                  ImGui::ColorConvertFloat4ToU32(text), label);

    return clicked;
}

void UIControls::KeyCap(const char* text, float width)
{
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImGuiStyle& style = ImGui::GetStyle();

    const ImVec2 textSize = ImGui::CalcTextSize(text);
    const float padX = 6.0f;
    const float padY = 1.0f;
    const ImVec2 size(width > 0.0f ? width : textSize.x + padX * 2.0f, textSize.y + padY * 2.0f);

    const ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::Dummy(size);

    const ImVec2 max(p.x + size.x, p.y + size.y);
    const ImVec4 face = Mix(style.Colors[ImGuiCol_WindowBg], ImVec4(1, 1, 1, 1), 0.06f);

    // «Кнопка клавиатуры»: грань снизу чуть темнее — даёт объём.
    draw->AddRectFilled(ImVec2(p.x, p.y + 1.0f), ImVec2(max.x, max.y + 1.0f), IM_COL32(0, 0, 0, 90), 4.0f);
    draw->AddRectFilled(p, max, ImGui::ColorConvertFloat4ToU32(face), 4.0f);
    draw->AddRect(p, max, IM_COL32(255, 255, 255, 22), 4.0f);

    draw->AddText(ImVec2(p.x + (size.x - textSize.x) * 0.5f, p.y + padY),
                  ImGui::GetColorU32(ImGuiCol_Text, 0.85f), text);
}

bool UIControls::ErrorBadge(const char* id, const std::string& tooltip, float pulse)
{
    const float size = ImGui::GetTextLineHeight();
    const ImVec2 p = ImGui::GetCursorScreenPos();

    ImGui::InvisibleButton(id, ImVec2(size, size));
    const bool hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled);

    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 c(p.x + size * 0.5f, p.y + size * 0.5f);
    const float r = size * 0.42f;

    if (pulse > 0.0f)
    {
        draw->AddCircleFilled(c, r + 4.0f * pulse, IM_COL32(230, 72, 72, static_cast<int>(70 * pulse)));
    }

    draw->AddCircleFilled(c, r, IM_COL32(222, 66, 66, 255));

    // Восклицательный знак рисуем сами: в шрифте его может не быть
    // нужной жирности, а значок должен читаться и на мелком масштабе.
    const float w = (std::max)(1.5f, r * 0.22f);
    draw->AddRectFilled(ImVec2(c.x - w * 0.5f, c.y - r * 0.55f), ImVec2(c.x + w * 0.5f, c.y + r * 0.15f),
                        IM_COL32(255, 255, 255, 255), w * 0.5f);
    draw->AddCircleFilled(ImVec2(c.x, c.y + r * 0.45f), w * 0.6f, IM_COL32(255, 255, 255, 255));

    if (hovered && !tooltip.empty())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 26.0f);
        ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.55f, 1.0f), "Не удалось включить");
        ImGui::TextUnformatted(tooltip.c_str());
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }

    return hovered;
}

void UIControls::StatusDot(const ImVec2& center, float radius, ImU32 color, float pulse)
{
    ImDrawList* draw = ImGui::GetWindowDrawList();

    if (pulse > 0.0f)
    {
        ImVec4 halo = ImGui::ColorConvertU32ToFloat4(color);
        halo.w = 0.35f * (1.0f - pulse);
        draw->AddCircleFilled(center, radius + radius * 1.6f * pulse, ImGui::ColorConvertFloat4ToU32(halo));
    }

    draw->AddCircleFilled(center, radius, color);
}

bool UIControls::ValueStepper(const char* id, ImGuiDataType type, void* value, double step, float width)
{
    if (!value) return false;

    ImGui::PushID(id);

    const ImGuiStyle& style = ImGui::GetStyle();
    const float arrow = ImGui::GetFrameHeight(); // стрелки квадратные
    const float fieldWidth = width - arrow * 2.0f - style.ItemInnerSpacing.x * 2.0f;

    bool changed = false;

    const auto nudge = [&](double direction)
    {
        switch (type)
        {
        case ImGuiDataType_S32:    *static_cast<int*>(value)    += static_cast<int>(step * direction); break;
        case ImGuiDataType_Float:  *static_cast<float*>(value)  += static_cast<float>(step * direction); break;
        case ImGuiDataType_Double: *static_cast<double*>(value) += step * direction; break;
        case ImGuiDataType_S64:    *static_cast<long long*>(value) += static_cast<long long>(step * direction); break;
        default: break;
        }
        changed = true;
    };

    // Удержание стрелки повторяет шаг: набирать большое значение по одному
    // щелчку невозможно.
    ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, true);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemInnerSpacing.x, style.ItemSpacing.y));

    if (ImGui::ArrowButton("##dec", ImGuiDir_Left)) nudge(-1.0);

    ImGui::SameLine();
    ImGui::SetNextItemWidth(fieldWidth > 0.0f ? fieldWidth : 1.0f);

    const char* format = (type == ImGuiDataType_Float || type == ImGuiDataType_Double) ? "%.3f" : nullptr;
    if (ImGui::InputScalar("##value", type, value, nullptr, nullptr, format)) changed = true;

    ImGui::SameLine();
    if (ImGui::ArrowButton("##inc", ImGuiDir_Right)) nudge(1.0);

    ImGui::PopStyleVar();
    ImGui::PopItemFlag();
    ImGui::PopID();

    return changed;
}

bool UIControls::ValueStepper(const char* id, int* value, int step, int minValue, int maxValue, float width)
{
    const bool changed = ValueStepper(id, ImGuiDataType_S32, value, static_cast<double>(step), width);
    if (changed) *value = Utils::Clamp(*value, minValue, maxValue);
    return changed;
}
