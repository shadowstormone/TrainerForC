#pragma once
#include <algorithm>
#include <string>

#include <imgui.h>

// Разметка панели.
//
// Всё считается от ширины окна и текущего стиля ImGui, а не задаётся
// константами в пикселях: раньше переключатель стоял на жёстком отступе
// 325 px, поле ввода — на 160, и при другой ширине окна или другом шрифте
// они разъезжались между собой.
//
// Правило простое: подписи слева в колонке фиксированной доли, управляющие
// элементы прижаты к правому краю, а то, что между ними, тянется.
namespace Layout
{
    // Размер шрифта, под который нарисованы все размеры в пикселях:
    // 11 pt × 1.5 — Full HD. На 1440p и 4K шрифт крупнее, и вместе с ним
    // во столько же раз растут отступы, переключатели и окно. Раньше рос
    // только шрифт, и на 4K панель становилась тесной.
    inline constexpr float REFERENCE_FONT_SIZE = 16.5f;

    // Во сколько раз интерфейс крупнее эталонного.
    inline float Scale()
    {
        return ImGui::GetFontSize() / REFERENCE_FONT_SIZE;
    }

    // Пиксели эталонного макета -> пиксели текущего экрана.
    inline float Px(float referencePixels)
    {
        return referencePixels * Scale();
    }

    // Меньше этого подписи сжимать бессмысленно — останется одно многоточие.
    inline constexpr float MIN_LABEL_WIDTH = 90.0f;

    // Ширина поля ввода значения. Подпись получает всё, что останется.
    inline float InputWidth() { return Px(150.0f); }

    // Размеры переключателя. Должны совпадать со значением по умолчанию
    // у AnimatedToggleSwitch.
    //
    // Высоту строки задаёт именно тумблер, а не текст: пока он был 25 px,
    // строки не ужимались, сколько ни уменьшай шрифт.
    inline float ToggleWidth() { return Px(38.0f); }
    inline float ToggleHeight() { return Px(18.0f); }

    inline float ContentLeft()
    {
        return ImGui::GetStyle().WindowPadding.x;
    }

    inline float ContentRight()
    {
        return ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x;
    }

    inline float ContentWidth()
    {
        return ContentRight() - ContentLeft();
    }

    // Сколько места остаётся подписи, если справа стоят элементы общей
    // шириной controlsWidth.
    //
    // Считается именно так, а не долей от ширины окна: тумблеру нужно 50 px,
    // и отдавать под него половину строки, обрезая имя чита, незачем.
    inline float LabelWidthFor(float controlsWidth)
    {
        const float available = ContentWidth() - controlsWidth - ImGui::GetStyle().ItemSpacing.x;

        // Скобки вокруг std::max не случайны: Windows.h определяет max
        // макросом, и без них он подменяет вызов.
        return (std::max)(MIN_LABEL_WIDTH, available);
    }

    // X, с которого начинается элемент шириной controlWidth, прижатый вправо.
    inline float ControlX(float controlWidth)
    {
        return ContentRight() - controlWidth;
    }

    // Убирает с конца один символ UTF-8 целиком, а не байт: иначе кириллица
    // распадается на мусор.
    inline void PopUtf8Char(std::string& text)
    {
        if (text.empty()) return;

        size_t i = text.size() - 1;
        while (i > 0 && (static_cast<unsigned char>(text[i]) & 0xC0) == 0x80) --i;
        text.erase(i);
    }

    // Укорачивает подпись многоточием, чтобы она не залезала на элемент
    // справа. Раньше длинное имя чита рисовалось прямо поверх тумблера.
    inline std::string FitText(const std::string& text, float maxWidth)
    {
        if (maxWidth <= 0.0f) return std::string();
        if (ImGui::CalcTextSize(text.c_str()).x <= maxWidth) return text;

        std::string shortened = text;
        while (!shortened.empty()
               && ImGui::CalcTextSize((shortened + "...").c_str()).x > maxWidth)
        {
            PopUtf8Char(shortened);
        }

        return shortened + "...";
    }

    // Подпись строки: обрезается по колонке и показывает полный текст
    // подсказкой, если не поместилась.
    inline void RowLabel(const std::string& text, float width, const ImVec4& color)
    {
        const std::string shown = FitText(text, width);

        ImGui::TextColored(color, "%s", shown.c_str());

        if (shown != text && ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s", text.c_str());
        }
    }

    // Промежуток между смысловыми группами — разделитель с воздухом,
    // а не подобранный на глаз отступ.
    inline void GroupGap()
    {
        const float gap = ImGui::GetStyle().ItemSpacing.y * 1.5f;

        ImGui::Dummy(ImVec2(0.0f, gap));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0.0f, gap));
    }

    // Единая высота строки таблицы.
    //
    // Раньше высоту задавал самый высокий элемент строки, поэтому ряды
    // с переключателем и ряды с полем ввода были разной высоты.
    inline float RowHeight()
    {
        const float byToggle = ToggleHeight();
        const float byWidget = ImGui::GetFrameHeight();

        return (std::max)(byToggle, byWidget);
    }

}
