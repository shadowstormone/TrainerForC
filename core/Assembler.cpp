#include "core/Assembler.h"

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <format>

#include <asmjit/core.h>
#include <asmjit/x86.h>
#include <asmtk/asmtk.h>

namespace
{
    bool IsWordChar(char c)
    {
        return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_';
    }

    bool IsHexDigits(std::string_view s)
    {
        if (s.empty()) return false;
        for (char c : s)
        {
            if (std::isxdigit(static_cast<unsigned char>(c)) == 0) return false;
        }
        return true;
    }

    std::string Trim(std::string_view s)
    {
        std::size_t b = 0;
        std::size_t e = s.size();
        while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
        while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
        return std::string(s.substr(b, e - b));
    }

    // Числовой литерал после (float)/(double)/(int): "-1.5", "100".
    std::size_t ScanDecimalLiteral(std::string_view s, std::size_t pos)
    {
        std::size_t end = pos;
        if (end < s.size() && (s[end] == '-' || s[end] == '+')) ++end;
        while (end < s.size() && (std::isdigit(static_cast<unsigned char>(s[end])) || s[end] == '.')) ++end;
        return end;
    }

    // Приводит числа одной строки из записи Cheat Engine к явной.
    bool ConvertCeNumbers(std::string_view line, std::string& out, std::string& error)
    {
        out.clear();
        std::size_t i = 0;

        while (i < line.size())
        {
            const char c = line[i];

            // (float)1.5 / (double)1.5 / (int)100 — значение пишется как
            // целое с теми же битами, как это делает CE.
            if (c == '(')
            {
                const std::size_t close = line.find(')', i);
                if (close != std::string_view::npos)
                {
                    std::string cast(line.substr(i + 1, close - i - 1));
                    for (char& ch : cast) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));

                    if (cast == "float" || cast == "double" || cast == "int")
                    {
                        std::size_t start = close + 1;
                        while (start < line.size() && line[start] == ' ') ++start;
                        const std::size_t end = ScanDecimalLiteral(line, start);
                        const std::string literal(line.substr(start, end - start));

                        if (literal.empty() || literal == "-" || literal == "+")
                        {
                            error = "после (" + cast + ") ожидалось число";
                            return false;
                        }

                        if (cast == "float")
                        {
                            const float f = std::strtof(literal.c_str(), nullptr);
                            std::uint32_t bits = 0;
                            std::memcpy(&bits, &f, sizeof(bits));
                            out += std::format("0x{:X}", bits);
                        }
                        else if (cast == "double")
                        {
                            const double d = std::strtod(literal.c_str(), nullptr);
                            std::uint64_t bits = 0;
                            std::memcpy(&bits, &d, sizeof(bits));
                            out += std::format("0x{:X}", bits);
                        }
                        else
                        {
                            out += std::to_string(std::strtoll(literal.c_str(), nullptr, 10));
                        }

                        i = end;
                        continue;
                    }
                }
            }

            // #123 — явно десятичное.
            if (c == '#')
            {
                std::size_t end = i + 1;
                if (end < line.size() && line[end] == '-') ++end;
                while (end < line.size() && std::isdigit(static_cast<unsigned char>(line[end]))) ++end;
                out.append(line.substr(i + 1, end - i - 1));
                i = end;
                continue;
            }

            if (IsWordChar(c))
            {
                std::size_t end = i;
                while (end < line.size() && IsWordChar(line[end])) ++end;
                const std::string_view word = line.substr(i, end - i);

                const bool startsWithDigit = std::isdigit(static_cast<unsigned char>(word[0])) != 0;
                const bool hasHexPrefix = word.size() > 2 && word[0] == '0' && (word[1] == 'x' || word[1] == 'X');

                if (startsWithDigit && !hasHexPrefix)
                {
                    // 800h — тоже шестнадцатеричное.
                    std::string_view digits = word;
                    if (digits.size() > 1 && (digits.back() == 'h' || digits.back() == 'H'))
                        digits.remove_suffix(1);

                    if (!IsHexDigits(digits))
                    {
                        error = "непонятное число: " + std::string(word);
                        return false;
                    }

                    out += "0x";
                    out.append(digits);
                }
                else
                {
                    out.append(word);
                }

                i = end;
                continue;
            }

            out.push_back(c);
            ++i;
        }

        return true;
    }
}

std::string Assembler::Preprocess(std::string_view text, AsmSyntax syntax, std::string* error)
{
    // 1. Комментарии: { ... } блоком и // до конца строки. ';' — разделитель.
    std::string plain;
    plain.reserve(text.size());

    for (std::size_t i = 0; i < text.size(); ++i)
    {
        const char c = text[i];

        if (c == '{')
        {
            const std::size_t close = text.find('}', i);
            if (close == std::string_view::npos)
            {
                if (error) *error = "незакрытый комментарий {";
                return {};
            }
            i = close;
            plain.push_back(' ');
            continue;
        }

        if (c == '/' && i + 1 < text.size() && text[i + 1] == '/')
        {
            while (i < text.size() && text[i] != '\n') ++i;
            plain.push_back('\n');
            continue;
        }

        plain.push_back(c == ';' || c == '\r' ? '\n' : c);
    }

    // 2. По инструкции на строку, без пустых строк и отступов.
    std::string result;
    std::size_t start = 0;

    while (start <= plain.size())
    {
        std::size_t end = plain.find('\n', start);
        if (end == std::string::npos) end = plain.size();

        std::string line = Trim(std::string_view(plain).substr(start, end - start));
        start = end + 1;

        if (line.empty()) continue;

        if (syntax == AsmSyntax::CheatEngine)
        {
            std::string converted;
            std::string lineError;
            if (!ConvertCeNumbers(line, converted, lineError))
            {
                if (error) *error = lineError + " — в строке: " + line;
                return {};
            }
            line = std::move(converted);
        }

        if (!result.empty()) result.push_back('\n');
        result += line;
    }

    if (error) error->clear();
    return result;
}

AssembleResult Assembler::Assemble(std::string_view text,
                                   bool is64Bit,
                                   std::uintptr_t baseAddress,
                                   AsmSyntax syntax)
{
    AssembleResult result;

    std::string preprocessError;
    const std::string source = Preprocess(text, syntax, &preprocessError);

    if (!preprocessError.empty())
    {
        result.error = preprocessError;
        return result;
    }

    if (source.empty())
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

    // Собираем построчно: тогда в ошибке можно назвать ИМЕННО ту строку,
    // которая не понравилась, а не весь патч целиком.
    std::size_t lineStart = 0;
    while (lineStart < source.size())
    {
        std::size_t lineEnd = source.find('\n', lineStart);
        if (lineEnd == std::string::npos) lineEnd = source.size();

        const std::string line = source.substr(lineStart, lineEnd - lineStart);
        lineStart = lineEnd + 1;

        if (const asmjit::Error parsed = parser.parse(line.c_str()); parsed != asmjit::Error::kOk)
        {
            // Самая частая причина — не указан размер операнда:
            // "mov [rbx+0x800], 1000" неоднозначен, ассемблер не знает,
            // писать 4 байта или 8. Нужно "mov qword ptr [rbx+0x800], 1000".
            std::string hint;
            if (line.find('[') != std::string::npos && line.find("ptr") == std::string::npos)
            {
                hint = " (укажите размер: byte/word/dword/qword ptr)";
            }

            result.error = std::format("{} — в строке: {}{}",
                                       asmjit::stringify_error(parsed), line, hint);
            return result;
        }
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
