#include "WriteAdressNum.h"

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

    //std::cout << "Base address: 0x" << std::hex << baseAddressProcess << std::endl;

    uintptr_t currentAddress = baseAddressProcess;

    for (size_t i = 0; i < offsets.size(); ++i)
    {
        if (i < offsets.size() - 1)
        {
            currentAddress += offsets[i];
            //std::cout << "Address after offset " << i << ": 0x" << std::hex << currentAddress << std::endl;

            currentAddress = ReadMem(hProcess, currentAddress);
            if (currentAddress == 0)
            {
                ShowErrorMessage(NULL, L"Failed to read memory at offset\r\nError code: ");
                CloseHandle(hProcess);
                return false;
            }
            //std::cout << "Value read: 0x" << std::hex << currentAddress << std::endl;
        }
        else
        {
            currentAddress += offsets[i];
        }
    }

    //std::cout << "Final address: 0x" << std::hex << currentAddress << std::endl;

    int result = WriteMem(hProcess, currentAddress, value);
    if (result != 0)
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