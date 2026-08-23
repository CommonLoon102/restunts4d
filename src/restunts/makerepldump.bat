call setpath.bat

set "target=S:\src\restunts\c\makefile"
set "temp=%target%.tmp-%RANDOM%"

make clean

make restunts-original
make repldump-original

@echo off
rem This is a hack to work around that the build process stops before linking the repldump target, so we need to run it twice to get the correct output.
rem If I modify the makefile then the build process completes correctly, so I delete it and then recreate it.
@echo on

copy /b "%target%" "%temp%" >nul ^
  && del "%target%" ^
  && move "%temp%" "%target%" >nul

make repldump-original

make clean

make restunts
make repldump

rem This below is here intentionally. It is very hard to get all 4 exes to compile.
copy /b "%target%" "%temp%" >nul ^
  && del "%target%" ^
  && move "%temp%" "%target%" >nul

make repldump
make repldump-original
