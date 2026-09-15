@echo off
call setpath.bat
if errorlevel 1 exit /b 1
make restunts restunts-original repldump repldump-original
if errorlevel 1 exit /b 1
