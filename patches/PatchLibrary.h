#pragma once
#include <string_view>

// Именованные AOB-сигнатуры.
//
// Сигнатура пишется как её показывает Cheat Engine: "29 93 ?? ?? 8B".
// Джокер — ?? (или ?, *, **). Байты, которые меняются от сборки к сборке
// (смещения disp32, адреса), заменяйте джокерами.
//
// Проверить сигнатуру на живой игре: консоль (клавиша ` в отладке),
// команда aob 29 93 ?? ?? 8B — покажет все совпадения.
namespace PatchLibrary
{

	// sub [rbx+disp32], edx / mov ecx, [rbx+disp32] / mov r9d, 0xFF / lea r8, [rbp-0x108]
	inline constexpr std::string_view SIG_CHEAT_TEST_3 =
		"29 93 ?? ?? ?? ?? 8B 8B ?? ?? ?? ?? 41 B9 FF 00 00 00 4C 8D 85 F8 FE FF FF";

} // namespace PatchLibrary
