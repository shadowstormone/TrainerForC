#pragma once
#include "patches/Patch.h"

class CavePatch : public Patch
{
    LPVOID allocatedAddress = nullptr;
    PBYTE patchBytes = nullptr;
    BYTE originalSize = 0;
    int caveSize = 0;
    int patchOffset = 0;

public:
    CavePatch(CheatOption* parentInstance, LPCWSTR signature, PBYTE pBytes, int pSize) :
        Patch(parentInstance, signature, pSize)
    {
        patchBytes = new BYTE[pSize];
        memcpy(patchBytes, pBytes, pSize);
    }

    ~CavePatch()
    {
        delete[] patchBytes;
    }

    static PBYTE CalculateJumpBytes(LPVOID from, LPVOID to, BYTE& outSize);

    bool Apply(MemoryAccess& mem) override;
    bool Restore(MemoryAccess& mem) override;
};

//class CavePatch : public Patch
//{
//	LPVOID allocatedAddress = NULL;
//	PBYTE patchBytes = NULL;
//	BYTE originalSize = 0;
//	int caveSize = 0;
//	int patchOffset = 0;
//
//	//PBYTE CalculateJumpBytes(LPVOID from, LPVOID to, BYTE& outSize, HANDLE hProcess);
//	PBYTE CalculateJumpBytes(LPVOID from, LPVOID to, BYTE& outSize);
//
//public:
//	CavePatch(CheatOption* parentInstance, LPCWSTR signature, PBYTE pBytes, int pSize) :
//		Patch(parentInstance, signature, pSize)
//	{
//		patchBytes = pBytes;
//	}
//
//
//	bool Apply(MemoryAccess& mem) override;
//	bool Restore(MemoryAccess& mem) override;
//};