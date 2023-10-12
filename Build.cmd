@echo off
setlocal

set TOOLS=..\Tools
set SDCC_HOME=%TOOLS%\sdcc_410
set PATH=%SDCC_HOME%\bin;%TOOLS%\zxcc;%TOOLS%\hex2bin;%PATH%

set CPMDIR80=%TOOLS%\cpm

:: set SDCC_OPTS=-c -mz80 --opt-code-size
set SDCC_OPTS=-c -mz80 --opt-code-size --max-allocs-per-node200000

sdasz80 -fflopz ucrt0.s

sdcc %SDCC_OPTS% chario.c
sdcc %SDCC_OPTS% bios.c
sdcc %SDCC_OPTS% bdos.c
if not exist ff.rel sdcc %SDCC_OPTS% ff.c
sdcc %SDCC_OPTS% diskio.c
sdcc %SDCC_OPTS% fat.c
sdcc %SDCC_OPTS% fatiotst.c

sdldz80 -mjxi -b _CODE=0x0100 -k %SDCC_HOME%\lib\z80 -l z80 fat ucrt0.rel chario.rel bios.rel bdos.rel ff.rel diskio.rel fat.rel

move /Y fat.ihx fat.hex
zxcc mload25 fat.hex

sdldz80 -mjxi -b _CODE=0x0100 -k %SDCC_HOME%\lib\z80 -l z80 fatiotst ucrt0.rel chario.rel bios.rel bdos.rel diskio.rel fatiotst.rel

move /Y fatiotst.ihx fatiotst.hex
zxcc mload25 fatiotst.hex

:: copy fat.com C:\Users\Wayne\Desktop\