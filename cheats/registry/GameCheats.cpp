// Реестр читов. Одна запись — один чит.
//
// Сигнатуры и байты патча пишутся строкой ровно в том виде, в каком их
// показывает Cheat Engine: "29 93 ?? ??". Длину считать не нужно.

#include "cheats/CheatRegistry.h"
#include "patches/PatchLibrary.h"
#include "platform/VKeys.h"

REGISTER_CHEAT({
    L"[Numpad 1] - Cave Cheat Test 1",
    { VKeys::KEY_NUMPAD1 },
    { Cave(PatchLibrary::SIG_CHEAT_TEST_3,
           // movabs rsi, 1000 ; mov [rbx+0x800], rsi
           "48 BE E8 03 00 00 00 00 00 00 48 89 B3 00 08 00 00") },
})

// Значение по цепочке указателей, как её видно в Cheat Engine:
// база модуля + 0x346C10 -> разыменовать -> + 0x800.
REGISTER_CHEAT({
    L"[Numpad 2] - Set 9999 pointer",
    { VKeys::KEY_NUMPAD2 },
    { WriteValue(Address::Module(0x00346C10).Deref(0x800), 9999) },
    true, 450
})

REGISTER_CHEAT({
    L"[Numpad 3] - Cheat Test 3 (no used)",
    { VKeys::KEY_NUMPAD3 },
    { Cave(PatchLibrary::SIG_CHEAT_TEST_3,
           "48 BE E8 03 00 00 00 00 00 00 48 89 B3 00 08 00 00") },
})

REGISTER_CHEAT({
    L"[Numpad 4] - First Function(Nop)",
    { VKeys::KEY_NUMPAD4 },
    { Nop(PatchLibrary::SIG_CHEAT_TEST_3, 6) },
})
