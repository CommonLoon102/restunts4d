@echo off
set "WATCOM=%~dp0watcom"
if not exist "%WATCOM%\binnt\wcc.exe" (
    echo Open Watcom 2 is missing. Run tools\scripts\install-open-watcom.ps1 or .sh first.
    exit /b 1
)
if not exist "%WATCOM%\binnt\wlink.exe" (
    echo Open Watcom 2 linker is missing. Reinstall the pinned toolchain.
    exit /b 1
)
if not exist "%WATCOM%\binnt\wasm.exe" (
    echo Open Watcom 2 assembler is missing. Reinstall the pinned toolchain.
    exit /b 1
)
set "PATH=%WATCOM%\binnt;%~dp0bin;%PATH%"
set "INCLUDE=%WATCOM%\h"
exit /b 0
