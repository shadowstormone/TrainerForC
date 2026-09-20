#include "patches/CavePatch.h"
#include "cheats/CheatOption.h"
#include <stdexcept>
#include <cstring>

#define NMD_ASSEMBLY_IMPLEMENTATION
#include "third_party/nmd_assembly.h"

namespace
{
    constexpr size_t MAX_INSTRUCTION_LENGTH = 15;
    constexpr size_t CAVE_SIZE = 4096;
}

PBYTE CavePatch::CalculateJumpBytes(LPVOID from, LPVOID to, BYTE& outSize)
{
    const uintptr_t delta = reinterpret_cast<uintptr_t>(to) - reinterpret_cast<uintptr_t>(from);
    const uintptr_t normalized_delta = std::abs(static_cast<intptr_t>(delta));

    PBYTE bytes;

    if (normalized_delta < (1ULL << 31))
    {
        bytes = new BYTE[5];
        bytes[0] = 0xE9;
        const uint32_t relative_addr = static_cast<uint32_t>(delta - 5);
        std::memcpy(bytes + 1, &relative_addr, sizeof(relative_addr));
        outSize = 5;
    }
    else if (normalized_delta < (1ULL << 63))
    {
        bytes = new BYTE[14];
        bytes[0] = 0xFF;
        bytes[1] = 0x25;
        std::memset(bytes + 2, 0, 4);
        const uintptr_t to_address = reinterpret_cast<uintptr_t>(to);
        std::memcpy(bytes + 6, &to_address, sizeof(to_address));
        outSize = 14;
    }
    else
    {
        throw std::runtime_error("Jump distance too large");
    }

    return bytes;
}

