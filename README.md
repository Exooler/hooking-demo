# Function hooking: EXE + DLL

Windows x64 educational inline hook, written in C++ and MASM. No hooking library.

## Run

Keep `bin/target.exe` and `bin/hook.dll` together. Run `target.exe` and press Enter between stages. For an uninterrupted verification, run `target.exe --auto`.

Expected results: **6 before, 16 during, 6 after**. Each stage prints the function's actual entry bytes. Installation prints target, detour, and trampoline addresses. The executable explicitly loads its sibling DLL with LoadLibraryW and calls InstallHook; there is no injection or separate controller program.

## Build

Install Visual Studio C++ desktop build tools, open its **x64 Native Tools Command Prompt**, and run `build.cmd` in this folder. PDB files are included by the build for debugger exploration.

## Code

`target.asm` defines the original function:

```asm
; Windows x64: input in ECX, output in EAX
; 14 NOPs precede this body as an explicit patch area
lea eax, [rcx+1]
ret
```

The DLL replaces those 14 NOPs with:

```asm
jmp qword ptr [rip+0]  ; FF 25 00 00 00 00
dq address_of_detour  ; eight-byte address, not an instruction
```

The trampoline contains the original 14 NOPs followed by the same absolute jump form pointing at `Calculate + 14`, where the original body begins. The C++ detour calls this trampoline and adds 10 to its return value. The compiler handles the Windows x64 calling convention for that call.

1. Show the original result and NOP bytes.
2. Step through InstallHook: copy bytes, create trampoline, patch entry.
3. Look at the `FF 25` jump and changed result.
4. Step into HookedCalculate, then the trampoline, then the original body.
5. Remove the hook and show the restored NOPs and original result.

This executable uses native x64 assembly; the earlier infographic used a simplified 32-bit x86 example. The argument locations therefore differ.

## Deliberate limits

This is a controlled, single-threaded demonstration. Install/remove only between calls, with no other callers active. The patcher accepts only this known 14-byte NOP entry. It does not decode arbitrary instructions, relocate relative addressing, or coordinate concurrent execution. A general hook engine needs those capabilities. Do not adapt the fixed-size copy to arbitrary functions.

Page protection is temporarily changed for patching, restored afterward, and the instruction cache is flushed. Trampoline memory is written first, then made executable/read-only. Removal restores the bytes before freeing the trampoline and unloading the DLL. Hook work is performed in explicit exports rather than DllMain.
