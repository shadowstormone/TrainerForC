#include "WriteAdressNum.h"
#include "CheatOption.h"
#include <iostream>

WriteAddressPatch::WriteAddressPatch()
{
    processId = 0;
    hProcess = nullptr;
    baseAddressProcess = 0;
}

bool WriteAddressPatch::WriteValueMemory(LPCWSTR processName, std::vector<uintptr_t> offsets, int value)
{
	processId = GetProcessIdByProcessName(processName);
	hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
	baseAddressProcess = GetProcessBaseAddress(hProcess);

    if (!hProcess)
    {
        ShowErrorMessage(NULL, L"Failed to open process:\r\nError code: ");
        return false;
    }

    uintptr_t currentAddress = baseAddressProcess;

    for(size_t i = 0; i < offsets.size(); ++i)
    {
        if(i < offsets.size() - 1)
        {
            currentAddress += offsets[i];

            currentAddress = ReadMem(hProcess, currentAddress);
            if (currentAddress == 0)
            {
                ShowErrorMessage(NULL, L"Failed to read memory at offset\r\nError code: ");
                CloseHandle(hProcess);
                return false;
            }
        }
        else
        {
            currentAddress += offsets[i];
        }
    }

#ifdef _DEBUG
    std::printf("Final address: 0x%I64X\n", currentAddress);
#endif // _DEBUG

    int result = WriteMem(hProcess, currentAddress, value);
    if(result != 0)
    {
        ShowErrorMessage(NULL, L"Failed to write memory. \r\nError code: ");
        CloseHandle(hProcess);
        return false;
    }

    CloseHandle(hProcess);
    return true;
}

WriteAddressPatch::~WriteAddressPatch()
{
}