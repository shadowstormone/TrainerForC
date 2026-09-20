#pragma once
#include <gtest/gtest.h>
#include <Windows.h>

constexpr auto W_WIDTH = 400;
constexpr auto W_HEIGHT = 444;

extern LPCWSTR WindowTitle; // Объявление, а не определение

// Запуск тестов. Возвращает код выхода gtest (0 — все тесты прошли),
// чтобы результат был виден снаружи: приложение собрано как Windows-
// подсистема и консольного вывода не имеет.
inline int RunTests()
{
	int argc = 0;
	char** argv = nullptr;
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}