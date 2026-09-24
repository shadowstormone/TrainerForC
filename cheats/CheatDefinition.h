#pragma once
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <string>
#include <type_traits>
#include <utility>
#include <string_view>
#include <vector>

#include "core/Assembler.h"          // AsmSyntax
#include "patches/CavePatch.h"       // CaveMode
#include "patches/WriteAddressPatch.h" // PatchValue

// Описание патча — это ДАННЫЕ, а не объект. Конкретный Patch собирается
// из PatchSpec фабрикой (CheatFactory), поэтому добавление нового типа
// патча = новый класс + одна ветка в фабрике.
//
// Руками PatchSpec не заполняют — для этого хелперы ниже: Nop, Cave,
// CaveKeepOriginal, WriteValue, Freeze.
struct PatchSpec
{
    enum class Kind
    {
        Nop,
        Cave,
        WriteValue,
        Freeze,
    };

    Kind kind = Kind::Nop;

    // Nop / Cave
    std::string signature;                 // AOB-сигнатура (utf-8)
    std::size_t length = 0;                // для Nop: сколько байт; 0 — одну инструкцию
    std::vector<std::uint8_t> patchBytes;  // для Cave: сами байты; длина = size()
    std::ptrdiff_t offset = 0;             // сдвиг места патча от начала сигнатуры

    // Cave: текст ассемблера, если патч описан им, а не байтами.
    // Собирается при применении — под разрядность цели.
    std::string patchAsm;
    AsmSyntax asmSyntax = AsmSyntax::Standard;

    // Cave: что делать с инструкциями, которые затирает прыжок, и спасать ли
    // регистры, в которые пишет патч.
    CaveMode caveMode = CaveMode::ReplaceOriginal;
    bool preserveRegisters = true;

    // WriteValue / Freeze
    std::vector<std::uintptr_t> offsets;
    PatchValue value{};
    std::wstring module;                   // модуль адреса; пусто — модуль чита

    // true  — offsets.back() это готовый абсолютный адрес (как показывает
    //         Cheat Engine для динамического значения);
    // false — цепочка "база модуля + оффсеты".
    bool absoluteAddress = false;

    // Место патча дальше начала сигнатуры на n байт:
    //   Cave(SIG, Asm("...")).At(6)
    // Сигнатуру выгодно брать с запасом до нужной инструкции — так она
    // уникальнее, а место патча остаётся точным.
    PatchSpec& At(std::ptrdiff_t bytesFromSignatureStart)
    {
        offset = bytesFromSignatureStart;
        return *this;
    }

    // Не оборачивать кейв в push/pop портящихся регистров — когда патч
    // меняет регистр НАМЕРЕННО и игра должна увидеть новое значение.
    PatchSpec& KeepRegisterChanges()
    {
        preserveRegisters = false;
        return *this;
    }
};

// Разбирает байты патча из строки, как их отдаёт Cheat Engine:
//   "48 BE E8 03 00 00 00 00 00 00 48 89 B3 00 08 00 00"
// Префикс 0x и запятые допустимы. Длину считать руками больше не нужно —
// она берётся из количества байт.
inline std::vector<std::uint8_t> ParseBytes(std::string_view text)
{
    std::vector<std::uint8_t> bytes;
    std::string token;

    auto flush = [&]()
    {
        if (token.empty()) return;
        if (token.size() > 2 && token[0] == '0' && (token[1] == 'x' || token[1] == 'X'))
            token.erase(0, 2);

        // Слитная запись "48BEE803" — по два знака на байт.
        for (std::size_t i = 0; i < token.size(); i += 2)
        {
            const std::string byte = token.substr(i, 2);
            bytes.push_back(static_cast<std::uint8_t>(std::strtoul(byte.c_str(), nullptr, 16)));
        }
        token.clear();
    };

    for (char c : text)
    {
        const bool separator = (c == ',') || std::isspace(static_cast<unsigned char>(c)) != 0;
        if (separator) flush();
        else token.push_back(c);
    }
    flush();

    return bytes;
}

// Патч, написанный текстом ассемблера, а не байтами:
//     Cave(SIG, Asm("mov qword ptr [rbx+0x800], 1000"))
// Байты, ModRM, REX-префиксы и размеры констант считает ассемблер — там,
// где человек ошибается чаще всего.
//
// Инструкции — по строке или через ';'. Комментарии // и { }.
// Числа как в C: 1000 — десятичное, 0x3E8 — шестнадцатеричное.
struct Asm
{
    std::string text;
    AsmSyntax syntax = AsmSyntax::Standard;

