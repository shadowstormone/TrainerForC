#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "cheats/CheatDefinition.h" // Address

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
    bool absoluteAddress = false;         // offsets.back() — готовый адрес
    int defaultValue = 1;                 // что показывать в пустом поле
};

// Описание одной строкой, тем же Address, что и у читов:
//
//   REGISTER_VALUE_FIELD(ValueField(L"Set HP", Address::Module(0x240600).Deref(0x4B4)))
inline ValueFieldDefinition ValueField(std::wstring name, const Address& address, int defaultValue = 1)
{
    ValueFieldDefinition field;
    field.name = std::move(name);
    field.offsets = address.offsets;
    field.absoluteAddress = address.absolute;
    field.defaultValue = defaultValue;
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
