; /*
; Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
; SPDX-License-Identifier: BSD-3-Clause-Clear
;/
;*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*
;
;                           R E X  A R M   C O R E 
;
; GENERAL DESCRIPTION
;   Capture a "core" dump
;
; EXTERNALIZED FUNCTIONS
;   rex_jettison_core
;
;*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

;============================================================================
;
;                      EDIT HISTORY FOR MODULE
;
; $Header: //components/rel/core.ioe/1.0/v2/patch/drivers/debugtools/err/src/err_jettison_core_m4_patch.s#1 $
;
; when      who    what, where, why
; --------  -----  -----------------------------------------------------------
;============================================================================

;
; Any change to the arch_coredump_type structure will
; necessitate a change to REX_CORE.
;

    AREA    REX_CORE, DATA

arm_core_t RN     r7 
           MAP    0,arm_core_t
	
; Fields must match up with arch_coredump_type for ARM
; Registers should match enum arm_coredump_register_type 
arm_r0             FIELD  4
arm_r1             FIELD  4
arm_r2             FIELD  4
arm_r3             FIELD  4
arm_r4             FIELD  4
arm_r5             FIELD  4
arm_r6             FIELD  4
arm_r7             FIELD  4
arm_r8             FIELD  4
arm_r9             FIELD  4
arm_r10            FIELD  4
arm_r11            FIELD  4
arm_r12            FIELD  4
arm_sp             FIELD  4
arm_lr             FIELD  4
arm_pc             FIELD  4
arm_psp            FIELD  4
arm_msp            FIELD  4
arm_psr            FIELD  4
arm_apsr           FIELD  4
arm_ipsr           FIELD  4
arm_epsr           FIELD  4
arm_primask        FIELD  4
arm_faultmask      FIELD  4
arm_basepri        FIELD  4
arm_control        FIELD  4
arm_exception_r0   FIELD  4
arm_exception_r1   FIELD  4
arm_exception_r2   FIELD  4
arm_exception_r3   FIELD  4
arm_exception_r12  FIELD  4
arm_exception_lr   FIELD  4
arm_exception_pc   FIELD  4
arm_exception_xpsr FIELD  4

    IMPORT arch_coredump_ptr

    AREA RexRom, CODE, READONLY

;=============================================================================
; FUNCTION JETTISON_CORE
;
; DESCRIPTION
;    Captures the ARM register state
;=============================================================================
    EXPORT __patch__err_jettison_core_m4__jettison_core		
__patch__err_jettison_core_m4__jettison_core PROC	
    CPSID  i  ; set primask
    push   {r7, r8}

    ; Capture User Mode r0-r14 (no SPSR)
    ; Registers are stored in svc structure for backwards compatibility
   ldr     arm_core_t,=arch_coredump_ptr
    ldr     arm_core_t, [r7]
    cmp     r7,#0
    beq     goto_end1
    str     r0,  arm_r0
    str     r1,  arm_r1
    str     r2,  arm_r2
    str     r3,  arm_r3
    str     r4,  arm_r4
    str     r5,  arm_r5
    str     r6,  arm_r6
    ; Store r7 later after restoring it from the stack
    str     r8,  arm_r8
    str     r9,  arm_r9
    str     r10, arm_r10
    str     r11, arm_r11    
    str     r12, arm_r12

    ; Store SP value (USR mode)
    str     sp, arm_sp

    ; LR is USR (SYS) mode values.
    str     lr,  arm_lr

    ; We do not have an entry yet for the PC in user mode.Do not save this
    ; as it will cause confusion

    ; LR in all other modes will be destroyed by a syscall to determine
    ; it is contents, so ignore this for the time being

    push    {r6}

    mrs     r6,    psp
    str     r6,    arm_psp
    mrs     r6,    msp
    str     r6,    arm_msp
    mrs     r6,    psr
    str     r6,    arm_psr
    mrs     r6,    apsr
    str     r6,    arm_apsr
    mrs     r6,    epsr
    str     r6,    arm_epsr
    mrs     r6,    ipsr
    str     r6,    arm_ipsr

    mrs     r6,    primask
    str     r6,    arm_primask
    mrs     r6,    faultmask
    str     r6,    arm_faultmask
    mrs     r6,    basepri
    str     r6,    arm_basepri
    mrs     r6,    control
    str     r6,    arm_control

    pop     {r6}

    ; Store R7 into r8
    pop     {r8}
    str     r8, arm_r7

    movs    r7, r8
    pop     {r8}
    b       end

goto_end1
    push   {r7, r8}
end
    ; Finished so return
    bx      lr
    ALIGN
    ENDP

;=============================================================================
; FUNCTION ERR_DUMP_REGS_FROM_STACK
;
; DESCRIPTION
;    Dump registers from stack on an exception 
;=============================================================================
    EXPORT __patch__err_jettison_core_m4__err_dump_regs_from_stack
__patch__err_jettison_core_m4__err_dump_regs_from_stack PROC	
    CPSID  i  ; set primask
    push   { r2, r6 -r7}

    ; Registers are stored in svc structure for backwards compatibility
    ldr     arm_core_t,=arch_coredump_ptr
    ldr     arm_core_t, [r7]
    cmp     r7,#0
    beq     goto_end
    ; LR is at +20 in SP after register pushes (LR|R2|R6|R7)
    movs    r2, #16                 ; lr is pushed to stack at this location
    add     r2, r2, sp
    ldr     r2, [r2]                ; Get lr at exception entry to r2

    ; save exception entry lr
    ; here, pc is not used currently, so save to pc just in patch code 
    str     r2, arm_pc

    cmp     r2, #0xFFFFFFF9         ; 2nd bit is not set, use MSP
    beq     use_msp
    cmp     r2, #0xFFFFFFE9         ; 2nd bit is not set, and FP is enabled, use MSP
    beq     use_msp
    cmp     r2, #0xFFFFFFFD         ; 2nd bit is not set, use PSP
    beq     use_psp
    cmp     r2, #0xFFFFFFED         ; 2nd bit is not set, and FP is enabled, use PSP
    bne     goto_end                ; Bail out, if this is a nested exception or unknown value
use_psp
    mrs     r2, psp                 ; Move to register location in stack
    cmp     r2, #0                  ; check is psp is zero
    beq     goto_end                ; Bail out, if psp is zero
    b       get_reg
use_msp
    mrs     r2, msp                 ; Move to register location in stack
    cmp     r2, #0                  ; check is msp is zero
    beq     goto_end                ; Bail out, if msp is zero
    ; Regs are at +20 in SP after register pushes (LR|R2|R6|R7)
    add     r2, r2, #20
get_reg
    ldr     r6, [r2], #4
    str     r6, arm_exception_r0
    ldr     r6, [r2], #4
    str     r6, arm_exception_r1
    ldr     r6, [r2], #4
    str     r6, arm_exception_r2
    ldr     r6, [r2], #4
    str     r6, arm_exception_r3
    ldr     r6, [r2], #4
    str     r6, arm_exception_r12
    ldr     r6, [r2], #4
    str     r6, arm_exception_lr
    ldr     r6, [r2], #4
    str     r6, arm_exception_pc
    ldr     r6, [r2], #4
    str     r6, arm_exception_xpsr

goto_end
    pop     { r2, r6-r7} 
    
    ; Finished so return
    bx      lr
    ALIGN
    ENDP
 
;======================================================================
    END
;======================================================================
