#pragma once
#include <string>
#include <vector>

// Человекочитаемые имена клавиш.
//
// Нужны, чтобы горячая клавиша была ОТДЕЛЬНОЙ колонкой в интерфейсе, а не
// частью названия чита. Раньше приходилось писать L"[Numpad 1] - Бессмертие"
// и следить, чтобы текст не разошёлся с реально назначенной клавишей.
namespace KeyNames
{
    // 0x61 -> "Num 1", 0x11 -> "Ctrl", 0x70 -> "F1".
    std::string Key(int virtualKey);

    // { KEY_CTRL, KEY_NUMPAD1 } -> "Ctrl+Num 1".
    // Модификаторы выводятся первыми, в привычном порядке.
    std::string Hotkey(const std::vector<int>& keys);
}
