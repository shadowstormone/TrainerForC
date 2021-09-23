#pragma once
#include "Include.h"

class CavePatch : public Patch
{
	LPVOID allocatedAddress = NULL;
	PBYTE patchBytes = NULL;
	BYTE originalSize = 0;
	int caveSize = 0;
	int patchOffset = 0;

	PBYTE CalculateJumpBytes(LPVOID from, LPVOID to, BYTE* outSize);

public:
	CavePatch(CheatOption* parentInstance, LPCWSTR signature, PBYTE pBytes, int pSize) :
		Patch(parentInstance, signature, pSize)
	{
		patchBytes = pBytes;
	}

	bool Hack(HANDLE hProcess);
	bool Restore(HANDLE hProcess);
};