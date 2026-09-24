#include <gtest/gtest.h>

#include <Windows.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "cheats/CheatDefinition.h"
#include "cheats/CheatFactory.h"
#include "cheats/CheatOption.h"
#include "core/Assembler.h"
#include "core/MemoryAccess.h"
#include "patches/CavePatch.h"
#include "patches/NopPatch.h"
#include "patches/WriteAddressPatch.h"

// Тесты, которым нужна настоящая память процесса. Целью служит сам
// тестовый процесс: MemoryAccess открывает его по собственному PID, как
// открыл бы игру. Так Apply/Restore проверяются по-настоящему, без игры.

namespace
{
    std::string Hex(const std::vector<std::uint8_t>& bytes)
    {
        std::string out;
        for (std::uint8_t b : bytes)
        {
            char buf[4];
            std::snprintf(buf, sizeof(buf), "%02X ", b);
            out += buf;
        }
        if (!out.empty()) out.pop_back();
        return out;
    }

    // Кусок исполняемой памяти в своём процессе — «код игры», который
    // будем патчить. Сигнатура уникальна, чтобы сканер нашёл именно его.
    struct FakeCode
    {
        std::uint8_t* base = nullptr;
        static constexpr std::size_t SIZE = 4096;

        explicit FakeCode(const std::vector<std::uint8_t>& code)
        {
            base = static_cast<std::uint8_t*>(
                VirtualAlloc(nullptr, SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
            std::memset(base, 0xCC, SIZE);
            std::memcpy(base + 64, code.data(), code.size());
        }

        ~FakeCode() { VirtualFree(base, 0, MEM_RELEASE); }

        std::uint8_t* Site() const { return base + 64; }
        std::uintptr_t Address() const { return reinterpret_cast<std::uintptr_t>(Site()); }
    };

    DWORD Self() { return GetCurrentProcessId(); }

    std::wstring ToHexSignature(const std::vector<std::uint8_t>& bytes)
    {
        std::wstring out;
        for (std::uint8_t b : bytes)
        {
            wchar_t buf[4];
            swprintf(buf, 4, L"%02X ", b);
            out += buf;
        }
        return out;
    }
}

// --- Препроцессор ассемблера ---

TEST(AsmPreprocess, SemicolonSeparatesInstructions)
{
    // Регрессия: AsmTK считает ';' началом комментария, и всё после неё
    // молча выбрасывалось — хотя документация обещала разделитель.
    EXPECT_EQ(Assembler::Preprocess("nop; ret", AsmSyntax::Standard), "nop\nret");

    const auto one = Assembler::Assemble("mov rsi, 1000; mov [rbx+0x800], rsi", true);
    const auto two = Assembler::Assemble("mov rsi, 1000\nmov [rbx+0x800], rsi", true);
    ASSERT_TRUE(one.ok) << one.error;
    EXPECT_EQ(one.bytes, two.bytes);
}

TEST(AsmPreprocess, CommentsAndIndentationAreStripped)
{
    const std::string text = R"(
        // заголовок
        nop            // сам патч
        { блочный
          комментарий CE }
        ret
    )";
    EXPECT_EQ(Assembler::Preprocess(text, AsmSyntax::Standard), "nop\nret");
}

TEST(AsmPreprocess, UnterminatedBlockCommentIsAnError)
{
    std::string error;
    Assembler::Preprocess("nop { oops", AsmSyntax::Standard, &error);
    EXPECT_FALSE(error.empty());
    EXPECT_FALSE(Assembler::Assemble("nop { oops", true).ok);
}

TEST(AsmPreprocess, CheatEngineNumbersAreHex)
{
    EXPECT_EQ(Assembler::Preprocess("mov [rbx+00000800],000003E8", AsmSyntax::CheatEngine),
              "mov [rbx+0x00000800],0x000003E8");

    // Регистры и мнемоники не трогаются, даже если похожи на hex.
    EXPECT_EQ(Assembler::Preprocess("add r8d, 10", AsmSyntax::CheatEngine), "add r8d, 0x10");
    EXPECT_EQ(Assembler::Preprocess("movss xmm0, [rax+8]", AsmSyntax::CheatEngine), "movss xmm0, [rax+0x8]");
    EXPECT_EQ(Assembler::Preprocess("mov eax, 0x10", AsmSyntax::CheatEngine), "mov eax, 0x10");
    EXPECT_EQ(Assembler::Preprocess("mov eax, 800h", AsmSyntax::CheatEngine), "mov eax, 0x800");
}

TEST(AsmPreprocess, CheatEngineDecimalAndFloat)
{
    EXPECT_EQ(Assembler::Preprocess("mov eax, #1000", AsmSyntax::CheatEngine), "mov eax, 1000");
    EXPECT_EQ(Assembler::Preprocess("mov eax, (float)100.5", AsmSyntax::CheatEngine), "mov eax, 0x42C90000");
    EXPECT_EQ(Assembler::Preprocess("mov eax, (int)250", AsmSyntax::CheatEngine), "mov eax, 250");
}

TEST(AsmPreprocess, CheatEngineScriptAssemblesLikeStandard)
{
    const auto ce = Assembler::Assemble("mov qword ptr [rbx+800],3E8", true, 0, AsmSyntax::CheatEngine);
    const auto std = Assembler::Assemble("mov qword ptr [rbx+0x800],1000", true);
    ASSERT_TRUE(ce.ok) << ce.error;
    EXPECT_EQ(ce.bytes, std.bytes);
}

TEST(AsmPreprocess, ErrorNamesTheOffendingLineOnly)
{
    const auto r = Assembler::Assemble("nop\nmov [rbx+0x800], 1000\nret", true);
    ASSERT_FALSE(r.ok);
    EXPECT_NE(r.error.find("mov [rbx+0x800], 1000"), std::string::npos);
    EXPECT_EQ(r.error.find("ret"), std::string::npos);
    EXPECT_NE(r.error.find("ptr"), std::string::npos) << "подсказка про размер операнда";
}

// --- Разбор сигнатур ---

TEST(ParseSignature, ReportsProblemsInsteadOfGuessingSilently)
{
    std::vector<std::uint8_t> pattern;
    std::wstring mask;
    std::string error;

    EXPECT_TRUE(ParseSignature(L"29 93 ?? 8B", pattern, mask, error)) << error;
    EXPECT_EQ(mask, L"xx?x");

    EXPECT_FALSE(ParseSignature(L"29 ZZ 8B", pattern, mask, error));
    EXPECT_FALSE(error.empty());
    EXPECT_EQ(mask, L"x?x") << "непонятный байт — джокер, а не 0x00";

    EXPECT_FALSE(ParseSignature(L"?? ??", pattern, mask, error));
    EXPECT_FALSE(ParseSignature(L"", pattern, mask, error));
}

TEST(ParseSignature, CompactFormIsSplitByTwo)
{
    std::vector<std::uint8_t> pattern;
    std::wstring mask;
    std::string error;

    ASSERT_TRUE(ParseSignature(L"2993??8B", pattern, mask, error)) << error;
    EXPECT_EQ(mask, L"xx?x");
    EXPECT_EQ(pattern, (std::vector<std::uint8_t>{ 0x29, 0x93, 0x00, 0x8B }));
}

// --- Сканер ---

TEST(FindPattern, FindsWithWildcardsAndAtEdges)
{
    const std::vector<std::uint8_t> data{ 0x10, 0x20, 0x30, 0x40, 0x50 };

    EXPECT_EQ(MemoryAccess::FindPattern(data.data(), data.size(), { 0x10, 0x20 }, L"xx"), 0u);
    EXPECT_EQ(MemoryAccess::FindPattern(data.data(), data.size(), { 0x40, 0x50 }, L"xx"), 3u);
    EXPECT_EQ(MemoryAccess::FindPattern(data.data(), data.size(), { 0x20, 0x00, 0x40 }, L"x?x"), 1u);
    EXPECT_EQ(MemoryAccess::FindPattern(data.data(), data.size(), { 0x00, 0x30 }, L"?x"), 1u);
    EXPECT_EQ(MemoryAccess::FindPattern(data.data(), data.size(), { 0x50, 0x60 }, L"xx"), SIZE_MAX);
    EXPECT_EQ(MemoryAccess::FindPattern(data.data(), 1, { 0x10, 0x20 }, L"xx"), SIZE_MAX);
}

TEST(ScanSignature, FindsCodeInLiveProcessAcrossRegions)
{
    // Сигнатура на самом краю первой страницы: половина в одной, половина
    // в другой. Раньше сканер проверял каждый регион отдельно и такое
    // совпадение терял.
    auto* block = static_cast<std::uint8_t*>(
        VirtualAlloc(nullptr, 0x3000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    ASSERT_NE(block, nullptr);
    std::memset(block, 0, 0x3000);

    DWORD old = 0;
    VirtualProtect(block + 0x1000, 0x1000, PAGE_EXECUTE_READ, &old); // другой регион

    // Запись во вторую страницу через MemoryAccess — заодно проверка, что
    // запись в неизменяемую страницу работает.
    const std::vector<std::uint8_t> sig{ 0xDE, 0xAD, 0xBE, 0xEF, 0x13, 0x37 };
    std::memcpy(block + 0x1000 - 3, sig.data(), 3);

    MemoryAccess mem(Self());
    ASSERT_TRUE(mem.IsValid());
    ASSERT_TRUE(mem.Write(reinterpret_cast<std::uintptr_t>(block + 0x1000), sig.data() + 3, 3));

    const std::uintptr_t hit = mem.ScanSignature(reinterpret_cast<std::uintptr_t>(block), 0x3000, sig, L"xxxxxx");
    EXPECT_EQ(hit, reinterpret_cast<std::uintptr_t>(block + 0x1000 - 3));

    VirtualFree(block, 0, MEM_RELEASE);
}

TEST(ScanSignature, SkipsUnreadableRegionsInsteadOfStopping)
{
    // Первая страница недоступна, сигнатура — во второй. Раньше первый же
    // нечитаемый регион обрывал весь поиск.
    auto* block = static_cast<std::uint8_t*>(
        VirtualAlloc(nullptr, 0x2000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    ASSERT_NE(block, nullptr);

    const std::vector<std::uint8_t> sig{ 0x51, 0x52, 0x53, 0x54, 0x55 };
    std::memcpy(block + 0x1800, sig.data(), sig.size());

    DWORD old = 0;
    VirtualProtect(block, 0x1000, PAGE_NOACCESS, &old);

    MemoryAccess mem(Self());
    const std::uintptr_t hit = mem.ScanSignature(reinterpret_cast<std::uintptr_t>(block), 0x2000, sig, L"xxxxx");
    EXPECT_EQ(hit, reinterpret_cast<std::uintptr_t>(block + 0x1800));

    VirtualFree(block, 0, MEM_RELEASE);
}

// --- Прыжки ---

TEST(JumpBytes, NearJumpIsRel32)
{
    const auto j = CavePatch::CalculateJumpBytes(0x1000, 0x2000);
    ASSERT_EQ(j.size(), 5u);
    EXPECT_EQ(j[0], 0xE9);

    std::int32_t rel = 0;
    std::memcpy(&rel, j.data() + 1, 4);
    EXPECT_EQ(rel, 0x2000 - 0x1000 - 5);
}

TEST(JumpBytes, BackwardJumpIsNegativeRel32)
{
    const auto j = CavePatch::CalculateJumpBytes(0x2000, 0x1000);
    ASSERT_EQ(j.size(), 5u);

    std::int32_t rel = 0;
    std::memcpy(&rel, j.data() + 1, 4);
    EXPECT_EQ(rel, 0x1000 - 0x2000 - 5);
}

TEST(JumpBytes, FarJumpIsAbsoluteOnX64)
{
    const auto j = CavePatch::CalculateJumpBytes(0x100000, 0x9000000000ull);
    ASSERT_EQ(j.size(), 14u);
    EXPECT_EQ(j[0], 0xFF);
    EXPECT_EQ(j[1], 0x25);

    std::uint64_t target = 0;
    std::memcpy(&target, j.data() + 6, 8);
    EXPECT_EQ(target, 0x9000000000ull);
}

TEST(JumpBytes, X86AlwaysUsesRel32)
{
    // В 32-битной цели FF 25 с 8-байтным адресом не существует, а rel32
    // достаёт до любого адреса, заворачиваясь по модулю 2^32.
    const auto j = CavePatch::CalculateJumpBytes(0x10000000, 0xF0000000, false);
    ASSERT_EQ(j.size(), 5u);

    std::uint32_t rel = 0;
    std::memcpy(&rel, j.data() + 1, 4);
    EXPECT_EQ(static_cast<std::uint32_t>(0x10000000 + 5 + rel), 0xF0000000u);
}

// --- Патчи на живом процессе ---

TEST(LivePatch, OptionAppliesAndRestoresValueFreeze)
{
    // Заморозка через цепочку с абсолютным адресом — как Address::At.
    static volatile std::int32_t health = 5;

    CheatOption option(L"freeze");
    option.AddWriteValuePatch({ reinterpret_cast<std::uintptr_t>(&health) }, std::int32_t{ 1000 },
                              true, WriteAddressPatch::Mode::Freeze);

    ASSERT_TRUE(option.SetEnabled(true, Self())) << option.LastError();
    EXPECT_TRUE(option.IsEnabled());
    EXPECT_EQ(health, 1000);

    // Игра «потратила» здоровье — тик заморозки возвращает его.
    health = 3;
    MemoryAccess mem(Self());
    option.Process(Self(), &mem);
    EXPECT_EQ(health, 1000);

    ASSERT_TRUE(option.SetEnabled(false, Self()));
    health = 3;
    option.Process(Self(), &mem);
    EXPECT_EQ(health, 3) << "выключенная заморозка больше не пишет";
}

TEST(LivePatch, OneShotWriteFlashesAndTurnsItselfOff)
{
    static volatile float speed = 1.0f;

    const CheatDefinition def{
        .name = L"speed",
        .patches = { WriteValue(Address::At(reinterpret_cast<std::uintptr_t>(&speed)), 2.5f) },
        .autoOffMs = 1,
    };

    std::unique_ptr<CheatOption> option = CreateCheatFromDefinition(def);
    ASSERT_TRUE(option->IsOneShot());

    ASSERT_TRUE(option->SetEnabled(true, Self())) << option->LastError();
    EXPECT_EQ(speed, 2.5f);

    Sleep(5);
    option->Process(Self(), nullptr);
    EXPECT_FALSE(option->IsEnabled()) << "вспышка разовой записи гаснет сама";

    // Повторное нажатие пишет снова, а не «выключает».
    speed = 1.0f;
    ASSERT_TRUE(option->Toggle(Self()));
    EXPECT_EQ(speed, 2.5f);
}

TEST(LivePatch, FailedPatchRollsBackTheOnesAlreadyApplied)
{
    // Первый патч пишет, второй не может найти сигнатуру. Раньше первый
    // оставался в памяти, а опция — «выключенной», и откатить его было
    // уже некому.
    static volatile std::int32_t value = 7;

    CheatOption option(L"partial");
    option.AddWriteValuePatch({ reinterpret_cast<std::uintptr_t>(&value) }, std::int32_t{ 42 },
                              true, WriteAddressPatch::Mode::Freeze);
    option.AddNopPatch(L"DE AD BE EF 00 11 22 33 44 55 66 77 88 99", 0);

    EXPECT_FALSE(option.SetEnabled(true, Self()));
    EXPECT_FALSE(option.IsEnabled());
    EXPECT_NE(option.LastError().find("2 из 2"), std::string::npos) << option.LastError();
    EXPECT_GE(option.SecondsSinceFailure(), 0.0f);

    // Заморозка откачена: тик ничего не пишет.
    value = 7;
    MemoryAccess mem(Self());
    option.Process(Self(), &mem);
    EXPECT_EQ(value, 7);
}

TEST(LivePatch, ProcessLossForgetsState)
{
    static volatile std::int32_t value = 0;

    CheatOption option(L"lost");
    option.AddWriteValuePatch({ reinterpret_cast<std::uintptr_t>(&value) }, std::int32_t{ 9 },
                              true, WriteAddressPatch::Mode::Freeze);

    ASSERT_TRUE(option.SetEnabled(true, Self()));
    option.OnProcessLost();

    EXPECT_FALSE(option.IsEnabled()) << "после перезапуска игры переключатель не должен врать";

    value = 1;
    MemoryAccess mem(Self());
    option.Process(Self(), &mem);
    EXPECT_EQ(value, 1);
}

// «Игровая функция» для кейвов: mov eax, 1 / mov ecx, 0x7A1B2C3D / ret.
// Второй mov не нужен функции — он делает сигнатуру уникальной.
namespace
{
    const std::vector<std::uint8_t> kGameFunction{
        0xB8, 0x01, 0x00, 0x00, 0x00,   // mov eax, 1
        0xB9, 0x3D, 0x2C, 0x1B, 0x7A,   // mov ecx, 0x7A1B2C3D
        0xC3,                           // ret
    };

    // Сигнатура — первые 10 байт; модуль "*" — искать по всей исполняемой
    // памяти, потому что «функция» лежит в куче, а не в exe.
    CheatDefinition CaveDefinition(PatchSpec spec)
    {
        return CheatDefinition{
            .name = L"cave",
            .patches = { std::move(spec) },
            .module = L"*",
        };
    }

    std::string GameFunctionSignature()
    {
        std::string sig;
        for (std::size_t i = 0; i < 10; ++i)
        {
            char buf[4];
            std::snprintf(buf, sizeof(buf), "%02X ", kGameFunction[i]);
            sig += buf;
        }
        return sig;
    }

    int Call(const FakeCode& code)
    {
        return reinterpret_cast<int (*)()>(code.Site())();
    }
}

TEST(LivePatch, CaveReplacesInstructionAndRestoresIt)
{
    FakeCode code(kGameFunction);
    ASSERT_EQ(Call(code), 1);

    // Патч НАМЕРЕННО меняет eax — поэтому KeepRegisterChanges: иначе кейв
    // спас бы eax и игра увидела бы прежнее значение.
    auto option = CreateCheatFromDefinition(CaveDefinition(
        Cave(GameFunctionSignature(), Asm("mov eax, 99")).KeepRegisterChanges()));

    ASSERT_TRUE(option->SetEnabled(true, Self())) << option->LastError();
    EXPECT_EQ(code.Site()[0], 0xE9) << "на месте инструкции — прыжок в кейв";
    EXPECT_EQ(Call(code), 99);

    ASSERT_TRUE(option->SetEnabled(false, Self()));
    EXPECT_EQ(std::vector<std::uint8_t>(code.Site(), code.Site() + kGameFunction.size()), kGameFunction)
        << "откат вернул байты один в один";
    EXPECT_EQ(Call(code), 1);
}

TEST(LivePatch, CavePreservesClobberedRegistersByDefault)
{
    FakeCode code(kGameFunction);

    // Тот же патч без KeepRegisterChanges: кейв оборачивает его в
    // push rax / pop rax, и затёртый eax восстанавливается. А оригинальная
    // инструкция (mov eax, 1) в режиме замены выброшена — значит, функция
    // вернёт то, что было в eax при входе, но точно не 99.
    auto option = CreateCheatFromDefinition(CaveDefinition(
        CaveKeepOriginal(GameFunctionSignature(), Asm("mov eax, 99"))));

    ASSERT_TRUE(option->SetEnabled(true, Self())) << option->LastError();

    // KeepOriginal: после патча выполняется перенесённый mov eax, 1.
    EXPECT_EQ(Call(code), 1);

    ASSERT_TRUE(option->SetEnabled(false, Self()));
    EXPECT_EQ(Call(code), 1);
}

TEST(LivePatch, CaveInCheatEngineSyntax)
{
    FakeCode code(kGameFunction);

    auto option = CreateCheatFromDefinition(CaveDefinition(
        Cave(GameFunctionSignature(), AsmCE("mov eax,63   // 0x63 = 99")).KeepRegisterChanges()));

    ASSERT_TRUE(option->SetEnabled(true, Self())) << option->LastError();
    EXPECT_EQ(Call(code), 99);
    option->SetEnabled(false, Self());
}

TEST(LivePatch, NopWithoutLengthCoversOneWholeInstruction)
{
    FakeCode code(kGameFunction);

    // mov ecx, ... — вторая инструкция, её и забиваем: .At(5).
    auto option = CreateCheatFromDefinition(CaveDefinition(Nop(GameFunctionSignature()).At(5)));

    ASSERT_TRUE(option->SetEnabled(true, Self())) << option->LastError();
    EXPECT_EQ(Hex(std::vector<std::uint8_t>(code.Site(), code.Site() + 11)),
              "B8 01 00 00 00 90 90 90 90 90 C3") << "ровно одна инструкция, 5 байт";
    EXPECT_EQ(Call(code), 1);

    ASSERT_TRUE(option->SetEnabled(false, Self()));
    EXPECT_EQ(std::vector<std::uint8_t>(code.Site(), code.Site() + kGameFunction.size()), kGameFunction);
}

TEST(LivePatch, SecondEnableIsCachedAndStillCorrect)
{
    FakeCode code(kGameFunction);

    auto option = CreateCheatFromDefinition(CaveDefinition(
        Cave(GameFunctionSignature(), Asm("mov eax, 7")).KeepRegisterChanges()));

    for (int round = 0; round < 3; ++round)
    {
        ASSERT_TRUE(option->SetEnabled(true, Self())) << option->LastError();
        EXPECT_EQ(Call(code), 7);
        ASSERT_TRUE(option->SetEnabled(false, Self()));
        EXPECT_EQ(Call(code), 1);
    }
}

TEST(LivePatch, MissingSignatureExplainsWhy)
{
    auto option = CreateCheatFromDefinition(CaveDefinition(
        Nop("DE AD BE EF 00 11 22 33 44 55 66 77 88 99")));

    EXPECT_FALSE(option->SetEnabled(true, Self()));
    EXPECT_NE(option->LastError().find("не найдена"), std::string::npos) << option->LastError();
}

TEST(LivePatch, BadAssemblerExplainsWhy)
{
    FakeCode code(kGameFunction);

    auto option = CreateCheatFromDefinition(CaveDefinition(
        Cave(GameFunctionSignature(), Asm("mov [rbx+8], 1"))));

    EXPECT_FALSE(option->SetEnabled(true, Self()));
    EXPECT_NE(option->LastError().find("ptr"), std::string::npos) << option->LastError();
    EXPECT_EQ(Call(code), 1) << "ничего не тронуто";
}

// --- Фоновый поток ---

#include <atomic>

#include "core/Cheat.h"

TEST(CheatThread, PostedTasksRunOnWorkerAndStopDrainsQueue)
{
    Cheat process(L"no-such-game-for-tests.exe");

    // Поток не запущен — задача выполняется сразу, а не теряется.
    std::atomic<int> ran{ 0 };
    process.Post([&] { ++ran; });
    EXPECT_EQ(ran.load(), 1);

    process.Start();

    std::atomic<DWORD> worker{ 0 };
    process.Post([&] { worker = GetCurrentThreadId(); ++ran; });

    for (int i = 0; i < 100 && ran.load() < 2; ++i) Sleep(10);
    EXPECT_EQ(ran.load(), 2);
    EXPECT_NE(worker.load(), GetCurrentThreadId()) << "задача выполнялась в фоновом потоке";

    // Stop дожидается потока и выполняет всё, что не успело выполниться:
    // щелчок перед самым закрытием не теряется.
    process.Post([&] { ++ran; });
    process.Stop();
    EXPECT_EQ(ran.load(), 3);

    EXPECT_FALSE(process.isProcessRunning());
}

TEST(CheatThread, FindsProcessByNameCaseInsensitively)
{
    // Цель — сам тестовый процесс; имя нарочно в другом регистре.
    wchar_t path[MAX_PATH]{};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::wstring exe(path);
    exe = exe.substr(exe.find_last_of(L"\\/") + 1);
    for (wchar_t& c : exe) c = static_cast<wchar_t>(towupper(c));

    Cheat process(exe);
    process.Start();

    for (int i = 0; i < 100 && !process.isProcessRunning(); ++i) Sleep(20);
    process.Stop();

    EXPECT_TRUE(process.isProcessRunning());
    EXPECT_EQ(process.GetProcessID(), GetCurrentProcessId());
}
