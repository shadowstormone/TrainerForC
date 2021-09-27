#pragma once
#include "Patch.h"

class NopPatch : public Patch
{
public:
	NopPatch(CheatOption* parentInstance, LPCWSTR signature, SIZE_T pSize) : 
		Patch(parentInstance, signature, pSize) {}

	bool Hack(HANDLE hProcess);
	bool Restore(HANDLE hProcess);
};