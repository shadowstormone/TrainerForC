#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

// Как читать числа в тексте ассемблера.
enum class AsmSyntax
{
    // Обычная запись: 1000 — десятичное, 0x3E8 — шестнадцатеричное.
    Standard,

    // Как в auto-assembler Cheat Engine: голое число — ШЕСТНАДЦАТЕРИЧНОЕ.
    //   mov [rbx+00000800],000003E8   ->  mov [rbx+0x800], 0x3E8
    //   #1000                          ->  1000 (десятичное)
    //   (float)100.5                   ->  0x42C90000 (биты float)
    // Нужен, чтобы скрипт из CE можно было вставить как есть: раньше
    // "mov [rbx+800], 1000" молча превращался в 800 и 1000 ДЕСЯТИЧНЫЕ.
    CheatEngine,
};

// Результат сборки текста ассемблера в байты.
struct AssembleResult
{
    std::vector<std::uint8_t> bytes;
    bool ok = false;
    std::string error;
};

// Превращает текст ассемблера в машинные байты.
//
// Нужен, чтобы патчи писались так же, как в auto-assembler Cheat Engine:
//     mov [rbx+800], 1000
// вместо строки байт, которую приходилось собирать руками и пересчитывать
// при каждой правке.
//
// Под капотом AsmJit: он же сам считает кодировку, ModRM/SIB, REX-префиксы
// и размеры непосредственных операндов — то есть ровно то, где человек
// ошибается чаще всего.
class Assembler
{
public:
    // text        — инструкции через перевод строки или ';'.
    //               Комментарии: // до конца строки и { блоком } как в CE.
    // is64Bit     — разрядность ЦЕЛЕВОГО процесса, не трейнера
    // baseAddress — адрес, по которому код будет лежать. Важен для форм,
    //               считающих адрес относительно себя; для обычных патчей
    //               можно не указывать.
    static AssembleResult Assemble(std::string_view text,
                                   bool is64Bit,
                                   std::uintptr_t baseAddress = 0,
                                   AsmSyntax syntax = AsmSyntax::Standard);

    // Текст, который на самом деле уходит в AsmJit: без комментариев,
    // по инструкции на строку, числа приведены к явной записи.
    //
    // Раньше ';' в тексте считался разделителем инструкций только в
    // документации — AsmTK видит в нём начало комментария, и всё после
    // первой ';' молча выбрасывалось.
    static std::string Preprocess(std::string_view text, AsmSyntax syntax, std::string* error = nullptr);
};
