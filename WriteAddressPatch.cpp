#include "WriteAddressPatch.h"
#include "CheatOption.h"
#include <iostream>

WriteAddressPatch::WriteAddressPatch()
    : m_processId(0), m_hProcess(nullptr), m_baseAddress(0), m_finalAddress(0), m_isApplied(false)
{
}

DWORD_PTR WriteAddressPatch::DetermineBaseAddress(HANDLE hProcess) const
{
    if (!parent)
    {
        return GetProcessBaseAddress(hProcess);
    }

    const LPCWSTR moduleName = parent->GetModuleName();
    if (!moduleName || wcslen(moduleName) == 0)
	{
        return GetProcessBaseAddress(hProcess);
    }

    return GetModuleBaseAddress(hProcess, moduleName);
}

uintptr_t WriteAddressPatch::CalculateFinalAddress(HANDLE hProcess, DWORD_PTR baseAddr, const std::vector<uintptr_t>& offsets)
{
    if (offsets.empty())
    {
        return 0;
    }

    uintptr_t currentAddress = baseAddr;

    for (size_t i = 0; i < offsets.size() - 1; ++i)
    {
        currentAddress += offsets[i];
        currentAddress = ReadMem(hProcess, currentAddress);

        if (currentAddress == 0)
        {
            ShowErrorMessage(NULL, L"Failed to read memory at offset");
            return 0;
        }
    }

    return currentAddress + offsets.back();
}

template<typename T>
bool WriteAddressPatch::WriteValueMemory(LPCWSTR processName, const std::vector<uintptr_t>& offsets, T value)
{
    if (!InitializeProcess(processName))
    {
        return false;
    }

    m_baseAddress = DetermineBaseAddress(m_hProcess);
    m_finalAddress = CalculateFinalAddress(m_hProcess, m_baseAddress, offsets);

    if (m_finalAddress == 0)
    {
        Cleanup();
        return false;
    }

#ifdef _DEBUG
    std::printf("Final address: 0x%I64X\n", m_finalAddress);
#endif // _DEBUG

    SIZE_T bytesWritten = 0;
    if (!WriteProcessMemory(m_hProcess, (LPVOID)m_finalAddress, &value, sizeof(T), &bytesWritten))
    {
        ShowErrorMessage(NULL, L"Failed to write memory!");
        Cleanup();
        return false;
    }
    
    if (bytesWritten != sizeof(T))
    {
        ShowErrorMessage(NULL, L"Failed to write complete value to memory");
        Cleanup();
        return false;
    }

    Cleanup();
    return true;
}

// Явные инстанциации шаблона для поддерживаемых типов
template bool WriteAddressPatch::WriteValueMemory<int>(LPCWSTR, const std::vector<uintptr_t>&, int);
template bool WriteAddressPatch::WriteValueMemory<float>(LPCWSTR, const std::vector<uintptr_t>&, float);
template bool WriteAddressPatch::WriteValueMemory<double>(LPCWSTR, const std::vector<uintptr_t>&, double);

bool WriteAddressPatch::InitializeProcess(LPCWSTR processName)
{
    m_processId = GetProcessIdByProcessName(processName);
    m_hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, m_processId);

    if (!m_hProcess)
    {
        ShowErrorMessage(NULL, L"Failed to open process");
        return false;
    }

    return true;
}

void WriteAddressPatch::Cleanup()
{
    if (m_hProcess)
    {
        CloseHandle(m_hProcess);
        m_hProcess = nullptr;
    }
}

bool WriteAddressPatch::Hack(HANDLE hProcess)
{
    if (!m_isApplied)
    {
        m_isApplied = true;
        return WriteValueMemory(processName.c_str(), offsets, value);
    }
    return false;
}

bool WriteAddressPatch::Restore(HANDLE hProcess)
{
    if (m_isApplied)
    {
        m_isApplied = false;
        return true;
    }
    return false;
}

WriteAddressPatch::~WriteAddressPatch()
{
    Cleanup();
}