#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
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
    std::string signature;                    // AOB-сигнатура (utf-8)
    std::size_t length = 0;                   // сколько байт затрагиваем
    const std::uint8_t* patchData = nullptr;  // байты для Cave (статическое время жизни)

    // WriteValue
    std::vector<std::uintptr_t> offsets;
    std::variant<int, float, double> value{};

    // true  — offsets.back() это готовый абсолютный адрес (как показывает
    //         Cheat Engine для динамического значения);
    // false — цепочка "база модуля + оффсеты".
    bool absoluteAddress = false;
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

inline PatchSpec Cave(std::string_view signature, const std::uint8_t* data, std::size_t size)
{
    PatchSpec s;
    s.kind = PatchSpec::Kind::Cave;
    s.signature = std::string(signature);
    s.patchData = data;
    s.length = size;
    return s;
}

// Удобная перегрузка для массивов из PatchLibrary: Cave(SIG_X, PATCH_X)
template <std::size_t N>
inline PatchSpec Cave(std::string_view signature, const std::array<std::uint8_t, N>& bytes)
{
    return Cave(signature, bytes.data(), N);
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
