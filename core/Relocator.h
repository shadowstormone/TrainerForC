#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// Результат переноса «украденных» инструкций в кейв.
struct RelocationResult
{
    std::vector<std::uint8_t> bytes;  // что положить в кейв
    std::size_t stolen = 0;           // сколько байт заняли на месте патча
    bool ok = false;
    std::string error;                // почему не вышло, если не вышло
};

// Переносит инструкции с одного адреса на другой.
//
// Просто скопировать байты нельзя: часть адресов внутри инструкций задана
// ОТНОСИТЕЛЬНО самой инструкции — короткие и ближние переходы, а на x64 ещё
// и обращения к памяти через RIP. Переехав в кейв, такая инструкция начнёт
// указывать не туда.
//
// Здесь эти поля пересчитываются под новый адрес, а когда пересчитать нельзя
// (короткий переход не достаёт, кейв дальше ±2 ГБ) — возвращается внятная
// ошибка вместо испорченного кода. Прежний длино-дизассемблер знал только
// длину инструкции и молча копировал такие байты как есть.
class Relocator
{
public:
    // sourceAddress — адрес в ЧУЖОМ процессе, откуда взяты байты
    // source/sourceSize — прочитанные оттуда байты (желательно 15 * N)
    // is64Bit — разрядность ЦЕЛИ, а не трейнера
    // caveAddress — куда инструкции переедут
    // minBytes — сколько байт нужно освободить под прыжок (5 или 14)
    static RelocationResult Relocate(std::uintptr_t sourceAddress,
                                     const std::uint8_t* source,
                                     std::size_t sourceSize,
                                     bool is64Bit,
                                     std::uintptr_t caveAddress,
                                     std::size_t minBytes);

    // Сколько байт занимают ЦЕЛЫЕ инструкции, покрывающие minBytes.
    //
    // Нужно, когда украденные инструкции никуда не переносятся, а просто
    // затираются: пересчитывать в них нечего, но знать границу обязательно —
    // иначе прыжок обратно попадёт в середину следующей инструкции.
    // Отдельно от Relocate, чтобы не отказывать в патче из-за инструкции,
    // которую мы всё равно выбрасываем.
    static bool Measure(const std::uint8_t* source,
                        std::size_t sourceSize,
                        bool is64Bit,
                        std::size_t minBytes,
                        std::size_t& outBytes,
                        std::string& outError);

    // Регистры общего назначения, в которые этот код ПИШЕТ.
    //
    // Возвращает их номера в кодировке инструкций (0-15: rax, rcx, rdx, rbx,
    // rsp, rbp, rsi, rdi, r8..r15). rsp исключён — его трогать нельзя.
    //
    // По ним кейв оборачивается в push/pop: патч вроде
    // "movabs rsi, 1000; mov [rbx+0x800], rsi" затирает rsi, а он в Windows
    // x64 ABI callee-saved, то есть игра ждёт его целым.
    static std::vector<std::uint8_t> FindClobberedGpRegisters(const std::uint8_t* code,
                                                              std::size_t size,
                                                              bool is64Bit);

    // Текстовое представление инструкций.
    //
    // Нужно, чтобы патч можно было проверить глазами: редактор не
    // подсвечивает ассемблер внутри строкового литерала, зато собранный
    // код всегда можно показать обратно — и увидеть, что получилось
    // на самом деле.
    static std::vector<std::string> Disassemble(const std::uint8_t* code,
                                                std::size_t size,
                                                bool is64Bit,
                                                std::uintptr_t address = 0);
};
