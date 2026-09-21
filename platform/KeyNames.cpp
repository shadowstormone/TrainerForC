#include "platform/KeyNames.h"

#include <algorithm>

#include "platform/VKeys.h"

namespace
{
    bool IsModifier(int key)
    {
        return key == VKeys::KEY_CTRL || key == VKeys::KEY_SHIFT || key == VKeys::KEY_ALT;
    }
}

std::string KeyNames::Key(int virtualKey)
{
    // Цифровая клавиатура — то, на что вешают читы чаще всего.
    if (virtualKey >= VKeys::KEY_NUMPAD0 && virtualKey <= VKeys::KEY_NUMPAD9)
    {
        return "Num " + std::to_string(virtualKey - VKeys::KEY_NUMPAD0);
    }

    if (virtualKey >= VKeys::KEY_F1 && virtualKey <= VKeys::KEY_F12)
    {
        return "F" + std::to_string(virtualKey - VKeys::KEY_F1 + 1);
    }

    if (virtualKey >= VKeys::KEY_0 && virtualKey <= VKeys::KEY_9)
    {
        return std::string(1, static_cast<char>('0' + (virtualKey - VKeys::KEY_0)));
    }

    if (virtualKey >= VKeys::KEY_A && virtualKey <= VKeys::KEY_Z)
    {
        return std::string(1, static_cast<char>('A' + (virtualKey - VKeys::KEY_A)));
    }

    switch (virtualKey)
    {
    case VKeys::KEY_CTRL:            return "Ctrl";
    case VKeys::KEY_SHIFT:           return "Shift";
    case VKeys::KEY_ALT:             return "Alt";

    case VKeys::KEY_NUMPAD_ADD:      return "Num +";
    case VKeys::KEY_NUMPAD_SUBTRACT: return "Num -";
    case VKeys::KEY_NUMPAD_MULTIPLY: return "Num *";
    case VKeys::KEY_NUMPAD_DIVIDE:   return "Num /";
    case VKeys::KEY_NUMPAD_DECIMAL:  return "Num .";

    case VKeys::KEY_ESCAPE:          return "Esc";
    case VKeys::KEY_TAB:             return "Tab";
    case VKeys::KEY_SPACE:           return "Space";
    case VKeys::KEY_ENTER:           return "Enter";
    case VKeys::KEY_BACKSPACE:       return "Backspace";
    case VKeys::KEY_DELETE:          return "Delete";
    case VKeys::KEY_INSERT:          return "Insert";
    case VKeys::KEY_HOME:            return "Home";
    case VKeys::KEY_END:             return "End";
    case VKeys::KEY_PAGEUP:          return "PgUp";
    case VKeys::KEY_PAGEDOWN:        return "PgDn";

    case VKeys::KEY_UP:              return "Up";
    case VKeys::KEY_DOWN:            return "Down";
    case VKeys::KEY_LEFT:            return "Left";
    case VKeys::KEY_RIGHT:           return "Right";

    case VKeys::KEY_BACKTICK:        return "`";
    case VKeys::KEY_MINUS:           return "-";
    case VKeys::KEY_PLUS:            return "=";
    case VKeys::KEY_COMMA:           return ",";
    case VKeys::KEY_PERIOD:          return ".";
    case VKeys::KEY_SLASH:           return "/";
    case VKeys::KEY_SEMICOLON:       return ";";
    case VKeys::KEY_QUOTE:           return "'";
    case VKeys::KEY_OPEN_BRACKET:    return "[";
    case VKeys::KEY_CLOSE_BRACKET:   return "]";
    case VKeys::KEY_BACKSLASH:       return "\\";

    default: break;
    }

    // Неизвестный код лучше показать как есть, чем промолчать.
    return "0x" + [&]
    {
        static const char* digits = "0123456789ABCDEF";
        std::string hex;
        hex += digits[(virtualKey >> 4) & 0xF];
        hex += digits[virtualKey & 0xF];
        return hex;
    }();
}

std::string KeyNames::Hotkey(const std::vector<int>& keys)
{
    if (keys.empty()) return std::string();

    // Модификаторы вперёд: "Ctrl+Num 1" читается привычнее, чем "Num 1+Ctrl".
    std::vector<int> ordered(keys);
    std::stable_partition(ordered.begin(), ordered.end(), IsModifier);

    std::string label;
    for (const int key : ordered)
    {
        if (!label.empty()) label += "+";
        label += Key(key);
    }

    return label;
}
