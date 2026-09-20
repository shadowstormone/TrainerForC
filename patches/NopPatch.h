#pragma once
#include "patches/Patch.h"

class NopPatch : public Patch
{
public:
	NopPatch(CheatOption* parentInstance, LPCWSTR signature, SIZE_T pSize) : 
		Patch(parentInstance, signature, pSize) {}

	bool Apply(MemoryAccess& mem) override;
	bool Restore(MemoryAccess& mem) override;
};