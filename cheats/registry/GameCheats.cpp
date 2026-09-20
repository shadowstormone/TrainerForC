// Все читы игры описываются здесь — по одной записи на чит.
// Чтобы добавить новый чит, допиши REGISTER_CHEAT({...}) ниже.
// Больше НИЧЕГО трогать не нужно: ни enum, ни таблицу, ни фабрику.

#include "cheats/CheatRegistry.h"
#include "patches/PatchLibrary.h"
#include "platform/VKeys.h"

REGISTER_CHEAT({
    L"[Numpad 1] - Cheat Test 1",
    { VKeys::KEY_NUMPAD1 },
    { Cave(PatchLibrary::SIG_CHEAT_TEST_3, PatchLibrary::PATCH_CHEAT_TEST_3) },
})

// Тестовое значение из туториала Cheat Engine (Tutorial-x86_64.exe).
// Адрес абсолютный и у динамического значения меняется от запуска к запуску —
// годится для проверки записи, но не как готовый чит.
REGISTER_CHEAT({
    L"[Numpad 2] - Set 9999 (CE tutorial)",
    { VKeys::KEY_NUMPAD2 },
    { WriteValueAt(0x015F45D0, 9999) },
    true, 450
})

REGISTER_CHEAT({
    L"[Numpad 3] - Cheat Test 3",
    { VKeys::KEY_NUMPAD3 },
    { Cave(PatchLibrary::SIG_CHEAT_TEST_3, PatchLibrary::PATCH_CHEAT_TEST_3) },
})

REGISTER_CHEAT({
    L"[Numpad 4] - First Function(Nop)",
    { VKeys::KEY_NUMPAD4 },
    { Nop(PatchLibrary::SIG_CHEAT_TEST_3, 6) },
})
