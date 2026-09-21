#pragma once
#include <Windows.h>

constexpr auto W_WIDTH = 400;
constexpr auto W_HEIGHT = 444;

extern LPCWSTR WindowTitle; // Объявление, а не определение

// Тесты — только в отладочной сборке.
//
// Иначе релизный трейнер тянул бы за собой gtest и gmock: их библиотеки
// подставляет автолинковка vcpkg, и рядом с exe приходилось держать
// gmock.dll — при том, что тесты в релизе не запускаются никогда.
#ifdef _DEBUG

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "platform/Utils.h" // WStringToUtf8

// Запуск тестов. Возвращает код выхода gtest (0 — все тесты прошли),
// чтобы результат был виден снаружи: приложение собрано как Windows-
// подсистема и консольного вывода не имеет.
inline int RunTests()
{
	// Прокидываем настоящую командную строку. Раньше сюда передавались
	// argc=0 и argv=nullptr, поэтому флаги gtest (--gtest_filter,
	// --gtest_output=xml:...) молча игнорировались — а без консольного
	// вывода это единственный способ узнать, КАКОЙ тест упал.
	std::vector<std::string> storage;
	storage.reserve(static_cast<std::size_t>(__argc));

	for (int i = 0; i < __argc; ++i)
	{
		storage.push_back(Utils::WStringToUtf8(__wargv[i]));
	}

	std::vector<char*> argv;
	argv.reserve(storage.size() + 1);
	for (std::string& arg : storage)
	{
		argv.push_back(arg.data());
	}
	argv.push_back(nullptr);

	int argc = static_cast<int>(storage.size());
	::testing::InitGoogleTest(&argc, argv.data());
	return RUN_ALL_TESTS();
}

#endif // _DEBUG
