#pragma once
#include "patches/Patch.h"

// Забивает инструкции NOP-ами.
//
// Длина 0 — «ровно первую инструкцию по адресу, сколько бы байт она ни
// занимала»: её посчитает дизассемблер. Не нужно сверять длину в CE.
class NopPatch : public Patch
{
	bool m_applied = false;

public:
	NopPatch(CheatOption* parentInstance, LPCWSTR signature, SIZE_T pSize, std::ptrdiff_t offset = 0)
		: Patch(parentInstance, signature, pSize, offset) {}

	bool Apply(MemoryAccess& mem) override;
	bool Restore(MemoryAccess& mem) override;

	void Reset() override
	{
		Patch::Reset();
		m_applied = false;
	}
};
