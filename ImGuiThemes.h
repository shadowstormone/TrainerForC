#pragma once
#include <imgui.h>

void SetModernDarkStyle()
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Базовые цвета
    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.42f, 0.47f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border] = ImVec4(0.20f, 0.20f, 0.20f, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.05f, 0.05f, 0.05f, 0.79f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.39f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.20f, 0.25f, 0.30f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.25f, 0.30f, 0.35f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.30f, 0.35f, 0.40f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.11f, 0.64f, 0.92f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.11f, 0.64f, 0.92f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.08f, 0.50f, 0.72f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.25f, 0.30f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.30f, 0.35f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.30f, 0.35f, 0.40f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.30f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.30f, 0.35f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.30f, 0.35f, 0.40f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.20f, 0.20f, 0.50f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.25f, 0.25f, 0.25f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.11f, 0.64f, 0.92f, 1.00f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.20f, 0.70f, 1.00f, 1.00f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    colors[ImGuiCol_DockingPreview] = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.73f, 0.73f, 0.73f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.80f, 0.20f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.70f, 0.70f, 0.70f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

    // Настройки стиля
    style.WindowRounding = 5.0f;
    style.FrameRounding = 5.0f;
    style.ScrollbarRounding = 5.0f;
    style.GrabRounding = 5.0f;
    style.TabRounding = 5.0f;
    style.FramePadding = ImVec2(8.0f, 4.0f);
    style.ItemSpacing = ImVec2(8.0f, 6.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
}

void SetAdvancedPurpleDarkStyle()
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Основные цвета с фиолетовыми оттенками
    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.40f, 0.60f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.04f, 0.12f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.12f, 0.08f, 0.16f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.06f, 0.02f, 0.10f, 0.94f);
    colors[ImGuiCol_Border] = ImVec4(0.42f, 0.36f, 0.48f, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.10f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.18f, 0.32f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.30f, 0.22f, 0.38f, 1.00f);

    // Заголовки окон с градиентным эффектом
    colors[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.07f, 0.18f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.18f, 0.10f, 0.25f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.08f, 0.04f, 0.12f, 0.79f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.07f, 0.18f, 1.00f);

    // Элементы прокрутки с неоновым эффектом
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.04f, 0.02f, 0.06f, 0.39f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.41f, 0.25f, 0.78f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.52f, 0.32f, 0.88f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.64f, 0.40f, 0.95f, 1.00f);

    // Интерактивные элементы с яркими акцентами
    colors[ImGuiCol_CheckMark] = ImVec4(0.64f, 0.40f, 0.95f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.41f, 0.25f, 0.78f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.64f, 0.40f, 0.95f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.28f, 0.17f, 0.44f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.41f, 0.25f, 0.78f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.64f, 0.40f, 0.95f, 1.00f);

    // Заголовки с плавными переходами
    colors[ImGuiCol_Header] = ImVec4(0.28f, 0.17f, 0.44f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.41f, 0.25f, 0.78f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.64f, 0.40f, 0.95f, 1.00f);

    // Разделители с эффектом свечения
    colors[ImGuiCol_Separator] = ImVec4(0.42f, 0.36f, 0.48f, 0.50f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.41f, 0.25f, 0.78f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.64f, 0.40f, 0.95f, 1.00f);

    // Элементы изменения размера с неоновым эффектом
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.41f, 0.25f, 0.78f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.41f, 0.25f, 0.78f, 0.78f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.64f, 0.40f, 0.95f, 1.00f);

    // Вкладки с градиентным эффектом
    colors[ImGuiCol_Tab] = ImVec4(0.28f, 0.17f, 0.44f, 0.86f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.41f, 0.25f, 0.78f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.64f, 0.40f, 0.95f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.20f, 0.12f, 0.30f, 0.97f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.28f, 0.17f, 0.44f, 1.00f);

    // Dokcing с полупрозрачным эффектом
    colors[ImGuiCol_DockingPreview] = ImVec4(0.41f, 0.25f, 0.78f, 0.70f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.08f, 0.04f, 0.12f, 1.00f);

    // Графики и диаграммы
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.52f, 0.72f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.41f, 0.25f, 0.78f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.41f, 0.25f, 0.78f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.64f, 0.40f, 0.95f, 1.00f);

    // Выделение и перетаскивание
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.41f, 0.25f, 0.78f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.64f, 0.40f, 0.95f, 0.95f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.41f, 0.25f, 0.78f, 0.80f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.64f, 0.40f, 0.95f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.08f, 0.04f, 0.12f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.08f, 0.04f, 0.12f, 0.35f);

    // Настройки стиля для более современного вида
    style.WindowPadding = ImVec2(12.0f, 12.0f);
    style.FramePadding = ImVec2(10.0f, 6.0f);
    style.ItemSpacing = ImVec2(10.0f, 8.0f);
    style.ItemInnerSpacing = ImVec2(8.0f, 6.0f);
    style.TouchExtraPadding = ImVec2(0.0f, 0.0f);
    style.ScrollbarSize = 12.0f;
    style.GrabMinSize = 12.0f;

    // Закругления для всех элементов
    style.WindowRounding = 8.0f;
    style.ChildRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 12.0f;
    style.GrabRounding = 6.0f;
    style.TabRounding = 8.0f;

    // Настройки границ
    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.TabBorderSize = 0.0f;
}