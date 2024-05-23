/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/

/******************************************************************************************************************************************

Note:
-----------------------------------------------------------------------------------------------------------------
|	* PS txt		|	Power Save code																			|
|					|	(includes SOCPM code)																	|
|---------------------------------------------------------------------------------------------------------------|
|	* PS data		|	Power Save data																			|
|					|	(includes RRI table, HW descriptor, Packet Memory(beacon), Statistics, Logs)			|
|---------------------------------------------------------------------------------------------------------------|
|	* WLAN control	|	WiFi Security, Connection Management, FreeRTOS Kernel & Ping, iPerf, UDP utilities		|
|	  & Perf txt	|	related code and constant data															|
|---------------------------------------------------------------------------------------------------------------|
|	* WLAN control	|	WiFi Security, Connection Management, FreeRTOS Kernel & Ping, iPerf, UDP utilities		|
|	  & Perf data	|	related data section																	|
-----------------------------------------------------------------------------------------------------------------

											   RAM Bank A, B, C & D

												+---------------+
												|	ram ISR		|
												|	 vector		|
												+---------------+
												|	  BSS		|
												+---------------+
												|	PS txt		|
												+---------------+
												|	PS data		|
												+---------------+
												|  System Stack	|
												+---------------+
												| WLAN control &|
												|	Perf txt	|
												+---------------+
												| WLAN control &|
												|	Perf data	|
												+---------------+
												|	Hardware	|
												| Packet Memory	|
												+---------------+
												|	 HW DS &	|
												|  Descriptors	|
												+---------------+
												|	  Heap		|
												+---------------+

******************************************************************************************************************************************/

MEMORY
{
  RAM (xrw) : ORIGIN = 0x00000000, LENGTH = 0x00080000
}
/*Ram Bank address and their size*/
_ln_cMEM_Bank_size__ = 0x00020000;
_ln_cMEM_Bank_A_start_addr__ = ORIGIN(RAM);
_ln_cMEM_Bank_B_start_addr__ = _ln_cMEM_Bank_A_start_addr__ + _ln_cMEM_Bank_size__;
_ln_cMEM_Bank_C_start_addr__ = _ln_cMEM_Bank_B_start_addr__ + _ln_cMEM_Bank_size__;
_ln_cMEM_Bank_D_start_addr__ = _ln_cMEM_Bank_C_start_addr__ + _ln_cMEM_Bank_size__;


/*
 * The '__stack' definition is required by crt0, do not remove it.
 */
__stack = _ln_RAM_stack_start_addr;

_estack = __stack; 	/* End of System Stack */

/*
 * Default stack sizes.
 * These are used by the startup in order to allocate stacks
 * for the different modes.
 */

__Main_Stack_Size = 3072 ;		/* 3 KB system stack */

PROVIDE ( _Main_Stack_Size = __Main_Stack_Size ) ;

__Main_Stack_Limit = __stack  - __Main_Stack_Size ;

/* "PROVIDE" allows to easily override these values from an
 * object file or the command line. */
PROVIDE ( _Main_Stack_Limit = __Main_Stack_Limit ) ;

/*
 * Default heap definitions, Required by _sbrk. Heap size set to 100
 */
PROVIDE ( _Heap_Begin = _noinit ) ;
PROVIDE ( _Heap_Limit = _end_noinit ) ;
/*
 * The entry point is informative, for debuggers and simulators,
 * since the Cortex-M vector points to it anyway.
 */
ENTRY(_start)

/* Sections Definitions */

