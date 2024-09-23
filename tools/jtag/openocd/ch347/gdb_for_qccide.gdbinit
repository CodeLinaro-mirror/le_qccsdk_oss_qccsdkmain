# 
#  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
#  SPDX-License-Identifier: BSD-3-Clause-Clear
# 
 

# ONE-TIME when starting gdb
set target-async on
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
# symbol-file FERMION_QCLI_DEMO.elf

hb drv_flash_read