    explicit Asm(std::string_view source, AsmSyntax s = AsmSyntax::Standard)
        : text(source), syntax(s) {}
};

// То же, но числа читаются как в Cheat Engine — голые ШЕСТНАДЦАТЕРИЧНЫЕ.
// Скрипт из auto-assembler вставляется без правок:
//
//     Cave(SIG, AsmCE(R"(
//         mov [rbx+00000800],000003E8   // 0x3E8 = 1000
//         movss xmm0,[rax+10]
//     )"))
//
// #1000 — десятичное, (float)100.5 — биты float.
inline Asm AsmCE(std::string_view source)
{
    return Asm(source, AsmSyntax::CheatEngine);
}

// Адрес значения — читается так же, как его находишь в Cheat Engine.
//
//   Address::Module(0x346C10).Deref(0x800)
//     = взять базу модуля, прибавить 0x346C10, РАЗЫМЕНОВАТЬ, прибавить 0x800
//
//   Address::Module(L"UnityPlayer.dll", 0x1A2B3C).Deref(0x10).Deref(0x48)
//     = то же от базы указанного модуля — как "UnityPlayer.dll"+1A2B3C в CE
struct Address
{
    std::vector<std::uintptr_t> offsets;
    std::wstring module;
    bool absolute = false;

    // База модуля + offset. Модуль — тот, что задан у чита, или exe игры.
    static Address Module(std::uintptr_t offset)
    {
        Address a;
        a.offsets.push_back(offset);
        return a;
    }

    // База КОНКРЕТНОГО модуля + offset.
    static Address Module(std::wstring_view moduleName, std::uintptr_t offset)
    {
        Address a = Module(offset);
        a.module = std::wstring(moduleName);
        return a;
    }

    // Готовый абсолютный адрес из CE. У динамических значений он меняется
    // от запуска к запуску — годится для проверки, не для готового чита.
    static Address At(std::uintptr_t address)
    {
        Address a;
        a.offsets.push_back(address);
        a.absolute = true;
        return a;
    }

    // Разыменовать то, что накопили, и прибавить next.
    Address& Deref(std::uintptr_t next)
    {
        offsets.push_back(next);
        return *this;
    }

    // Вся цепочка оффсетов сразу — как она записана в указателе CE снизу
    // вверх: Address::Module(0x346C10).Chain({ 0x18, 0x40, 0x800 }).
    Address& Chain(std::initializer_list<std::uintptr_t> next)
    {
        offsets.insert(offsets.end(), next.begin(), next.end());
        return *this;
    }
};

// --- Хелперы: чтобы описание чита читалось одной строкой ---

// Забить NOP-ами length байт по сигнатуре. Без length — ровно одну
// инструкцию, какой бы длины она ни была.
inline PatchSpec Nop(std::string_view signature, std::size_t length = 0)
{
    PatchSpec s;
    s.kind = PatchSpec::Kind::Nop;
    s.signature = std::string(signature);
    s.length = length;
    return s;
}

inline PatchSpec Cave(std::string_view signature, std::vector<std::uint8_t> bytes)
{
    PatchSpec s;
    s.kind = PatchSpec::Kind::Cave;
    s.signature = std::string(signature);
    s.length = bytes.size();
    s.patchBytes = std::move(bytes);
    return s;
}

// Байты патча строкой — как копируются из Cheat Engine. Длина считается сама:
//   Cave("29 93 ?? ??", "48 BE E8 03 00 00 00 00 00 00")
inline PatchSpec Cave(std::string_view signature, std::string_view patchHex)
{
    return Cave(signature, ParseBytes(patchHex));
}

// Строковый литерал — байты, а не ассемблер: для ассемблера есть Asm(...).
inline PatchSpec Cave(std::string_view signature, const char* patchHex)
{
    return Cave(signature, std::string_view(patchHex));
}

inline PatchSpec Cave(std::string_view signature, Asm code)
{
    PatchSpec s;
    s.kind = PatchSpec::Kind::Cave;
    s.signature = std::string(signature);
    s.patchAsm = std::move(code.text);
    s.asmSyntax = code.syntax;
    return s;
}

