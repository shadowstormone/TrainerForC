#include "CavePatch.h"
#include "CheatOption.h"
#include "Memory_Functions.h"
#include <stdexcept>
#include <cstring>

#define NMD_ASSEMBLY_IMPLEMENTATION
#include "nmd_assembly.h"

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

bool CavePatch::Hack(HANDLE hProcess)
{
    originalSize = 0;

    const bool is64BitProcess = isTargetX64Process(hProcess);
    uintptr_t baseAddress;
    const uintptr_t scanSize = is64BitProcess ? 0x7FFFFFFFFFFFFFFF : 0x7FFFFFFF;

    if (parent->GetModuleName() && wcslen(parent->GetModuleName()) > 0)
    {
        baseAddress = GetModuleBaseAddress(hProcess, parent->GetModuleName());
    }
    else
    {
        baseAddress = GetProcessBaseAddress(hProcess);
    }

    if (!baseAddress)
    {
        //std::cerr << "Failed to get base address.\n";
        throw std::runtime_error("Failed to get base address.");
        return false;
    }

    patternAddress = ScanSignature(hProcess, baseAddress, scanSize, pattern, mask);
    patchAddress = static_cast<LPBYTE>(patternAddress) + patchOffset;
    originalAddress = reinterpret_cast<LPVOID>(patchAddress);
    originalBytes = static_cast<PBYTE>(ReadMem(hProcess, originalAddress, MAX_INSTRUCTION_LENGTH));

    allocatedAddress = VirtualAllocEx(hProcess, nullptr, CAVE_SIZE, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (!allocatedAddress)
    {
        return false;
    }

    BYTE jmpSize = 0;
    const PBYTE jmpBytes = CalculateJumpBytes(originalAddress, allocatedAddress, jmpSize);

    size_t offset = 0;
    do{
        const size_t length = nmd_x86_ldisasm(originalBytes + offset, MAX_INSTRUCTION_LENGTH - offset, is64BitProcess ? NMD_X86_MODE_64 : NMD_X86_MODE_32);
        //originalSize += length;
        originalSize += static_cast<BYTE>(length); // явное преобразование size_t в BYTE
        offset += length;
    } while (originalSize < jmpSize);

    if (originalSize > patchSize)
    {
        //std::cerr << "Original instructions too large to fit in the cave.\n";
        throw std::runtime_error("Original instructions too large to fit in the cave.");
        delete[] jmpBytes;
        return false;
    }

    if (jmpSize == originalSize)
    {
        WriteMem(hProcess, originalAddress, jmpBytes, jmpSize);
    }
    else
    {
        BYTE backJmpSize = 0;
        const PBYTE back_jmp_bytes = CalculateJumpBytes(
            static_cast<PBYTE>(allocatedAddress) + caveSize + patchSize,
            static_cast<PBYTE>(originalAddress) + originalSize, backJmpSize);

        if (!back_jmp_bytes)
        {
            //std::cerr << "Failed to allocate memory for the backward jump.\n";
            throw std::runtime_error("Failed to allocate memory for the backward jump.");
            delete[] jmpBytes;
            return false;
        }

        const size_t cave_size = patchSize + backJmpSize;
        PBYTE cave_bytes = new BYTE[cave_size];

        std::memcpy(cave_bytes, patchBytes, patchSize);
        std::memcpy(cave_bytes + patchSize, back_jmp_bytes, backJmpSize);
        WriteMem(hProcess, allocatedAddress, cave_bytes, cave_size);

        delete[] cave_bytes;
        delete[] back_jmp_bytes;

        const size_t nops = originalSize - jmpSize;
        if (nops > 0)
        {
            PBYTE bytes = new BYTE[originalSize];

            std::memcpy(bytes, jmpBytes, jmpSize);
            std::memset(bytes + jmpSize, 0x90, nops);
            std::memcpy(bytes + jmpSize + nops, originalBytes + jmpSize, originalSize - jmpSize - nops);

            WriteMem(hProcess, originalAddress, bytes, originalSize);

            delete[] bytes;
        }
        else
        {
            WriteMem(hProcess, originalAddress, jmpBytes, jmpSize);
        }
    }

    delete[] jmpBytes;
    return true;
}


bool CavePatch::Restore(HANDLE hProcess)
{
    WriteMem(hProcess, originalAddress, originalBytes, originalSize);
    FreeMem(hProcess, allocatedAddress, CAVE_SIZE);
    originalSize = 0;
    return true;
}