#pragma once
#include "SimpleRenderer.h"
#include <gtest/gtest.h>

constexpr auto W_WIDTH = 400;
constexpr auto W_HEIGHT = 444;

extern LPCWSTR WindowTitle; // Объявление, а не определение

// Функция для запуска тестов
void RunTests()
{
	int argc = 0;
	char** argv = nullptr;
	::testing::InitGoogleTest(&argc, argv);
	RUN_ALL_TESTS();
}
