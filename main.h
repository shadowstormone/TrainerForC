#pragma once
#include "SimpleRenderer.h"
#include <gtest/gtest.h>

#define W_WIDTH 400
#define W_HEIGHT 444

extern LPCWSTR WindowTitle; // ����������, � �� �����������

// ������� ��� ������� ������
void RunTests()
{
	int argc = 0;
	char** argv = nullptr;
	::testing::InitGoogleTest(&argc, argv);
	RUN_ALL_TESTS();
}
