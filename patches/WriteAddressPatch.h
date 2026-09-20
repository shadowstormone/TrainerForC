#pragma once
#include <Windows.h>
#include <vector>

#include "core/MemoryAccess.h"
#include "patches/Patch.h"

// Запись значения по адресу: либо по цепочке "база модуля + оффсеты",
// либо по готовому абсолютному адресу (как показывает Cheat Engine).
class WriteAddressPatch : public Patch
{
    // Какое из полей базового Patch писать. Раньше тип нигде не хранился,
    // и Hack() всегда писал int — float/double читы молча писали не то.
    enum class ValueType { Int, Float, Double };

    ValueType m_type = ValueType::Int;
    bool      m_absolute = false;
    uintptr_t m_finalAddress = 0;
    bool      m_isApplied = false;

    // Разрешает конечный адрес: абсолютный или по цепочке оффсетов.
    uintptr_t ResolveAddress(MemoryAccess& mem) const;

public:
    // absolute == true: offsets.back() — готовый абсолютный адрес,
    // база модуля к нему не прибавляется.
    WriteAddressPatch(CheatOption* parentInstance, LPCWSTR processName, const std::vector<uintptr_t>& offsets, int value, bool absolute = false)
        : Patch(parentInstance, processName, offsets, value), m_type(ValueType::Int), m_absolute(absolute)
    {
    }

    WriteAddressPatch(CheatOption* parentInstance, LPCWSTR processName, const std::vector<uintptr_t>& offsets, float value, bool absolute = false)
        : Patch(parentInstance, processName, offsets, value), m_type(ValueType::Float), m_absolute(absolute)
    {
    }

    WriteAddressPatch(CheatOption* parentInstance, LPCWSTR processName, const std::vector<uintptr_t>& offsets, double value, bool absolute = false)
        : Patch(parentInstance, processName, offsets, value), m_type(ValueType::Double), m_absolute(absolute)
    {
    }

    bool Apply(MemoryAccess& mem) override;
    bool Restore(MemoryAccess& mem) override;

    bool IsApplied() const { return m_isApplied; }
};
