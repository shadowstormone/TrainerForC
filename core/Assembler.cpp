#include "core/Assembler.h"

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

    if (parser.parse(source.c_str()) != asmjit::Error::kOk)
    {
        // Текст разобрать не удалось — почти всегда опечатка в мнемонике
        // или в операнде.
        result.error = "не удалось разобрать ассемблер: " + source;
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
