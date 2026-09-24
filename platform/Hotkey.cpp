#include "platform/Hotkey.h"

#include <Windows.h>

bool Hotkey::AllKeysDown() const
{
    if (_keys.empty()) return false;

    for (int key : _keys)
    {
        if (!(GetAsyncKeyState(key) & 0x8000)) return false;
    }
    return true;
}

bool Hotkey::JustPressed()
{
    const bool down = AllKeysDown();

    // Заглушены — нажатие запоминаем, но не отдаём: иначе клавиша,
    // зажатая во время ввода, сработала бы сразу после него.
    if (s_suppressed)
    {
        _wasDown = down;
        return false;
    }

    const bool justPressed = down && !_wasDown;
    _wasDown = down;
    return justPressed;
}
