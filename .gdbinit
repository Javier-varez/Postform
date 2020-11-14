target extended-remote | openocd -c "gdb_port pipe" -f "openocd.cfg"
monitor reset halt
load
monitor reset halt
set step-mode on
tui enable
layout asm
layout regs

