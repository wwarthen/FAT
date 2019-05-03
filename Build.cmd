@echo off
setlocal

set TOOLS=..\Tools
set SDCC_HOME=%TOOLS%\sdcc_390
set PATH=%SDCC_HOME%\bin;%TOOLS%\zx;%TOOLS%\hex2bin;%PATH%

set ZXBINDIR=%TOOLS%/cpm/bin/
set ZXLIBDIR=%TOOLS%/cpm/lib/
set ZXINCDIR=%TOOLS%/cpm/include/

sdasz80 -fflopz ucrt0.s

sdcc -c -mz80 chario.c
sdcc -c -mz80 bios.c
sdcc -c -mz80 bdos.c
if not exist ff.rel sdcc -c -mz80 ff.c
sdcc -c -mz80 diskio.c
sdcc -c -mz80 fat.c

sdldz80 -mjxi -b _CODE=0x0100 -k %SDCC_HOME%\lib\z80 -l z80 fat ucrt0.rel chario.rel bios.rel bdos.rel ff.rel diskio.rel fat.rel

move /Y fat.ihx fat.hex
zx mload25 fat.hex

copy fat.com ..\RomWBW\