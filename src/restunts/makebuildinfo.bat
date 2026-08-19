@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "git_dir=S:\.git"
set "git_hash="
set "git_head="

if exist "%git_dir%\HEAD" set /p "git_head="<"%git_dir%\HEAD"

if /I "!git_head:~0,5!"=="ref: " (
    set "git_ref=!git_head:~5!"
    if exist "%git_dir%\!git_ref!" (
        set /p "git_hash="<"%git_dir%\!git_ref!"
    ) else if exist "%git_dir%\packed-refs" (
        for /F "usebackq tokens=1,2" %%A in ("%git_dir%\packed-refs") do (
            if "%%B"=="!git_ref!" set "git_hash=%%A"
        )
    )
) else (
    set "git_hash=!git_head!"
)

if not defined git_hash set "git_hash=??????"

>c\buildinfo.h echo #define BUILD_GIT_HASH "!git_hash:~0,6!"

endlocal
