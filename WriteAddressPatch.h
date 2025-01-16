#pragma once
#include <Windows.h>
#include <vector>
#include <memory>
#include "Memory_Functions.h"
#include "Patch.h"

class WriteAddressPatch : public Patch
{
public:
    // Конструктор для базовой инициализации
    WriteAddressPatch();

    // Конструкторы для различных типов значений, использующие списки инициализаторов членов
    WriteAddressPatch( CheatOption* parentInstance, LPCWSTR processName, const std::vector<uintptr_t>& offsets, int value) 
        : Patch(parentInstance, processName, offsets, value), m_hProcess(nullptr), m_finalAddress(0), m_isApplied(false)
    {
    }

    WriteAddressPatch(CheatOption* parentInstance, LPCWSTR processName, const std::vector<uintptr_t>& offsets, float value) 
        : Patch(parentInstance, processName, offsets, value), m_hProcess(nullptr), m_finalAddress(0), m_isApplied(false)
    {
    }

    WriteAddressPatch(CheatOption* parentInstance, LPCWSTR processName, const std::vector<uintptr_t>& offsets, double value) 
        : Patch(parentInstance, processName, offsets, value), m_hProcess(nullptr), m_finalAddress(0), m_isApplied(false)
    {
    }

    // Виртуальный деструктор
    virtual ~WriteAddressPatch();

    // Переопределяйте методы базового класса Patch (удаляя ключевое слово override, если оно отсутствует в базовом классе)
    virtual bool Hack(HANDLE hProcess);
    virtual bool Restore(HANDLE hProcess);

    // Публичные методы
    bool IsApplied() const
    {
        return m_isApplied;
    }

    template<typename T>
    bool WriteValueMemory(LPCWSTR processName, const std::vector<uintptr_t>& offsets, T value);
private:
    // Приватные переменные-члены
    HANDLE m_hProcess = 0;
    DWORD m_processId = 0;
    DWORD_PTR m_baseAddress = NULL;
    uintptr_t m_finalAddress;
    bool m_isApplied;

    // Приватные вспомагательные методы
    DWORD_PTR DetermineBaseAddress(HANDLE hProcess) const;
    uintptr_t CalculateFinalAddress(HANDLE hProcess, DWORD_PTR baseAddr, const std::vector<uintptr_t>& offsets);
    bool InitializeProcess(LPCWSTR processName);
    void Cleanup();
};