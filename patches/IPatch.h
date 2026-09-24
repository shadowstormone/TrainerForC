#pragma once
#include <string>

class MemoryAccess;

// Что умеет любой патч: примениться к процессу и откатиться.
//
// Работает с MemoryAccess, а не с сырым HANDLE: патч не открывает процесс
// сам и не может забыть закрыть дескриптор — этим владеет вызывающий.
class IPatch
{
public:
    virtual ~IPatch() = default;

    virtual bool Apply(MemoryAccess& mem) = 0;
    virtual bool Restore(MemoryAccess& mem) = 0;

    // Одноразовое действие (записать значение), а не состояние: включать
    // и держать включённым тут нечего. Такие опции UI рисует кнопкой-вспышкой.
    virtual bool IsOneShot() const { return false; }

    // Вызывается фоновым потоком ~60 раз в секунду, пока опция включена.
    // Нужен патчам, которые поддерживают состояние сами (заморозка значения).
    virtual void Tick(MemoryAccess& mem) { (void)mem; }

    // Игра закрылась или перезапустилась: всё, что патч помнил о прежнем
    // процессе (адреса, кейвы, оригинальные байты), больше недействительно.
    // Память при этом НЕ трогается — её уже нет.
    virtual void Reset() {}

    // Почему последний Apply/Restore не удался — для подсказки в UI.
    virtual const std::string& LastError() const = 0;
};
