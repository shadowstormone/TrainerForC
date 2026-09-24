// Реестр читов. Одна запись — один чит.
//
// Поля пишутся по именам — ненужные просто пропускаются:
//
//   .name      подпись в интерфейсе
//   .keys      горячая клавиша или комбинация: { VKeys::KEY_CONTROL, VKeys::KEY_F1 }
//   .patches   что патчить (см. ниже)
//   .autoOffMs выключить самому через столько мс
//   .module    модуль игры, где искать сигнатуры: L"GameAssembly.dll"
//   .hint      подсказка при наведении на строку
//
// Виды патчей:
//
//   Nop(SIG)                        забить NOP-ами одну инструкцию
//   Nop(SIG, 6)                     ...или ровно 6 байт
//   Cave(SIG, Asm("..."))           кейв, код — ассемблером (числа как в C)
//   Cave(SIG, AsmCE("..."))         то же, числа как в Cheat Engine (hex)
//   Cave(SIG, "48 BE E8 03 ...")    кейв байтами, как их копирует CE
//   CaveKeepOriginal(SIG, ...)      кейв, после которого выполнится оригинал
//   WriteValue(Address, value)      записать значение один раз
//   Freeze(Address, value)          держать значение, пока чит включён
//
// К любому патчу по сигнатуре можно дописать .At(n) — место патча на n байт
// дальше начала сигнатуры.
//
// Клавишу в название писать не нужно: интерфейс рисует её отдельной
// колонкой, выводя подпись из самих кодов клавиш.
//
// Проверить, что всё здесь написано правильно, можно без игры: тест
// RegistryValidation собирает каждый ассемблерный патч и разбирает каждую
// сигнатуру (TrainerForC++.exe --run-tests в отладочной сборке).

#include "cheats/CheatRegistry.h"
#include "cheats/ValueFieldRegistry.h"
#include "patches/PatchLibrary.h"
#include "platform/VKeys.h"

using namespace PatchLibrary;

// Записать 1000 в [rbx+0x800] вместо того, чтобы вычитать урон.
//
// rsi здесь затирается, но беспокоиться не о чем: кейв оборачивает патч
// в push rsi / pop rsi сам — он разбирает патч и видит, что тот портит.
REGISTER_CHEAT({
    .name    = L"Cave Cheat Test 1",
    .keys    = { VKeys::KEY_NUMPAD1 },
    .patches = { Cave(SIG_CHEAT_TEST_3, Asm(R"(
        mov rsi, 1000            // сколько записать
        mov [rbx+0x800], rsi
    )")) },
    .hint    = L"Вместо вычитания урона записывает 1000",
})

// Значение по цепочке указателей, как её видно в Cheat Engine:
// база модуля + 0x346C10 -> разыменовать -> + 0x800.
//
// Разовая запись: переключатель вспыхивает и гаснет сам.
REGISTER_CHEAT({
    .name      = L"Set 9999 pointer",
    .keys      = { VKeys::KEY_NUMPAD2 },
    .patches   = { WriteValue(Address::Module(0x00346C10).Deref(0x800), 9999) },
    .autoOffMs = 450,
})

// То же место, но оригинальная инструкция СОХРАНЯЕТСЯ: она переносится
// в кейв и выполняется после патча. Нужно, когда игре всё ещё нужно то,
// что она делала, — настоящий хук, а не подмена.
//
// Скрипт в записи Cheat Engine: числа шестнадцатеричные, как в CE.
// Размер операнда обязателен: "mov [rbx+800], 3E8" неоднозначен —
// ассемблер не знает, писать 4 байта или 8.
REGISTER_CHEAT({
    .name    = L"Cave с сохранением оригинала",
    .keys    = { VKeys::KEY_NUMPAD3 },
    .patches = { CaveKeepOriginal(SIG_CHEAT_TEST_3, AsmCE("mov qword ptr [rbx+800],3E8")) },
})

// Без длины — ровно одна инструкция, сколько бы байт она ни занимала.
REGISTER_CHEAT({
    .name    = L"First Function (Nop)",
    .keys    = { VKeys::KEY_NUMPAD4 },
    .patches = { Nop(SIG_CHEAT_TEST_3) },
})

// Заморозка: значение держится, пока чит включён, — как галочка в CE.
REGISTER_CHEAT({
    .name    = L"Freeze 1000 pointer",
    .keys    = { VKeys::KEY_NUMPAD5 },
    .patches = { Freeze(Address::Module(0x00346C10).Deref(0x800), 1000) },
    .hint    = L"Держит значение 1000, пока включено",
})

// --- Поля ввода значений ---
//
// Описываются тем же Address, что и читы. Тип поля — по типу значения
// по умолчанию: 1 — целое, 1.0f — float.
REGISTER_VALUE_FIELD(ValueField(L"Set HP", Address::Module(0x00346C10).Deref(0x800), 100))
REGISTER_VALUE_FIELD(ValueField(L"Set MP", Address::Module(0x00346C10).Deref(0x800), 50))
