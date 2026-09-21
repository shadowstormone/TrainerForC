// Реестр читов. Одна запись — один чит.
//
// Клавишу в название писать не нужно: интерфейс рисует её отдельной
// колонкой, выводя подпись из самих кодов клавиш.
//
// Сигнатуры пишутся как их показывает Cheat Engine: "29 93 ?? ??".
// Патчи — текстом ассемблера, как в auto-assembler CE. Байты, ModRM,
// REX-префиксы и размеры констант считает ассемблер; кейв сам спасает
// регистры, которые патч затирает.

#include "cheats/CheatRegistry.h"
#include "cheats/ValueFieldRegistry.h"
#include "patches/PatchLibrary.h"
#include "platform/VKeys.h"

// Записать 1000 в [rbx+0x800] вместо того, чтобы вычитать урон.
//
// rsi здесь затирается, но беспокоиться не о чем: кейв оборачивает патч
// в push rsi / pop rsi сам — он разбирает патч и видит, что тот портит.
REGISTER_CHEAT({
    L"Cave Cheat Test 1",
    { VKeys::KEY_NUMPAD1 },
    { Cave(PatchLibrary::SIG_CHEAT_TEST_3, Asm("mov rsi, 1000\n"
                                               "mov [rbx+0x800], rsi")) },
})

// Значение по цепочке указателей, как её видно в Cheat Engine:
// база модуля + 0x346C10 -> разыменовать -> + 0x800.
REGISTER_CHEAT({
    L"Set 9999 pointer",
    { VKeys::KEY_NUMPAD2 },
    { WriteValue(Address::Module(0x00346C10).Deref(0x800), 9999) },
    true, 450
})

// То же место, но оригинальная инструкция СОХРАНЯЕТСЯ: она переносится
// в кейв и выполняется после патча. Нужно, когда игре всё ещё нужно то,
// что она делала, — настоящий хук, а не подмена.
REGISTER_CHEAT({
    L"Cave с сохранением оригинала",
    { VKeys::KEY_NUMPAD3 },
    { CaveKeepOriginal(PatchLibrary::SIG_CHEAT_TEST_3, Asm("mov [rbx+0x800], 1000")) },
})

REGISTER_CHEAT({
    L"First Function (Nop)",
    { VKeys::KEY_NUMPAD4 },
    { Nop(PatchLibrary::SIG_CHEAT_TEST_3, 6) },
})

// --- Поля ввода значений ---
//
// Описываются тем же Address, что и читы: база модуля + 0x240600 ->
// разыменовать -> + 0x4B4.
REGISTER_VALUE_FIELD(ValueField(L"Set HP", Address::Module(0x00346C10).Deref(0x800)))
REGISTER_VALUE_FIELD(ValueField(L"Set MP", Address::Module(0x00346C10).Deref(0x800)))
