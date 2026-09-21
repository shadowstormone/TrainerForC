#include "core/Relocator.h"

#include <cstring>
#include <format>

#include <Zydis/Zydis.h>

namespace
{
    constexpr std::int64_t INT32_LOW  = -2147483648LL;
    constexpr std::int64_t INT32_HIGH = 2147483647LL;
    constexpr std::int64_t INT8_LOW   = -128;
    constexpr std::int64_t INT8_HIGH  = 127;
}

RelocationResult Relocator::Relocate(std::uintptr_t sourceAddress,
                                     const std::uint8_t* source,
                                     std::size_t sourceSize,
                                     bool is64Bit,
                                     std::uintptr_t caveAddress,
                                     std::size_t minBytes)
{
    RelocationResult result;

    if (source == nullptr || sourceSize == 0 || minBytes == 0)
    {
        result.error = "нечего переносить";
        return result;
    }

    ZydisDecoder decoder;
    const ZyanStatus init = ZydisDecoderInit(
        &decoder,
        is64Bit ? ZYDIS_MACHINE_MODE_LONG_64 : ZYDIS_MACHINE_MODE_LEGACY_32,
        is64Bit ? ZYDIS_STACK_WIDTH_64 : ZYDIS_STACK_WIDTH_32);

    if (!ZYAN_SUCCESS(init))
    {
        result.error = "не удалось инициализировать декодер";
        return result;
    }

    std::size_t offset = 0;

    // Крадём целые инструкции, пока не наберём место под прыжок.
    while (offset < minBytes)
    {
        if (offset >= sourceSize)
        {
            result.error = "прочитанных байт не хватило, чтобы набрать место под прыжок";
            return result;
        }

        ZydisDecodedInstruction insn;
        ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];

        const ZyanStatus decoded = ZydisDecoderDecodeFull(
            &decoder, source + offset, sourceSize - offset, &insn, operands);

        if (!ZYAN_SUCCESS(decoded))
        {
            result.error = std::format("не удалось разобрать инструкцию по смещению {}", offset);
            return result;
        }

        // Где инструкция была и где окажется.
        const std::uint64_t here  = static_cast<std::uint64_t>(sourceAddress) + offset;
        const std::uint64_t there = static_cast<std::uint64_t>(caveAddress) + result.bytes.size();

        std::vector<std::uint8_t> encoded(source + offset, source + offset + insn.length);

        // --- Обращение к памяти через RIP (только x64) ---
        if (is64Bit)
        {
            for (ZyanU8 i = 0; i < insn.operand_count; ++i)
            {
                if (operands[i].type != ZYDIS_OPERAND_TYPE_MEMORY) continue;
                if (operands[i].mem.base != ZYDIS_REGISTER_RIP) continue;

                if (insn.raw.disp.size != 32)
                {
                    result.error = "RIP-обращение с необычным размером смещения";
                    return result;
                }

                const std::uint64_t target = here + insn.length
                                           + static_cast<std::uint64_t>(insn.raw.disp.value);
                const std::int64_t shifted = static_cast<std::int64_t>(target)
                                           - static_cast<std::int64_t>(there + insn.length);

                if (shifted < INT32_LOW || shifted > INT32_HIGH)
                {
                    result.error = "кейв слишком далеко: RIP-обращение оттуда не дотянется";
                    return result;
                }

                const std::int32_t fixed = static_cast<std::int32_t>(shifted);
                std::memcpy(encoded.data() + insn.raw.disp.offset, &fixed, sizeof(fixed));
            }
        }

        // --- Относительные переходы и вызовы ---
        if ((insn.attributes & ZYDIS_ATTRIB_IS_RELATIVE) && insn.raw.imm[0].is_relative)
        {
            const std::uint64_t target = here + insn.length
                                       + static_cast<std::uint64_t>(insn.raw.imm[0].value.s);
            const std::int64_t shifted = static_cast<std::int64_t>(target)
                                       - static_cast<std::int64_t>(there + insn.length);

            switch (insn.raw.imm[0].size)
            {
            case 32:
            {
                if (shifted < INT32_LOW || shifted > INT32_HIGH)
                {
                    result.error = "цель ближнего перехода недостижима из кейва";
                    return result;
                }

                const std::int32_t fixed = static_cast<std::int32_t>(shifted);
                std::memcpy(encoded.data() + insn.raw.imm[0].offset, &fixed, sizeof(fixed));
                break;
            }

            case 8:
            {
                // Короткий переход адресуется одним байтом (±127). Из кейва,
                // лежащего за километры, он почти никогда не достаёт, а у
                // LOOP/JCXZ длинной формы вообще не существует. Раньше такие
                // байты копировались как есть и переход уводил в никуда.
                if (shifted < INT8_LOW || shifted > INT8_HIGH)
                {
                    result.error = "короткий переход не достаёт из кейва — нужна другая точка патча";
                    return result;
                }

                encoded[insn.raw.imm[0].offset] =
                    static_cast<std::uint8_t>(static_cast<std::int8_t>(shifted));
                break;
            }

            default:
                result.error = "относительный переход неподдерживаемого размера";
                return result;
            }
        }

        result.bytes.insert(result.bytes.end(), encoded.begin(), encoded.end());
        offset += insn.length;
    }

    result.stolen = offset;
    result.ok = true;
    return result;
}
