#include <windows.h>
#include <cstdio>
#include <cstring>
#include <string>

extern "C" int Calculate(int value);
using InstallFn = BOOL (*)(void*);
using RemoveFn = BOOL (*)();

static void Pause(bool automatic)
{
    if (!automatic) { std::puts("\nPress Enter to continue..."); std::getchar(); }
}

static void DumpEntry()
{
    auto bytes = reinterpret_cast<const unsigned char*>(&Calculate);
    std::printf("  Entry bytes: ");
    for (int i = 0; i < 14; ++i) std::printf("%02X ", bytes[i]);
    std::puts("");
}

int main(int argc, char** argv)
{
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    const bool automatic = argc > 1 && std::strcmp(argv[1], "--auto") == 0;
    std::puts("FUNCTION HOOKING DEMO | Windows x64 | EXE + DLL\n");
    std::printf("PID: %lu | Calculate: %p\n", GetCurrentProcessId(),
                reinterpret_cast<void*>(&Calculate));
    std::puts("\n1. BEFORE HOOK: Calculate(x) = x + 1");
    DumpEntry();
    const int before = Calculate(5);
    std::printf("  Calculate(5) = %d\n", before);
    Pause(automatic);

    wchar_t path[32768];
    const DWORD length = GetModuleFileNameW(nullptr, path, 32768);
    if (!length || length >= 32768) return 1;
    const std::wstring exe(path, length);
    const std::wstring dll = exe.substr(0, exe.find_last_of(L"\\/") + 1) + L"hook.dll";
    HMODULE module = LoadLibraryW(dll.c_str());
    if (!module) { std::printf("LoadLibrary failed: %lu\n", GetLastError()); return 1; }
    auto install = reinterpret_cast<InstallFn>(GetProcAddress(module, "InstallHook"));
    auto remove = reinterpret_cast<RemoveFn>(GetProcAddress(module, "RemoveHook"));
    if (!install || !remove || !install(reinterpret_cast<void*>(&Calculate)))
    { std::puts("Hook installation failed."); FreeLibrary(module); return 1; }

    std::puts("\n2. HOOK ACTIVE: entry -> detour -> trampoline -> original body");
    DumpEntry();
    const int during = Calculate(5);
    std::printf("  Calculate(5) = %d\n", during);
    Pause(automatic);

    if (!remove())
    { std::puts("Removal failed; stopping without unloading the DLL."); return 1; }
    FreeLibrary(module);
    std::puts("\n3. HOOK REMOVED: original entry bytes restored");
    DumpEntry();
    const int after = Calculate(5);
    std::printf("  Calculate(5) = %d\n", after);
    const bool passed = before == 6 && during == 16 && after == 6;
    std::puts(passed ? "\nPASS: 6 -> 16 -> 6" : "\nFAIL: unexpected results");
    Pause(automatic);
    return passed ? 0 : 1;
}
