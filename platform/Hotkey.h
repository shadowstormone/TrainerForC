#pragma once
#include <atomic>
#include <vector>

// Одна горячая комбинация клавиш.
//
// Отдаёт СОБЫТИЕ нажатия (фронт), а не состояние удержания, поэтому опция
// переключается ровно один раз на нажатие. Состояние хранится в объекте,
// а не в функциональном статике: раньше флаг антидребезга был общим для
// всех опций сразу, и ветка "клавиша отпущена" одной опции сбрасывала его
// для остальных — антидребезг практически не работал.
class Hotkey
{
    std::vector<int> _keys;
    bool _wasDown = false;

    bool AllKeysDown() const;

public:
    Hotkey() = default;
    explicit Hotkey(std::vector<int> keys) : _keys(std::move(keys)) {}

    void SetKeys(std::vector<int> keys)
    {
        _keys = std::move(keys);
        _wasDown = false;
    }

    bool HasKeys() const { return !_keys.empty(); }

    // true ровно один раз на каждое нажатие комбинации.
    bool JustPressed();

    // Заглушить все горячие клавиши, пока в окне трейнера вводят текст:
    // иначе число, набранное на цифровой клавиатуре в поле значения,
    // переключало читы на Num 1..Num 9.
    static void SetSuppressed(bool suppressed) { s_suppressed = suppressed; }

private:
    static inline std::atomic<bool> s_suppressed{ false };
};
