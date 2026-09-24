#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "cheats/CheatDefinition.h" // Address, PatchValue

// Поле ввода значения: подпись, адрес и кнопка записи.
//
// Раньше такие поля были захардкожены в Application при старте программы,
// хотя читы объявлялись в реестре. Получалось, что половина содержимого
// панели описана в одном месте, половина — в другом. Теперь и то, и другое
// лежит рядом, в cheats/registry/.
struct ValueFieldDefinition
{
    std::wstring name;                    // подпись в UI
    std::vector<std::uintptr_t> offsets;  // цепочка адреса
    std::wstring module;                  // модуль адреса; пусто — exe игры
    bool absoluteAddress = false;         // offsets.back() — готовый адрес

    // Что показывать в пустом поле. ТИП значения задаёт тип поля:
    // 1 — целое (4 байта), 1.0f — float, 1.0 — double.
    PatchValue defaultValue = std::int32_t{ 1 };

    std::wstring hint;                    // подсказка при наведении
};

// Описание одной строкой, тем же Address, что и у читов:
//
//   REGISTER_VALUE_FIELD(ValueField(L"Золото", Address::Module(0x240600).Deref(0x4B4), 1000))
//   REGISTER_VALUE_FIELD(ValueField(L"Скорость", Address::Module(0x240600).Deref(0x10), 1.0f))
template <typename T = std::int32_t>
inline ValueFieldDefinition ValueField(std::wstring name, const Address& address, T defaultValue = T{ 1 },
                                       std::wstring hint = {})
{
    ValueFieldDefinition field;
    field.name = std::move(name);
    field.offsets = address.offsets;
    field.module = address.module;
    field.absoluteAddress = address.absolute;
    field.defaultValue = detail::ToPatchValue(defaultValue);
    field.hint = std::move(hint);
    return field;
}

// Реестр полей ввода. Устроен так же, как CheatRegistry: записи попадают
// сюда статическими инициализаторами ещё до main().
//
// Порядок внутри одного .cpp гарантирован и определяет порядок в UI;
// между разными .cpp он не определён, поэтому держим всё в одном файле.
class ValueFieldRegistry
{
    std::vector<ValueFieldDefinition> _fields;

    ValueFieldRegistry() = default;

public:
    ValueFieldRegistry(const ValueFieldRegistry&) = delete;
    ValueFieldRegistry& operator=(const ValueFieldRegistry&) = delete;

    // Meyers-синглтон: безопасен во время статической инициализации.
    static ValueFieldRegistry& Instance();

    // Возвращает bool, чтобы результат можно было присвоить статической
    // переменной внутри REGISTER_VALUE_FIELD.
    static bool Register(ValueFieldDefinition field);

    const std::vector<ValueFieldDefinition>& All() const { return _fields; }
    std::size_t Size() const { return _fields.size(); }
};

#define VALUE_FIELD_CONCAT_INNER(a, b) a##b
#define VALUE_FIELD_CONCAT(a, b) VALUE_FIELD_CONCAT_INNER(a, b)

#define REGISTER_VALUE_FIELD(...)                                                  \
    namespace                                                                      \
    {                                                                              \
        const bool VALUE_FIELD_CONCAT(g_valueFieldRegistered_, __LINE__) =         \
            ValueFieldRegistry::Register(__VA_ARGS__);                             \
    }
