#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <cctype>
#include <cstdlib>
#include <variant>
#include <vector>

// Описание патча — это ДАННЫЕ, а не объект. Конкретный Patch собирается
// из PatchSpec фабрикой (CheatFactory), поэтому добавление нового типа
// патча = новый класс + одна ветка в фабрике.
struct PatchSpec
{
    enum class Kind
    {
        Nop,
        Cave,
        WriteValue
    };

    Kind kind = Kind::Nop;

    // Nop / Cave
    std::string signature;                 // AOB-сигнатура (utf-8)
    std::size_t length = 0;                // для Nop: сколько байт занопить
    std::vector<std::uint8_t> patchBytes;  // для Cave: сами байты; длина = size()

    // WriteValue
    std::vector<std::uintptr_t> offsets;
    std::variant<int, float, double> value{};

    // true  — offsets.back() это готовый абсолютный адрес (как показывает
    //         Cheat Engine для динамического значения);
    // false — цепочка "база модуля + оффсеты".
    bool absoluteAddress = false;
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
        if (!token.empty())
            bytes.push_back(static_cast<std::uint8_t>(std::strtoul(token.c_str(), nullptr, 16)));
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

// Адрес значения — читается так же, как его находишь в Cheat Engine.
//
//   Address::Module(0x346C10).Deref(0x800)
//     = взять базу модуля, прибавить 0x346C10, РАЗЫМЕНОВАТЬ, прибавить 0x800
//
// Раньше приходилось писать {0x346C10, 0x800} и держать в голове неписаное
// правило «все оффсеты кроме последнего разыменовываются».
struct Address
{
    std::vector<std::uintptr_t> offsets;
    bool absolute = false;

    // База модуля + offset.
    static Address Module(std::uintptr_t offset)
    {
        Address a;
        a.offsets.push_back(offset);
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
};

// --- Хелперы: чтобы описание чита читалось одной строкой ---

inline PatchSpec Nop(std::string_view signature, std::size_t length)
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

// Перегрузка для массивов из PatchLibrary: Cave(SIG_X, PATCH_X)
template <std::size_t N>
inline PatchSpec Cave(std::string_view signature, const std::array<std::uint8_t, N>& bytes)
{
    return Cave(signature, std::vector<std::uint8_t>(bytes.begin(), bytes.end()));
}

inline PatchSpec WriteValue(std::vector<std::uintptr_t> offsets, int value)
{
    PatchSpec s;
    s.kind = PatchSpec::Kind::WriteValue;
    s.offsets = std::move(offsets);
    s.value = value;
    return s;
}

inline PatchSpec WriteValue(std::vector<std::uintptr_t> offsets, float value)
{
    PatchSpec s;
    s.kind = PatchSpec::Kind::WriteValue;
    s.offsets = std::move(offsets);
    s.value = value;
    return s;
}

inline PatchSpec WriteValue(std::vector<std::uintptr_t> offsets, double value)
{
    PatchSpec s;
    s.kind = PatchSpec::Kind::WriteValue;
    s.offsets = std::move(offsets);
    s.value = value;
    return s;
}

// Запись по адресу, описанному через Address — предпочтительная форма.
template <typename T>
inline PatchSpec WriteValue(const Address& address, T value)
{
    PatchSpec s = WriteValue(address.offsets, value);
    s.absoluteAddress = address.absolute;
    return s;
}

// Запись по готовому абсолютному адресу — тому, что показывает Cheat Engine.
// Внимание: у динамических значений такой адрес меняется от запуска к запуску,
// поэтому это годится для проверки, но не для готового чита.
template <typename T>
inline PatchSpec WriteValueAt(std::uintptr_t address, T value)
{
    PatchSpec s = WriteValue(std::vector<std::uintptr_t>{ address }, value);
    s.absoluteAddress = true;
    return s;
}

// Полное описание одного чита. Всё, что нужно знать, чтобы его показать
// в UI, повесить на клавиши и собрать его патчи.
struct CheatDefinition
{
    std::wstring name;                 // подпись в UI
    std::vector<int> keys;             // горячие клавиши
    std::vector<PatchSpec> patches;    // что именно патчим
    bool autoDisable = false;          // выключить сам через autoDisableDelay мс
    int autoDisableDelay = 0;
};