SECTIONS
{

/* ---------------------------- RAM ISR vector(Debug Purpose only) ---------------------------- */
    .ram_isr_vector : ALIGN(4)         /* which goes to RAM*/
    {
        FILL(0xFF)
        __ram_vectors_start = ABSOLUTE(.) ;
        __vectors_start = ABSOLUTE(.) ;
        KEEP(*(.ram_isr_vector))            /* stack pointer and Reset Handler */
        *(.after_ram_vectors .after_ram_vectors.*)    /* minimum code and ISR */
        . = ALIGN(4) ;

    } >RAM

/* ------------------------------------- BSS -------------------------------------------------- */
    /* The primary uninitialised data section. */
    .__sect_sleep_data_retention (NOLOAD) : ALIGN(4)
    {
        _ln_bss_start__ = .;         /* standard newlib definition */

        *(.bss_begin .bss_begin.*)

        *(.bss .bss.*)
        *(COMMON)

        *(.bss_end .bss_end.*)
        . = ALIGN(4);
        _ln_bss_end__ = .;        /* standard newlib definition */

    } >RAM

    _ln_sect_sleep_data_retention_size__ = SIZEOF(.__sect_sleep_data_retention) ;

/*    ASSERT( (20K > (_ln_bss_size__)), "bss exceeds the limit")
The above ASSERT is temporarily disabled for the performance measurements, as it is known the BSS size will exceed 20K    */

/* ---------------------------- Power Save/Minimal code - text section ------------------------- */

    .__sect_ps_txt : ALIGN(4)
    {
        _ln_RAM_start_addr_ps_txt__ = ABSOLUTE(.) ;

        *nt_socpm_sleep.o(.text .text.*)
        *nt_minimum_code.o(.text .text.*)
        *nt_socpm_sleep.o(.rodata .rodata.*)
        *nt_minimum_code.o(.rodata .rodata.*)
        *(.__sect_ps_txt)

        . = ALIGN(4);
        _ln_RAM_end_addr_ps_txt__ =  . ;
    } >RAM

    _ln_ps_txt_size__ = SIZEOF(.__sect_ps_txt) ;

/* ----------------------------- Power Save/Minimal code - data section ------------------------ */

    .__sect_ps_data : ALIGN(4)
    {
        _ln_RAM_start_addr_ps_data__ = ABSOLUTE(.) ;

        *nt_socpm_sleep.o(.data .data.*)
        *nt_minimum_code.o(.data .data.*)
        *(.__sect_ps_data)                 /* includes RRI table, HW descriptors, Packet Memory(beacon), Statistics, Logs */

        . = ALIGN(4) ;
        _ln_RAM_end_addr_ps_data__ = ABSOLUTE(.) ;
    } >RAM

    _ln_ps_data_size__ = SIZEOF(.__sect_ps_data) ;

    ASSERT( (28K > (_ln_ps_data_size__)), "power save data exceeds the limit")

/* --------------------------------------- System Stack --------------------------------------- */
    .system_stack (NOLOAD) : ALIGN(4)
    {
    	_ln_RAM_stack_end_addr = ABSOLUTE(.) ;
    	. += __Main_Stack_Size ;		/* 3KB Memory reserved */
    	_ln_RAM_stack_start_addr = ABSOLUTE(.) ;
    } >RAM

    _ln_system_stack_size__ = SIZEOF(.system_stack);

    .text : ALIGN(4)
    {
        *(.after_vectors .after_vectors.*)	/* Startup code and ISR */
        *(.text .text.*)			/* all remaining code */

 		/* read-only data (constants) */
        *(.rodata .rodata.* .constdata .constdata.*)

        *(vtable)					/* C++ virtual tables */

		KEEP(*(.eh_frame*))

		/*
		 * Stub sections generated by the linker, to glue together
		 * ARM and Thumb code. .glue_7 is used for ARM code calling
		 * Thumb code, and .glue_7t is used for Thumb code calling
		 * ARM code. Apparently always generated by the linker, for some
		 * architectures, so better leave them here.
		 */
        *(.glue_7)
        *(.glue_7t)

    } >RAM

    /*
     * The initialised data section.
     *
     * The program executes knowing that the data is in the RAM
     * but the loader puts the initial values in the FLASH (inidata).
     * It is one task of the startup to copy the initial values from
     * FLASH to RAM.
     */
    .data : ALIGN(4)
    {
    	FILL(0xFF)
        /* This is used by the startup code to initialise the .data section */

        __data_start__ = . ;
		*(.data_begin .data_begin.*)

		*(.data .data.*)

		*(.data_end .data_end.*)
	    . = ALIGN(4);

	    /* This is used by the startup code to initialise the .data section */

        __data_end__ = . ;

    } >RAM

/* ----------------------------- Hardware Packet Memory ------------------------------------- */
/* Warning : Linker script reserves the memory for HW Packet Memory.
* The address is configured in hal_int_mmap.h file. */
	.__sect_hw_pktmem (NOLOAD) : ALIGN(0x80)
	{
		_ln_RAM_start_addr_hw_pktmem__ = ABSOLUTE(.) ;
		. += 32768 ;
		_ln_RAM_end_addr_hw_pktmem__ = ABSOLUTE(.) ;
	} >RAM

/* --------------------------- Hardware Descriptors/Data structures --------------------------- */
/* Warning : Linker script reserves the memory for HW Descriptors.
* The address is configured in hal_int_mmap.h file. */
	.__sect_hw_desc (NOLOAD) : ALIGN(4)
	{
		_ln_RAM_start_addr_hw_desc__ = ABSOLUTE(.) ;
		. += 22504 ;
		_ln_RAM_end_addr_hw_desc__ = ABSOLUTE(.) ;
	} >RAM

/*  Default heap definitions required by _sbrk. Heap size set to 100 */

	.noinit (NOLOAD) : ALIGN(4)
    {
        _noinit = ABSOLUTE(.);

        *(.noinit .noinit.*)
        . += 100 ;

         . = ALIGN(4) ;
        _end_noinit = .;
    } > RAM


/* ----------------------------------------- Heap Section ------------------------------------- */
	.heap : ALIGN(4)
	{
		_ln_RAM_addr_heap_start__ = ABSOLUTE(.) ;
		*(.heap)
		. = ALIGN(4) ;
		_ln_RAM_addr_heap_end__ = ABSOLUTE(.) ;
	} >RAM

	ASSERT( ((0x7FFFF - _ln_RAM_addr_heap_start__) > 100K), "ERROR : Heap is less than 100 KB")

}
