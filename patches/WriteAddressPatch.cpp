#include "patches/WriteAddressPatch.h"

#include <format>

#include "cheats/CheatOption.h"
#include "platform/Logger.h"

uintptr_t WriteAddressPatch::ResolveAddress(MemoryAccess& mem) const
{
    if (offsets.empty()) return 0;

    // Адрес уже абсолютный — база не прибавляется.
    if (m_absolute) return offsets.back();

    const LPCWSTR moduleName = parent ? parent->GetModuleName() : nullptr;
    const DWORD_PTR base = (moduleName && wcslen(moduleName) > 0)
                               ? mem.ModuleBase(moduleName)
                               : mem.ProcessBase();
    if (base == 0) return 0;

    return mem.ResolveChain(base, offsets);
}

bool WriteAddressPatch::Apply(MemoryAccess& mem)
{
    // Процесс больше не открывается здесь: раньше патч игнорировал
    // переданный дескриптор и делал собственный OpenProcess.
    if (!mem.IsValid()) return false;
    if (m_isApplied) return false;

    m_finalAddress = ResolveAddress(mem);
    if (m_finalAddress == 0)
    {
        Log::Error("Не удалось вычислить адрес для записи значения");
        return false;
    }

#ifdef _DEBUG
    Log::Debug(std::format("Финальный адрес значения: 0x{:X}", m_finalAddress));
#endif

    const void* source = nullptr;
    SIZE_T size = 0;

    switch (m_type)
    {
    case ValueType::Int:    source = &value;  size = sizeof(value);  break;
    case ValueType::Float:  source = &fvalue; size = sizeof(fvalue); break;
    case ValueType::Double: source = &dvalue; size = sizeof(dvalue); break;
    }

    SIZE_T written = 0;
    if (!WriteProcessMemory(mem.Handle(), reinterpret_cast<LPVOID>(m_finalAddress), source, size, &written)
        || written != size)
    {
        Log::Error("Не удалось записать значение в память");
        return false;
    }

    // Флаг ставим только при успехе: раньше он взводился до записи и врал,
    // если запись не удалась.
    m_isApplied = true;
    return true;
}

bool WriteAddressPatch::Restore(MemoryAccess& mem)
{
    // Это одноразовая запись значения: прежнее значение не сохранялось,
    // поэтому «откат» лишь снимает флаг применённости.
    (void)mem;

    if (!m_isApplied) return false;

    m_isApplied = false;
    return true;
}
