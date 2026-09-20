#pragma once

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
};
