#pragma once
#include <Windows.h>
#include <cstdint>
#include <cwctype>
#include <sstream>
#include <string>
#include <vector>

#include "core/MemoryAccess.h"
#include "patches/IPatch.h"
#include "platform/Logger.h"
#include "platform/Utils.h"

class CheatOption;	// Предварительное объявление класса CheatOption

// Разбирает AOB-сигнатуру в байты + маску ('x' — байт должен совпасть,
// '?' — любой).
//
// Понимает оба стиля записи, чтобы можно было вставлять как есть:
//   из Cheat Engine:  "29 93 ?? ?? ?? ?? 8B 8B"
//   и прежний:        "0x29, 0x93, 0x**, 0x**"
// Джокер — любое из: ?  ??  *  **  xx  XX
// Слитная запись "2993????8B8B" тоже понимается — по два знака на байт.
//
// Раньше "0x**" не подходил ни под один случай джокера, уходил в разбор
// числа, wcstoul возвращал 0 и маска ставилась 'x' — то есть сигнатура
// требовала в этом месте байт 0x00 и никогда не находилась.
//
// Непонятный байт становится джокером (ложное совпадение хуже пропуска),
// а функция возвращает false и объясняет, что не так, в error.
inline bool ParseSignature(LPCWSTR text, std::vector<uint8_t>& pattern, std::wstring& mask, std::string& error)
{
	pattern.clear();
	mask.clear();
	error.clear();

	std::wstring source(text ? text : L"");
	for (wchar_t& c : source)
	{
		if (c == L',' || iswspace(c)) c = L' ';
	}

	const auto addWildcard = [&]() { mask += L'?'; pattern.push_back(0); };
	const auto complain = [&](const std::string& message)
	{
		if (error.empty()) error = message;
	};

	std::wstringstream wss(source);
	std::wstring tok;

	while (wss >> tok)
	{
		// Префикс 0x необязателен.
		if (tok.size() > 2 && tok[0] == L'0' && (tok[1] == L'x' || tok[1] == L'X'))
		{
			tok.erase(0, 2);
		}
		if (tok.empty()) continue;

		if (tok == L"?" || tok == L"*" || tok == L"xx" || tok == L"XX")
		{
			addWildcard();
			continue;
		}

		// Токен длиннее байта — слитная запись: режем по два знака.
		if (tok.size() % 2 != 0 && tok.size() > 1)
		{
			complain("Нечётное число знаков в сигнатуре: " + Utils::WStringToUtf8(tok.c_str()));
			tok += L'?';
		}

		for (std::size_t i = 0; i < tok.size(); i += (tok.size() == 1 ? 1 : 2))
		{
			const std::wstring byte = tok.size() == 1 ? tok : tok.substr(i, 2);

			if (byte.find_first_of(L"?*") != std::wstring::npos)
			{
				addWildcard();
				continue;
			}

			wchar_t* end = nullptr;
			const unsigned long parsed = wcstoul(byte.c_str(), &end, 16);

			if (end == byte.c_str() || *end != 0 || parsed > 0xFF)
			{
				complain("Непонятный байт в сигнатуре — считаю его джокером: "
						 + Utils::WStringToUtf8(byte.c_str()));
				addWildcard();
				continue;
			}

			mask += L'x';
			pattern.push_back(static_cast<uint8_t>(parsed));
		}
	}

	if (pattern.empty()) complain("Сигнатура пуста");
	else if (mask.find(L'x') == std::wstring::npos) complain("Сигнатура из одних джокеров совпадёт где угодно");

	return error.empty();
}

// Общая для патчей по сигнатуре часть: разбор сигнатуры, поиск её в игре,
// оригинальные байты и текст последней ошибки.
// Сам контракт (Apply/Restore) объявлен в IPatch.
class Patch : public IPatch
{
protected:
	std::vector<uint8_t> pattern;
	std::wstring mask;

	// Сдвиг места патча от начала сигнатуры. Сигнатуру удобно брать
	// с запасом ДО нужной инструкции — так она уникальнее.
	std::ptrdiff_t patchOffset = 0;

	LPVOID originalAddress = nullptr;
	std::vector<uint8_t> originalBytes;
	SIZE_T patchSize = 0;
	CheatOption* parent = nullptr;

	// Где нашлась сигнатура в прошлый раз. Сканирование — самая медленная
	// часть включения чита, а место кода между включениями не меняется.
	// Перед повторным использованием адрес перепроверяется.
	DWORD cachedPid = 0;
	uintptr_t cachedMatch = 0;

	std::string lastError;

	// Запоминает причину отказа и пишет её в лог. Возвращает false, чтобы
	// можно было писать return Fail("...").
	bool Fail(const std::string& message)
	{
		lastError = message;
		Log::Error(message);
		return false;
	}

	// Модуль, в котором искать сигнатуру: задаётся у чита, пусто — exe игры.
	std::wstring ModuleName() const;

	// Находит сигнатуру в модуле и возвращает адрес МЕСТА ПАТЧА (с учётом
	// patchOffset). 0 — не найдена; причина уже в lastError.
	uintptr_t Locate(MemoryAccess& mem);

	// Разбирает сигнатуру в pattern/mask (см. ParseSignature). Ошибки — в лог.
	void convertPattern(LPCWSTR sign)
	{
		std::string error;
		if (!ParseSignature(sign, pattern, mask, error))
		{
			Log::Error(error);
		}
	}

public:
	Patch() = default;

	Patch(CheatOption* parentInstance, LPCWSTR signature, SIZE_T pSize, std::ptrdiff_t offset = 0)
		: patchOffset(offset)
		, patchSize(pSize)
		, parent(parentInstance)
	{
		convertPattern(signature);
	}

	bool Apply(MemoryAccess& mem) override = 0;
	bool Restore(MemoryAccess& mem) override = 0;

	void Reset() override
	{
		originalAddress = nullptr;
		originalBytes.clear();
		cachedPid = 0;
		cachedMatch = 0;
	}

	const std::string& LastError() const override { return lastError; }

	CheatOption* GetParent() const { return parent; }
	LPVOID GetOriginalAddress() const { return originalAddress; }
	void SetParent(CheatOption* newParent) { parent = newParent; }
};
