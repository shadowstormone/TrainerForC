#pragma once

// Прослойка между AsmTK и нынешним AsmJit.
//
// AsmTK написан под AsmJit, где служебные функции жили в namespace Support.
// В текущем AsmJit они переехали в namespace axl, а несколько устаревших
// возможностей x86 удалены вовсе: BND-регистры и префиксы xacquire/xrelease
// (Intel MPX и HLE — обе технологии отменены). Для патчей в играх ничего из
// этого не нужно.
//
// Прослойка позволяет не трогать исходники AsmTK. Риск версионного разрыва
// закрыт проверками: то, что собрал ассемблер, разбирается обратно Zydis и
// сверяется с ожидаемыми байтами.

#include <asmjit/core.h>

namespace asmjit
{
namespace Support
{
    // Почти всё, что нужно AsmTK, лежит теперь здесь под теми же именами.
    using namespace axl;

    // Единственное, чему в axl нет замены: маска из n младших бит.
    template <typename T>
    constexpr T lsb_mask(size_t n) noexcept
    {
        return n >= sizeof(T) * 8u ? T(~T(0)) : T((T(1) << n) - T(1));
    }
}
} // namespace asmjit
