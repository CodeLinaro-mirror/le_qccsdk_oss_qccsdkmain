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
symbol-file FERMION_IOE_QCLI_DEMO.elf

#hb drv_flash_read
set *(uint32_t*)0x11AF8E0=0

# For ramdumps that are a result of a catastrophic exception:
define coreregs
  if (coredump != 0)
    # Show M4's General Purpose Registers at the time of dump
    echo \n\nM4 coredump area:\n
    p *coredump

    set $r4=coredump->arch->regs->name->regs[4]
    set $r5=coredump->arch->regs->name->regs[5]
    set $r6=coredump->arch->regs->name->regs[6]
    set $r7=coredump->arch->regs->name->regs[7]
    set $r8=coredump->arch->regs->name->regs[8]
    set $r9=coredump->arch->regs->name->regs[9]
    set $r10=coredump->arch->regs->name->regs[10]
    set $r11=coredump->arch->regs->name->regs[11]

    if (coredump->arch->regs->name->exception_lr != 0)
      set $r0=coredump->arch->regs->name->exception_r0
      set $r1=coredump->arch->regs->name->exception_r1
      set $r2=coredump->arch->regs->name->exception_r2
      set $r3=coredump->arch->regs->name->exception_r3
      set $r12=coredump->arch->regs->name->exception_r12
      set $lr=coredump->arch->regs->name->exception_lr
    
      set $exception_entry_lr=coredump->arch->regs->name->pc
      set $exception_entry_pc=coredump->arch->regs->name->exception_pc
      set $xpsr=coredump->arch->regs->name->exception_xpsr
   
      if (($exception_entry_lr==0xFFFFFFF9) || ($exception_entry_lr==0xFFFFFFE9))
        set $sp=coredump->arch->regs->name->msp+20+0x20
      else
        if (($exception_entry_lr==0xFFFFFFFD) || ($exception_entry_lr==0xFFFFFFED))
          set $sp=coredump->arch->regs->name->psp+0x20
        else
          if ($exception_entry_lr==0)
            printf "Warning: sp may be wrong, need err_jettison_core_m4_patch.s \n"
            set $sp=coredump->arch->regs->name->psp+0x20
          else
            printf "Error: exception_entry_lr(saved in pc) is 0x%x, not supported \n", $exception_entry_lr
          end
        end
      end
      set $pc=$exception_entry_pc
    else
      set $r0=coredump->arch->regs->name->regs[0]
      set $r1=coredump->arch->regs->name->regs[1]
      set $r2=coredump->arch->regs->name->regs[2]
      set $r3=coredump->arch->regs->name->regs[3]
      set $r12=coredump->arch->regs->name->regs[12]
      set $lr=coredump->arch->regs->name->lr
      set $sp=coredump->arch->regs->name->sp+0x20

      if ((unsigned int)coredump->arch->regs->name->pc < 1024)
        set $pc=coredump->arch->regs->name->lr
      else
        set $pc=coredump->arch->regs->name->pc
      end
    end
  end
end




