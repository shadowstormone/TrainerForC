#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

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
    // text        — одна или несколько инструкций, через перевод строки или ';'
    // is64Bit     — разрядность ЦЕЛЕВОГО процесса, не трейнера
    // baseAddress — адрес, по которому код будет лежать. Важен для форм,
    //               считающих адрес относительно себя; для обычных патчей
    //               можно не указывать.
    static AssembleResult Assemble(std::string_view text,
                                   bool is64Bit,
                                   std::uintptr_t baseAddress = 0);
};
