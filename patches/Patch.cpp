#include "patches/Patch.h"

#include <format>

#include "cheats/CheatOption.h"

std::wstring Patch::ModuleName() const
{
	return parent ? parent->GetModuleName() : std::wstring();
}

uintptr_t Patch::Locate(MemoryAccess& mem)
{
	if (pattern.empty())
	{
		Fail("Сигнатура пуста");
		return 0;
	}

	// Та же игра, что и в прошлый раз, и байты на месте всё те же — второй
	// раз не сканируем. Проверка обязательна: игра могла перезаписать код
	// или выгрузить модуль.
	if (cachedMatch != 0 && cachedPid == mem.Pid())
	{
		const std::vector<uint8_t> current = mem.ReadBytes(cachedMatch, pattern.size());
		if (!current.empty() && MemoryAccess::FindPattern(current.data(), current.size(), pattern, mask) == 0)
		{
			return cachedMatch + patchOffset;
		}
	}

	cachedMatch = 0;

	const std::wstring moduleName = ModuleName();

	// "*" — вся исполняемая память процесса. Для кода, который живёт вне
	// модулей: JIT Unity/Mono, распакованные протекторы.
	const bool everywhere = (moduleName == L"*");

	const MemoryAccess::ModuleInfo module = everywhere
		? MemoryAccess::WholeAddressSpace()
		: mem.ModuleOrMain(moduleName);
	if (!module)
	{
		Fail(moduleName.empty()
			? std::string("Не удалось получить адрес модуля игры")
			: "Модуль не найден в игре: " + Utils::WStringToUtf8(moduleName));
		return 0;
	}

	const std::string where = everywhere ? std::string("памяти игры")
		: moduleName.empty() ? std::string("модуле игры")
		: Utils::WStringToUtf8(moduleName);

	// Ищем внутри модуля — там живёт код, и это быстро. Раньше сканировалось
	// всё адресное пространство от базы, вплоть до 0x7FFFFFFFFFFFFFFF.
	const uintptr_t end = module.base + module.size;
	const uintptr_t match = mem.ScanSignature(module.base, module.size, pattern, mask, everywhere);

	if (match == 0)
	{
		Fail("Сигнатура не найдена в " + where + ": другая версия игры, или это место "
			"уже занято другим включённым читом");
		return 0;
	}

	// Сигнатура должна быть уникальной: второе совпадение значит, что патч
	// может лечь не туда. Не отказываем — так бывает и с рабочими
	// сигнатурами, — но предупреждаем автора.
	const uintptr_t next = match + 1;
	if (next < end && mem.ScanSignature(next, end - next, pattern, mask, everywhere) != 0)
	{
		Log::Error(std::format("Сигнатура встречается в {} больше одного раза — "
			"беру первое совпадение 0x{:X}. Удлините её для надёжности.", where, match));
	}

	cachedPid = mem.Pid();
	cachedMatch = match;
	return match + patchOffset;
}
