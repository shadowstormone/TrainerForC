#pragma once
#include <Windows.h>
#include <vector>
#include <string>
#include <iterator>
#include <sstream>
#include <cstdint>
#include "core/MemoryAccess.h"
#include "patches/IPatch.h"
#include "platform/Logger.h"
#include "platform/Utils.h"

class CheatOption;	// Предварительное объявление класса CheatOption

// Общая для всех патчей часть: сигнатура, оригинальные байты, ссылка на опцию.
// Сам контракт (Apply/Restore) объявлен в IPatch.
class Patch : public IPatch
{
protected:
	std::vector<uint8_t> pattern;   // владеет байтами паттерна (было: PBYTE + new[])
	std::wstring mask;
	LPVOID originalAddress = nullptr;
	PBYTE  originalBytes   = nullptr; // аллоцируется ReadMem в наследниках; освобождается ~Patch
	SIZE_T patchSize = 0;
	CheatOption* parent = nullptr;
	LPBYTE patchAddress = nullptr;
	LPVOID patternAddress = nullptr;

	// Дополнительные атрибуты для конструктора с processName, offset и value
	std::wstring processName;
	std::vector<uintptr_t> offsets;
	int value = 0;
	float fvalue = 0;
	double dvalue = 0;

	// Разбирает AOB-сигнатуру в байты + маску.
	//
	// Понимает оба стиля записи, чтобы можно было вставлять как есть:
	//   из Cheat Engine:  "29 93 ?? ?? ?? ?? 8B 8B"
	//   и прежний:        "0x29, 0x93, 0x**, 0x**"
	// Джокер — любое из: ?  ??  *  **  xx  XX
	//
	// Раньше "0x**" не подходил ни под один случай джокера, уходил в разбор
	// числа, wcstoul возвращал 0 и маска ставилась 'x' — то есть сигнатура
	// требовала в этом месте байт 0x00 и никогда не находилась.
	void convertPattern(LPCWSTR sign)
	{
		pattern.clear();
		mask.clear();

		std::wstring text(sign ? sign : L"");
		for (wchar_t& c : text)
		{
			if (c == L',' || iswspace(c)) c = L' ';
		}

		std::wstringstream wss(text);
		std::wstring tok;

		while (wss >> tok)
		{
			// Префикс 0x необязателен.
			if (tok.size() > 2 && tok[0] == L'0' && (tok[1] == L'x' || tok[1] == L'X'))
			{
				tok.erase(0, 2);
			}
			if (tok.empty()) continue;

			if (tok.find_first_of(L"?*") != std::wstring::npos || tok == L"xx" || tok == L"XX")
			{
				mask += L'?';
				pattern.push_back(0);
				continue;
			}

			wchar_t* end = nullptr;
			const unsigned long parsed = wcstoul(tok.c_str(), &end, 16);

			if (end == tok.c_str() || *end != 0 || parsed > 0xFF)
			{
				// Непонятный токен. Берём его как джокер, а не как 0x00:
				// ложное совпадение хуже пропуска. И говорим об этом вслух.
				Log::Error("Непонятный байт в сигнатуре — считаю его джокером: "
						   + Utils::WStringToUtf8(tok.c_str()));
				mask += L'?';
				pattern.push_back(0);
				continue;
			}

			mask += L'x';
			pattern.push_back(static_cast<uint8_t>(parsed));
		}
	}

public:
	Patch(){}

	virtual ~Patch()
	{
		delete[] originalBytes;
		originalBytes = nullptr;
	}

	Patch(CheatOption* parentInstance, LPCWSTR signature, int patchOffset, SIZE_T pSize)
	{
		parent = parentInstance;
		convertPattern(signature);
		patchSize = pSize;
	}

	Patch(CheatOption* parentInstance, LPCWSTR signature, SIZE_T pSize) : 
		Patch(parentInstance, signature, 0 ,pSize) {}

	Patch(CheatOption* parentInstance, LPCWSTR processName, std::vector<uintptr_t> offsets, int value)
	{
		parent = parentInstance;
		this->processName = processName;
		this->offsets = offsets;
		this->value = value;
	}

	Patch(CheatOption* parentInstance, LPCWSTR processName, std::vector<uintptr_t> offsets, float value)
	{
		parent = parentInstance;
		this->processName = processName;
		this->offsets = offsets;
		this->fvalue = value;
	}

	Patch(CheatOption* parentInstance, LPCWSTR processName, std::vector<uintptr_t> offsets, double value)
	{
		parent = parentInstance;
		this->processName = processName;
		this->offsets = offsets;
		this->dvalue = value;
	}

	bool Apply(MemoryAccess& mem) override = 0;
	bool Restore(MemoryAccess& mem) override = 0;

	CheatOption* GetParent() const
	{
		return parent;
	}

	LPVOID GetOriginalAddress() const
	{
		return originalAddress;
	}

	void SetParent(CheatOption* newParent)
	{
		parent = newParent;
	}
};