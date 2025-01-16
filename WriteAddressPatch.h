#pragma once
#include <Windows.h>
#include <vector>
#include <memory>
#include "Memory_Functions.h"
#include "Patch.h"

class WriteAddressPatch : public Patch {
public:
    // Constructor for basic initialization
    WriteAddressPatch();

    // Constructors for different value types using member initializer lists
    WriteAddressPatch(
        CheatOption* parentInstance,
        LPCWSTR processName,
        const std::vector<uintptr_t>& offsets,
        int value
    ) : Patch(parentInstance, processName, offsets, value)
        , m_hProcess(nullptr)
        , m_finalAddress(0)
        , m_isApplied(false) {
    }

    WriteAddressPatch(
        CheatOption* parentInstance,
        LPCWSTR processName,
        const std::vector<uintptr_t>& offsets,
        float value
    ) : Patch(parentInstance, processName, offsets, value)
        , m_hProcess(nullptr)
        , m_finalAddress(0)
        , m_isApplied(false) {
    }

    WriteAddressPatch(
        CheatOption* parentInstance,
        LPCWSTR processName,
        const std::vector<uintptr_t>& offsets,
        double value
    ) : Patch(parentInstance, processName, offsets, value)
        , m_hProcess(nullptr)
        , m_finalAddress(0)
        , m_isApplied(false) {
    }

    // Virtual destructor
    virtual ~WriteAddressPatch();

    // Override methods from Patch base class (removing override keyword if not in base)
    virtual bool Hack(HANDLE hProcess);
    virtual bool Restore(HANDLE hProcess);

    // Public methods
    bool IsApplied() const
    {
        return m_isApplied;
    }

    template<typename T>
    bool WriteValueMemory(LPCWSTR processName, const std::vector<uintptr_t>& offsets, T value);
private:
    // Private member variables
    HANDLE m_hProcess = 0;
    DWORD m_processId = 0;
    DWORD_PTR m_baseAddress = NULL;
    uintptr_t m_finalAddress;
    bool m_isApplied;

    // Private helper methods
    DWORD_PTR DetermineBaseAddress(HANDLE hProcess) const;
    uintptr_t CalculateFinalAddress(HANDLE hProcess, DWORD_PTR baseAddr, const std::vector<uintptr_t>& offsets);
    bool InitializeProcess(LPCWSTR processName);
    void Cleanup();
};