bool CavePatch::Apply(MemoryAccess& mem)
{
    originalSize = 0;

    const bool is64BitProcess = mem.IsTargetX64();
    uintptr_t baseAddress;
    const uintptr_t scanSize = is64BitProcess ? 0x7FFFFFFFFFFFFFFF : 0x7FFFFFFF;

    if (parent->GetModuleName() && wcslen(parent->GetModuleName()) > 0)
    {
        baseAddress = mem.ModuleBase(parent->GetModuleName());
    }
    else
    {
        baseAddress = mem.ProcessBase();
    }

    if (!baseAddress)
    {
        throw std::runtime_error("Failed to get base address.");
        return false;
    }

    patternAddress = mem.ScanSignature(baseAddress, scanSize, pattern.data(), mask);
    patchAddress = static_cast<LPBYTE>(patternAddress) + patchOffset;
    originalAddress = reinterpret_cast<LPVOID>(patchAddress);
    originalBytes = static_cast<PBYTE>(mem.Read(originalAddress, MAX_INSTRUCTION_LENGTH));

    allocatedAddress = VirtualAllocEx(mem.Handle(), nullptr, CAVE_SIZE, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (!allocatedAddress)
    {
        return false;
    }

    BYTE jmpSize = 0;
    const PBYTE jmpBytes = CalculateJumpBytes(originalAddress, allocatedAddress, jmpSize);

    // Считаем, сколько целых инструкций нужно "украсть" под прыжок.
    // Если длино-дизассемблер не смог разобрать инструкцию, он возвращает 0:
    // раньше originalSize не рос, offset не двигался и цикл крутился вечно,
    // подвешивая интерфейс. Теперь такой случай — честная ошибка.
    size_t offset = 0;
    bool decoded = true;

    while (originalSize < jmpSize)
    {
        if (offset >= MAX_INSTRUCTION_LENGTH)
        {
            decoded = false;
            break;
        }

        const size_t length = nmd_x86_ldisasm(originalBytes + offset,
                                              MAX_INSTRUCTION_LENGTH - offset,
                                              is64BitProcess ? NMD_X86_MODE_64 : NMD_X86_MODE_32);
        if (length == 0)
        {
            decoded = false;
            break;
        }

        originalSize += static_cast<BYTE>(length);
        offset += length;
    }

    // Общий откат: освобождаем кейв и буфер прыжка, чтобы не утекали.
    const auto rollback = [&]()
    {
        delete[] jmpBytes;
        if (allocatedAddress)
        {
            mem.Free(allocatedAddress, CAVE_SIZE);
            allocatedAddress = nullptr;
        }
        originalSize = 0;
    };

    if (!decoded)
    {
        rollback();
        throw std::runtime_error("Failed to decode original instructions at patch address.");
    }

    if (originalSize > patchSize)
    {
        rollback();
        throw std::runtime_error("Original instructions too large to fit in the cave.");
    }

    if (jmpSize == originalSize)
    {
        mem.Write(originalAddress, jmpBytes, jmpSize);
    }
    else
    {
        BYTE backJmpSize = 0;
        const PBYTE back_jmp_bytes = CalculateJumpBytes(
            static_cast<PBYTE>(allocatedAddress) + caveSize + patchSize,
            static_cast<PBYTE>(originalAddress) + originalSize, backJmpSize);

        if (!back_jmp_bytes)
        {
            throw std::runtime_error("Failed to allocate memory for the backward jump.");
            delete[] jmpBytes;
            return false;
        }

        const size_t cave_size = patchSize + backJmpSize;
        PBYTE cave_bytes = new BYTE[cave_size];

        std::memcpy(cave_bytes, patchBytes, patchSize);
        std::memcpy(cave_bytes + patchSize, back_jmp_bytes, backJmpSize);
        mem.Write(allocatedAddress, cave_bytes, cave_size);

        delete[] cave_bytes;
        delete[] back_jmp_bytes;

        const size_t nops = originalSize - jmpSize;
        if (nops > 0)
        {
            PBYTE bytes = new BYTE[originalSize];

            std::memcpy(bytes, jmpBytes, jmpSize);
            std::memset(bytes + jmpSize, 0x90, nops);
            std::memcpy(bytes + jmpSize + nops, originalBytes + jmpSize, originalSize - jmpSize - nops);

            mem.Write(originalAddress, bytes, originalSize);
            delete[] bytes;
        }
        else
        {
            mem.Write(originalAddress, jmpBytes, jmpSize);
        }
    }

    delete[] jmpBytes;
    return true;
}


bool CavePatch::Restore(MemoryAccess& mem)
{
    // Проверяем и восстанавливаем оригинальные байты
    if (mem.IsValid() && originalAddress && originalAddress != INVALID_HANDLE_VALUE && originalSize > 0)
    {
        // Проверяем, что процесс еще активен
        DWORD exitCode;
        if (GetExitCodeProcess(mem.Handle(), &exitCode) && exitCode == STILL_ACTIVE)
        {
            // Проверяем доступность памяти для восстановления
            MEMORY_BASIC_INFORMATION mbi;
            if (VirtualQueryEx(mem.Handle(), (LPCVOID)originalAddress, &mbi, sizeof(mbi)) != 0)
            {
                if (mbi.State == MEM_COMMIT)
                {
                    mem.Write(originalAddress, originalBytes, originalSize);
                }
            }
        }
    }

    // Проверяем и освобождаем выделенную память
    if (mem.IsValid() && allocatedAddress && allocatedAddress != INVALID_HANDLE_VALUE)
    {
        // Проверяем, что процесс еще активен
        DWORD exitCode;
        if (GetExitCodeProcess(mem.Handle(), &exitCode) && exitCode == STILL_ACTIVE)
        {
            // Проверяем доступность выделенной памяти
            MEMORY_BASIC_INFORMATION mbi;
            if (VirtualQueryEx(mem.Handle(), (LPCVOID)allocatedAddress, &mbi, sizeof(mbi)) != 0)
            {
                if (mbi.State == MEM_COMMIT)
                {
                    mem.Free(allocatedAddress, CAVE_SIZE);
                }
            }
        }
    }

    originalSize = 0;
    return true;
}