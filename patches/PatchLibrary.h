#pragma once
#include <string_view>

// Именованные AOB-сигнатуры.
//
// Байты патча здесь больше не лежат: они пишутся прямо в описании чита
// строкой, как копируются из Cheat Engine, и длину считать руками не нужно.
namespace PatchLibrary
{

	// sub [rbx+disp32], edx / mov ecx, [rbx+disp32] / mov r9d, 0xFF / lea r8, [rbp-0x108]
	// ?? — байты, которые меняются от сборки к сборке (disp32).
	inline constexpr std::string_view SIG_CHEAT_TEST_3 =
		"29 93 ** ** ** ** 8B 8B ** ** ** ** 41 B9 FF 00 00 00 4C 8D 85 F8 FE FF FF";

} // namespace PatchLibrary
