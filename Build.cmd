@echo off
setlocal

set TOOLS=..\Tools
set SDCC_HOME=%TOOLS%\sdcc_430
set PATH=%SDCC_HOME%\bin;%PATH%

:: === max-allocs-per-node analysis ===
::
:: Measure-Command {.\Build.cmd | Out-Default}
::
:: Value	Result		Time
:: ------------	------------	------------
:: default	42630 bytes	32 seconds
:: 50000	40734 bytes	170 seconds
:: 100000	39972 bytes	320 seconds
:: 200000	40035 bytes	616 seconds
:: 300000	39972 bytes	890 seconds

set SDCC_OPTS=-c -mz80 --opt-code-size --verbose --no-std-crt0
set SDCC_OPTS=%SDCC_OPTS% --max-allocs-per-node 100000

sdasz80 -fflopz ucrt0.s

sdcc %SDCC_OPTS% char_cpm.c
sdcc %SDCC_OPTS% bios.c
sdcc %SDCC_OPTS% bdos.c
sdcc %SDCC_OPTS% ff.c
sdcc %SDCC_OPTS% diskio.c
sdcc %SDCC_OPTS% fat.c
sdcc %SDCC_OPTS% fatiotst.c

sdldz80 -mxi -b _CODE=0x0100 -k %SDCC_HOME%\lib\z80 -l z80 fat ucrt0.rel char_cpm.rel bios.rel bdos.rel ff.rel diskio.rel fat.rel
makebin -p -o 0x100 -s 0x10000 fat.ihx fat.com

sdldz80 -mxi -b _CODE=0x0100 -k %SDCC_HOME%\lib\z80 -l z80 fatiotst ucrt0.rel char_cpm.rel bios.rel bdos.rel diskio.rel fatiotst.rel
makebin -p -o 0x100 fatiotst.ihx fatiotst.com
