@echo off
call setpath.bat
if errorlevel 1 exit /b 1

make pixldump-original
if errorlevel 1 exit /b 1

make pixldump
if errorlevel 1 exit /b 1
