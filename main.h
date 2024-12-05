#pragma once
#include <gtest/gtest.h>

struct VKeys {
    // Основные буквы
    static constexpr int
        KEY_A = 0x41, // A key
        KEY_B = 0x42, // B key
        KEY_C = 0x43, // C key
        KEY_D = 0x44, // D key
        KEY_E = 0x45, // E key
        KEY_F = 0x46, // F key
        KEY_G = 0x47, // G key
        KEY_H = 0x48, // H key
        KEY_I = 0x49, // I key
        KEY_J = 0x4A, // J key
        KEY_K = 0x4B, // K key
        KEY_L = 0x4C, // L key
        KEY_M = 0x4D, // M key
        KEY_N = 0x4E, // N key
        KEY_O = 0x4F, // O key
        KEY_P = 0x50, // P key
        KEY_Q = 0x51, // Q key
        KEY_R = 0x52, // R key
        KEY_S = 0x53, // S key
        KEY_T = 0x54, // T key
        KEY_U = 0x55, // U key
        KEY_V = 0x56, // V key
        KEY_W = 0x57, // W key
        KEY_X = 0x58, // X key
        KEY_Y = 0x59, // Y key
        KEY_Z = 0x5A; // Z key

    // Цифровые клавиши
    static constexpr int
        KEY_0 = 0x30, // 0 key
        KEY_1 = 0x31, // 1 key
        KEY_2 = 0x32, // 2 key
        KEY_3 = 0x33, // 3 key
        KEY_4 = 0x34, // 4 key
        KEY_5 = 0x35, // 5 key
        KEY_6 = 0x36, // 6 key
        KEY_7 = 0x37, // 7 key
        KEY_8 = 0x38, // 8 key
        KEY_9 = 0x39; // 9 key

    // Функциональные клавиши
    static constexpr int
        KEY_F1 = 0x70,  // F1 key
        KEY_F2 = 0x71,  // F2 key
        KEY_F3 = 0x72,  // F3 key
        KEY_F4 = 0x73,  // F4 key
        KEY_F5 = 0x74,  // F5 key
        KEY_F6 = 0x75,  // F6 key
        KEY_F7 = 0x76,  // F7 key
        KEY_F8 = 0x77,  // F8 key
        KEY_F9 = 0x78,  // F9 key
        KEY_F10 = 0x79, // F10 key
        KEY_F11 = 0x7A, // F11 key
        KEY_F12 = 0x7B; // F12 key

    // Специальные клавиши
    static constexpr int
        KEY_ESCAPE = 0x1B,     // ESC key
        KEY_TAB = 0x09,        // TAB key
        KEY_CAPS_LOCK = 0x14,  // CAPS LOCK key
        KEY_SHIFT = 0x10,      // SHIFT key
        KEY_CTRL = 0x11,       // CTRL key
        KEY_ALT = 0x12,        // ALT key
        KEY_SPACE = 0x20,      // SPACE key
        KEY_ENTER = 0x0D,      // ENTER key
        KEY_BACKSPACE = 0x08,  // BACKSPACE key
        KEY_DELETE = 0x2E,     // DELETE key
        KEY_INSERT = 0x2D,     // INSERT key
        KEY_HOME = 0x24,       // HOME key
        KEY_END = 0x23,        // END key
        KEY_PAGEUP = 0x21,     // PAGE UP key
        KEY_PAGEDOWN = 0x22,   // PAGE DOWN key
        KEY_PAUSE = 0x13,      // PAUSE key
        KEY_SCROLL_LOCK = 0x91,// SCROLL LOCK key
        KEY_PRINT_SCREEN = 0x2C,// PRINT SCREEN key
        KEY_WIN = 0x5B,        // Windows key
        KEY_MENU = 0x5D;       // Application key

    // Клавиши навигации
    static constexpr int
        KEY_UP = 0x26,    // UP ARROW key
        KEY_DOWN = 0x28,  // DOWN ARROW key
        KEY_LEFT = 0x25,  // LEFT ARROW key
        KEY_RIGHT = 0x27; // RIGHT ARROW key

    // Символьные клавиши
    static constexpr int
        KEY_SEMICOLON = 0xBA,    // ; key
        KEY_PLUS = 0xBB,         // = key
        KEY_COMMA = 0xBC,        // , key
        KEY_MINUS = 0xBD,        // - key
        KEY_PERIOD = 0xBE,       // . key
        KEY_SLASH = 0xBF,        // / key
        KEY_BACKTICK = 0xC0,     // ` key
        KEY_OPEN_BRACKET = 0xDB, // [ key
        KEY_BACKSLASH = 0xDC,    // \ key
        KEY_CLOSE_BRACKET = 0xDD,// ] key
        KEY_QUOTE = 0xDE;        // ' key

    // Цифровая клавиатура (Numpad)
    static constexpr int
        KEY_NUMPAD0 = 0x60,      // Numpad 0
        KEY_NUMPAD1 = 0x61,      // Numpad 1
        KEY_NUMPAD2 = 0x62,      // Numpad 2
        KEY_NUMPAD3 = 0x63,      // Numpad 3
        KEY_NUMPAD4 = 0x64,      // Numpad 4
        KEY_NUMPAD5 = 0x65,      // Numpad 5
        KEY_NUMPAD6 = 0x66,      // Numpad 6
        KEY_NUMPAD7 = 0x67,      // Numpad 7
        KEY_NUMPAD8 = 0x68,      // Numpad 8
        KEY_NUMPAD9 = 0x69,      // Numpad 9
        KEY_NUMPAD_MULTIPLY = 0x6A, // Numpad *
        KEY_NUMPAD_ADD = 0x6B,      // Numpad +
        KEY_NUMPAD_SUBTRACT = 0x6D,  // Numpad -
        KEY_NUMPAD_DECIMAL = 0x6E,   // Numpad .
        KEY_NUMPAD_DIVIDE = 0x6F;    // Numpad /
};

constexpr auto W_WIDTH = 400;
constexpr auto W_HEIGHT = 444;

extern LPCWSTR WindowTitle; // Объявление, а не определение

// Функция для запуска тестов
void RunTests()
{
	int argc = 0;
	char** argv = nullptr;
	::testing::InitGoogleTest(&argc, argv);
	RUN_ALL_TESTS();
}