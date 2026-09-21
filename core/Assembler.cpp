#include "core/Assembler.h"

#include <format>

#include <asmjit/core.h>
#include <asmjit/x86.h>
#include <asmtk/asmtk.h>

AssembleResult Assembler::Assemble(std::string_view text,
                                   bool is64Bit,
                                   std::uintptr_t baseAddress)
{
    AssembleResult result;

    if (text.empty())
    {
        result.error = "пустой текст ассемблера";
        return result;
    }

    asmjit::Environment environment;
    environment.set_arch(is64Bit ? asmjit::Arch::kX64 : asmjit::Arch::kX86);

    asmjit::CodeHolder code;
    if (code.init(environment, static_cast<std::uint64_t>(baseAddress)) != asmjit::Error::kOk)
    {
        result.error = "не удалось подготовить ассемблер";
        return result;
    }

    asmjit::x86::Assembler assembler(&code);
    asmtk::AsmParser parser(&assembler);

    // AsmTK ждёт нуль-терминированную строку.
    const std::string source(text);

    if (const asmjit::Error parsed = parser.parse(source.c_str()); parsed != asmjit::Error::kOk)
    {
        // Говорим, ЧТО именно не понравилось. Самая частая причина —
        // не указан размер операнда: "mov [rbx+0x800], 1000" неоднозначен,
        // ассемблер не знает, писать 4 байта или 8. Нужно писать
        // "mov qword ptr [rbx+0x800], 1000" (или dword).
        result.error = std::format("{} — в строке: {}",
                                   asmjit::stringify_error(parsed), source);
        return result;
    }

    if (code.flatten() != asmjit::Error::kOk)
    {
        result.error = "не удалось разложить код";
        return result;
    }

    const asmjit::CodeBuffer& buffer = code.text_section()->buffer();
    if (buffer.size() == 0)
    {
        result.error = "ассемблер не выдал ни одного байта";
        return result;
    }

    result.bytes.assign(buffer.data(), buffer.data() + buffer.size());
    result.ok = true;
    return result;
}
