@echo off
setlocal
cd /d "%~dp0"
if not defined VSCMD_VER (
  echo Run this script from an x64 Native Tools Command Prompt for Visual Studio.
  exit /b 1
)
if not "%VSCMD_ARG_TGT_ARCH%"=="x64" (
  echo This demo requires the x64 developer environment.
  exit /b 1
)
if not exist bin mkdir bin
ml64 /nologo /c /Zi /Fo bin\target_asm.obj target.asm
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /Zi /Od /LD hook.cpp /Fo:bin\hook.obj /Fd:bin\hook_compile.pdb /link /OUT:bin\hook.dll /IMPLIB:bin\hook.lib /PDB:bin\hook.pdb /INCREMENTAL:NO
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /Zi /Od target.cpp bin\target_asm.obj /Fo:bin\target.obj /Fd:bin\target_compile.pdb /link /OUT:bin\target.exe /PDB:bin\target.pdb /INCREMENTAL:NO
if errorlevel 1 exit /b 1
echo Built bin\target.exe and bin\hook.dll
