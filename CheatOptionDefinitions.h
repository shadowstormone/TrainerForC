#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "Cheat.h"
#include "VKeys.h"
#include "Utils.h"

// Центральное место для определения всех опций читов
namespace CheatOptionDefinitions
{
    // Перечисление идентификаторов опций (для безопасных с точки зрения типа ссылок на опции)
    enum class OptionID
    {
        CHEAT_TEST_1,
        SET_HP_9999,
        CHEAT_TEST_3,
        FIRST_FUNCTION_NOP
    };

    // Структура определения опций
    struct OptionDefinition
    {
        OptionID id;                   // Уникальный идентификатор
        std::wstring name;             // Отображаемое имя
        std::vector<int> keys;         // Привязка клавиш
        bool autoDisable;              // Должна ли опция автоматически отключаться
        int autoDisableDelay;          // Задержка в мс для автоматического отключения (если применимо)

        OptionDefinition(OptionID id, const std::wstring& name, const std::vector<int>& keys, bool autoDisable = false, int autoDisableDelay = 0)
            : id(id), name(name), keys(keys), autoDisable(autoDisable), autoDisableDelay(autoDisableDelay){}
    };

    // Определение всех опций
    const std::vector<OptionDefinition> AllOptions = {
        {OptionID::CHEAT_TEST_1, L"[Numpad 1] - Cheat Test", {VKeys::KEY_NUMPAD1}, false},
        {OptionID::SET_HP_9999, L"[Numpad 2] - Set 9999 HP", {VKeys::KEY_NUMPAD2}, true, 450},
        {OptionID::CHEAT_TEST_3, L"[Numpad 3] - Cheat Test 3", {VKeys::KEY_NUMPAD3}, false},
        {OptionID::FIRST_FUNCTION_NOP, L"[Numpad 4] - First Function(Nop)", {VKeys::KEY_NUMPAD4}, false}
    };

    // Вспомогательная функция для получения опции по идентификатору (by ID)
    inline const OptionDefinition& GetOptionById(OptionID id)
    {
        for (const auto& option : AllOptions)
        {
            if (option.id == id) return option;
        }

        // При правильном использовании этого не должно произойти
        throw std::runtime_error("Option ID not found");
    }

    // Вспомогательная функция для получения опции по имени (by name)
    inline OptionID GetOptionIdByName(const std::string& name)
    {
        for (const auto& option : AllOptions)
        {
            std::string optionName = Utils::WStringToUtf8(option.name);
            if (optionName == name) return option.id;
        }

        // При правильном использовании этого не должно произойти
        throw std::runtime_error("Option name not found");
    }
}