// Перегрузка для массивов байт: Cave(SIG_X, PATCH_X)
template <std::size_t N>
inline PatchSpec Cave(std::string_view signature, const std::array<std::uint8_t, N>& bytes)
{
    return Cave(signature, std::vector<std::uint8_t>(bytes.begin(), bytes.end()));
}

// Кейв, который СОХРАНЯЕТ оригинальные инструкции: они переносятся в кейв
// и выполняются после кода патча. Нужно, когда игре всё ещё нужно то, что
// они делали, — настоящий хук, а не подмена.
template <typename Code>
inline PatchSpec CaveKeepOriginal(std::string_view signature, Code&& code)
{
    PatchSpec s = Cave(signature, std::forward<Code>(code));
    s.caveMode = CaveMode::KeepOriginal;
    return s;
}

inline PatchSpec CaveKeepOriginal(std::string_view signature, std::vector<std::uint8_t> bytes)
{
    PatchSpec s = Cave(signature, std::move(bytes));
    s.caveMode = CaveMode::KeepOriginal;
    return s;
}

namespace detail
{
    template <typename T>
    PatchValue ToPatchValue(T value)
    {
        if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>
                   || std::is_same_v<T, std::int64_t> || std::is_same_v<T, std::uint8_t>
                   || std::is_same_v<T, std::int16_t>)
            return value;
        else
            return static_cast<std::int32_t>(value);
    }

    template <typename T>
    PatchSpec ValuePatch(PatchSpec::Kind kind, const Address& address, T value)
    {
        PatchSpec s;
        s.kind = kind;
        s.offsets = address.offsets;
        s.module = address.module;
        s.absoluteAddress = address.absolute;
        s.value = ToPatchValue(value);
        return s;
    }
}

// Записать значение один раз. Тип значения задаёт размер записи:
//   WriteValue(addr, 9999)       — 4 байта int
//   WriteValue(addr, 1.5f)       — float
//   WriteValue(addr, 1.5)        — double
//   WriteValue(addr, int64_t{5}) — 8 байт
template <typename T>
inline PatchSpec WriteValue(const Address& address, T value)
{
    return detail::ValuePatch(PatchSpec::Kind::WriteValue, address, value);
}

// Держать значение, пока чит включён (как заморозка в Cheat Engine):
//   Freeze(Address::Module(0x346C10).Deref(0x800), 1000)
template <typename T>
inline PatchSpec Freeze(const Address& address, T value)
{
    return detail::ValuePatch(PatchSpec::Kind::Freeze, address, value);
}

// Прежняя форма: список оффсетов вместо Address. Оставлена ради старых
// описаний; в новых удобнее Address::Module(...).Deref(...).
template <typename T>
inline PatchSpec WriteValue(std::vector<std::uintptr_t> offsets, T value)
{
    Address a;
    a.offsets = std::move(offsets);
    return WriteValue(a, value);
}

// Запись по готовому абсолютному адресу — тому, что показывает Cheat Engine.
// Внимание: у динамических значений такой адрес меняется от запуска к запуску,
// поэтому это годится для проверки, но не для готового чита.
template <typename T>
inline PatchSpec WriteValueAt(std::uintptr_t address, T value)
{
    return WriteValue(Address::At(address), value);
}

// Полное описание одного чита. Всё, что нужно знать, чтобы его показать
// в UI, повесить на клавиши и собрать его патчи.
//
// Удобнее всего заполнять по именам полей (C++20):
//
//   REGISTER_CHEAT({
//       .name    = L"Бессмертие",
//       .keys    = { VKeys::KEY_NUMPAD1 },
//       .patches = { Nop(SIG_DAMAGE) },
//       .hint    = L"Урон по игроку не проходит",
//   })
//
// Поля, которые не нужны, просто не пишутся. Порядок — как в объявлении.
struct CheatDefinition
{
    std::wstring name;                 // подпись в UI
    std::vector<int> keys;             // горячие клавиши (комбинация)
    std::vector<PatchSpec> patches;    // что именно патчим

    // Выключить самому через столько миллисекунд; 0 — держать включённым.
    // Для разовых записей (только WriteValue) это время «вспышки»
    // переключателя; если не задано — берётся короткое по умолчанию.
    int autoOffMs = 0;

    // Модуль, в котором искать сигнатуры и от которого считать адреса:
    // L"GameAssembly.dll". Пусто — сам exe игры.
    std::wstring module;

    // Подсказка при наведении на строку в интерфейсе.
    std::wstring hint;
};
