#
#

# ONE-TIME when starting gdb

#target remote localhost:2331
#target remote localhost:3333
target extended-remote :3333
#monitor halt


#monitor speed auto
#monitor endian little

set history filename gdb_history.log
set history save on
set print pretty on
set print object on
set print vtbl on
set pagination off
set output-radix 16

#set mem inaccessible-by-default off
set remotetimeout unlimited

#monitor reset 0

#Load RAM symbols
symbol-file FERMION.elf

#Add hw breakpoint if code in RRAM
#hb drv_flash_read




