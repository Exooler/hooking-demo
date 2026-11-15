#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cstdint>

using CalculateFn = int (*)(int);
static constexpr SIZE_T patchSize = 14;
static BYTE saved[patchSize];
static BYTE* target = nullptr;
static BYTE* trampoline = nullptr;
static CalculateFn original = nullptr;

// FF 25 00 00 00 00 = jmp qword ptr [rip+0], followed by destination.
static void MakeJump(BYTE* buffer, const void* destination)
{
    const BYTE opcode[6] = {0xFF, 0x25, 0, 0, 0, 0};
    std::memcpy(buffer, opcode, sizeof(opcode));
    const auto address = reinterpret_cast<std::uintptr_t>(destination);
    std::memcpy(buffer + 6, &address, sizeof(address));
}

static int HookedCalculate(int value)
{
    std::printf("  [DLL detour] Received %d; calling trampoline...\n", value);
    const int result = original(value);
    std::printf("  [DLL detour] Original returned %d; adding 10.\n", result);
    return result + 10;
}

// Call only while no other thread can execute Calculate or its detour.
extern "C" __declspec(dllexport) BOOL InstallHook(void* address)
{
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    if (target || !address) return FALSE;
    auto entry = static_cast<BYTE*>(address);
    // Deliberately accept only our assembly function's known NOP entry.
    for (SIZE_T i = 0; i < patchSize; ++i)
        if (entry[i] != 0x90) return FALSE;

    auto code = static_cast<BYTE*>(VirtualAlloc(
        nullptr, patchSize * 2, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    if (!code) return FALSE;
    std::memcpy(saved, entry, patchSize);
    std::memcpy(code, saved, patchSize);
    MakeJump(code + patchSize, entry + patchSize);
    DWORD previous;
    if (!VirtualProtect(code, patchSize * 2, PAGE_EXECUTE_READ, &previous))
    { VirtualFree(code, 0, MEM_RELEASE); return FALSE; }
    FlushInstructionCache(GetCurrentProcess(), code, patchSize * 2);

    BYTE jump[patchSize];
    MakeJump(jump, reinterpret_cast<const void*>(&HookedCalculate));
    if (!VirtualProtect(entry, patchSize, PAGE_EXECUTE_READWRITE, &previous))
    { VirtualFree(code, 0, MEM_RELEASE); return FALSE; }
    trampoline = code;
    original = reinterpret_cast<CalculateFn>(code);
    target = entry;
    std::memcpy(entry, jump, patchSize);
    DWORD ignored;
    if (!VirtualProtect(entry, patchSize, previous, &ignored))
    {
        std::fputs("Could not restore target page protection.\n", stderr);
        ExitProcess(2);
    }
    FlushInstructionCache(GetCurrentProcess(), entry, patchSize);
    std::printf("  [DLL] Target=%p  Detour=%p  Trampoline=%p\n",
        static_cast<void*>(entry), reinterpret_cast<void*>(&HookedCalculate),
        static_cast<void*>(code));
    return TRUE;
}

extern "C" __declspec(dllexport) BOOL RemoveHook()
{
    if (!target) return FALSE;
    DWORD previous;
    if (!VirtualProtect(target, patchSize, PAGE_EXECUTE_READWRITE, &previous))
        return FALSE;
    std::memcpy(target, saved, patchSize);
    DWORD ignored;
    if (!VirtualProtect(target, patchSize, previous, &ignored))
    { std::fputs("Could not restore page protection.\n", stderr); ExitProcess(2); }
    FlushInstructionCache(GetCurrentProcess(), target, patchSize);
    if (!VirtualFree(trampoline, 0, MEM_RELEASE)) return FALSE;
    target = nullptr;
    trampoline = nullptr;
    original = nullptr;
    return TRUE;
}

// No patching in DllMain: the EXE calls the exports explicitly.
