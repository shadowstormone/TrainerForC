#pragma once
#include <Windows.h>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include "core/MemoryAccess.h"
#include "patches/IPatch.h"

class CheatOption;

// Значение, которое пишется в память игры. Тип определяет, сколько байт
// и в каком формате записать.
using PatchValue = std::variant<std::int32_t, float, double, std::int64_t, std::uint8_t, std::int16_t>;

// Запись значения по адресу: либо по цепочке "база модуля + оффсеты",
// либо по готовому абсолютному адресу (как показывает Cheat Engine).
//
// Два режима:
//   OneShot — записать один раз (кнопка «выдать 9999 золота»);
//   Freeze  — держать значение, пока чит включён (бесконечное здоровье):
//             фоновый поток перезаписывает его каждый кадр, как галочка
//             заморозки в Cheat Engine. Адрес пересчитывается каждый раз —
//             объект в игре может переехать.
class WriteAddressPatch : public IPatch
{
public:
    enum class Mode { OneShot, Freeze };

private:
    CheatOption* m_parent = nullptr;
    std::vector<std::uintptr_t> m_offsets;
    std::wstring m_module;      // пусто — модуль чита, а если и он пуст — exe игры
    PatchValue m_value;
    bool m_absolute = false;
    Mode m_mode = Mode::OneShot;

    bool m_active = false;      // для Freeze: включён ли
    std::string m_lastError;

    // База модуля за время жизни процесса не меняется, а снимок модулей
    // дорог — заморозка иначе делала бы его 60 раз в секунду.
    DWORD m_basePid = 0;
    std::uintptr_t m_base = 0;

    // Разрешает конечный адрес: абсолютный или по цепочке оффсетов.
    std::uintptr_t ResolveAddress(MemoryAccess& mem);

    // Пишет значение; false — адрес не разрешился или запись не прошла.
    bool WriteNow(MemoryAccess& mem, bool logErrors);

public:
    // absolute == true: offsets.back() — готовый абсолютный адрес,
    // база модуля к нему не прибавляется.
    WriteAddressPatch(CheatOption* parent, std::vector<std::uintptr_t> offsets, PatchValue value,
                      bool absolute = false, Mode mode = Mode::OneShot, std::wstring module = {})
        : m_parent(parent)
        , m_offsets(std::move(offsets))
        , m_module(std::move(module))
        , m_value(value)
        , m_absolute(absolute)
        , m_mode(mode)
    {
    }

    bool Apply(MemoryAccess& mem) override;
    bool Restore(MemoryAccess& mem) override;
    void Tick(MemoryAccess& mem) override;
    void Reset() override
    {
        m_active = false;
        m_basePid = 0;
        m_base = 0;
    }

    bool IsOneShot() const override { return m_mode == Mode::OneShot; }
    const std::string& LastError() const override { return m_lastError; }

    // Сколько байт занимает значение этого типа.
    static std::size_t ValueSize(const PatchValue& value);
};
