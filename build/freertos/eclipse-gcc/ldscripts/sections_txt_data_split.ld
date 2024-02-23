/*
*/


/****************************************************************************************************************************************************

Note:
+---------------------------------------------------------------------------------------------------------------+
|	* PS txt		|	Power Save code																			|
|					|	(includes SOCPM code)																	|
|---------------------------------------------------------------------------------------------------------------|
|	* PS data		|	Power Save data																			|
|					|	(includes RRI table, HW descriptor, Packet Memory(beacon))								|
|---------------------------------------------------------------------------------------------------------------|
|	* App txt		|	Customer RAM resident applications code													|
|					|	(includes Ping, TCP utility, UDP utility, WiFi App)										|
|---------------------------------------------------------------------------------------------------------------|
|	* App data		|	Customer RAM resident applications data													|
|---------------------------------------------------------------------------------------------------------------|
|	* Perf txt		|	Performance code																		|
|					|	(includes FreeRTOS kernel, LWIP, Data Path, Nano lib)									|
|---------------------------------------------------------------------------------------------------------------|
|	* Perf data		|	Performance data 																		|
|---------------------------------------------------------------------------------------------------------------|
|	* Sleep data	|	Includes BSS, Logs, Statistics, Heap contents during the power save						|
|	Retention Region|																							|
+---------------------------------------------------------------------------------------------------------------+

RRAM Starting Address	- 0x0020 0000
RAM A Starting Address	- 0x0000 0000
RAM B Starting Address	- 0x0002 0000
RAM C Starting Address	- 0x0004 0000
RAM D Starting Address	- 0x0006 0000

									  RRAM					 RAM A, B, C & D
								+---------------+			+---------------+
								|	  PBL		|			|	ram ISR		|
								+---------------+			|	 vector		|
								|  File System	|			+---------------+
								+---------------+			|	PS txt		|
								|	Flash ISR	|			+---------------+
								|	 vector		|			|	PS data		|
								+---------------+			+---------------+
								|	PS txt		|			| System Stack	|
								+---------------+			+---------------+
								|	PS data		|			|	Sleep Data	|
								+---------------+			|	retention	|
								|	Perf txt	|			|	 region		|
								+---------------+			+---------------+
								|	Perf data	|			|	Perf txt	|
								+---------------+			+---------------+
								|	App txt		|			|	Perf data	|
								+---------------+			+---------------+
								|	App data	|			|	App txt		|
								+---------------+			+---------------+
								|	WLAN Ctrl &	|			|	App data	|
								|	 System		|			+---------------+
								|	  text   	|			|	  Packet	|
								+---------------+			|	  Memory	|
								|	WLAN Ctrl &	|			+---------------+
								|	 System		|			|	 HW	DS &	|
								|	  data		|			|	Descriptors	|
								+---------------+			+---------------+
															|	   Heap		|
															+---------------+

linker symbol to address space interpretation
--------------------------------------------
_ln_RF_*	- represents address in RRAM_FLASH
_ln_RAM_*	- represents address in RAM

****************************************************************************************************************************************************/

/*
* Stack Allocation for PBL.
* Placed at top of the heap so that the memory can be reused by the mission mode image
*/


_ln_PBL_stack_limit__ = _ln_RAM_addr_heap_start__;
_ln_PBL_stack_size__ = 0x200 ;
_ln_PBL_stack__  =  _ln_PBL_stack_limit__ + _ln_PBL_stack_size__ ;


 /* Mount Address for the File System */
_ln_FS_Start_Addr = ORIGIN(FSMEM) ;


/* CAL data of the system */
_ln_CAL_Start_Addr = ORIGIN(CALMEM) ;
_ln_CAL_Data_length = LENGTH(CALMEM) ;


/* RRAM Main Region */
__rram_region_st_address = ORIGIN(PBLMEM) ;
__rram_region_end_address = ORIGIN(PBLMEM) + LENGTH(PBLMEM) + LENGTH(FDTMEM) + LENGTH(FSMEM) + LENGTH(CALMEM) + LENGTH(RRAM_FLASH);


/*Neutrino OTP region address start*/
 __rram_OTP_region_st_address = ORIGIN(OTPMEM);
 __rram_OTP_region_end_address = ORIGIN(OTPMEM) + LENGTH(OTPMEM);


/*
 * The '__stack' definition is required by crt0, do not remove it.
 */
__stack = _ln_RAM_stack_start_addr ;
_estack = __stack; /* End of System Stack */

/*
 * Default stack sizes.
 * These are used by the startup in order to allocate stacks
 * for the different modes.
 */

__Main_Stack_Size = 3072 ;        /* 3 KB system stack */


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

    .PBL_code_data : ALIGN(4)
    {
        KEEP(*(.pbl_isr_vector))
        KEEP(*(.pbl_after_vectors))

        *\\pbl_main.o(.text .text.*)
        neutrino_pbl\\system\\*.o(.text .text.*)

        *\\pbl_main.o(.constdata .constdata.* .rodata .rodata.*)
        neutrino_pbl\\system\\*.o(.constdata .constdata.* .rodata .rodata.*)

        *\\pbl_main.o(.data .data.*)
        neutrino_pbl\\system\\*.o(.data .data.*)

    } >PBLMEM

/* ------------------------------------- ISR Vector ---------------------------------------------*/
    /*
     * For Cortex-M devices, the beginning of the startup code is stored in
     * the .flash_isr_vector section, which goes to RRAM(FLASH).
     */
    .flash_isr_vector : ALIGN(4)
    {
        FILL(0xFF)
        _ln_RF_flash_isr_vector_start__ = . ;

       __vectors_start = ABSOLUTE(.) ;
        KEEP(*(.flash_isr_vector))         /* Interrupt vectors */

        . = ALIGN(4) ;
        _ln_RF_flash_isr_vector_end__ = . ;

    } >RRAM_FLASH

    _ln_isr_vector_size__ = SIZEOF(.flash_isr_vector) ;

    _ln_addr_map_to_ram = 0x00000000;

/* ---------------------------- RAM ISR vector(Debug Purpose only) ---------------------------- */

    .ram_isr_vector : ALIGN(4)         /* which goes to RAM*/
    {
        FILL(0xFF)
        __ram_vectors_start = ABSOLUTE(.) ;
        KEEP(*(.ram_isr_vector))            /* stack pointer and Reset Handler */
		*(.after_ram_vectors .after_ram_vectors.*)    /* minimum code and ISR */
        . = ALIGN(4) ;

    } >RAM


/* ---------------------------- Power Save/Minimal code - text section ------------------------- */

    _ln_RF_start_addr_ps_txt__ = LOADADDR(.__sect_ps_txt);

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
    } >RAM AT> RRAM_FLASH

    _ln_ps_txt_size__ = SIZEOF(.__sect_ps_txt) ;

/* --------------------------------------- System Stack --------------------------------------- */

    .system_stack (NOLOAD) : ALIGN(4)
    {
    	_ln_RAM_stack_end_addr = ABSOLUTE(.) ;
    	. += __Main_Stack_Size ;		/* 3KB Memory reserved */
    	_ln_RAM_stack_start_addr = ABSOLUTE(.) ;
    } >RAM


/* ---------------------------- Sleep Data Retention section --------------------------------- */

    /* The primary uninitialised data section. */
    .__sect_sleep_data_retention (NOLOAD) : ALIGN(4)
    {
        _ln_bss_start__ = .;         /* standard newlib definition */

        *(.bss_begin .bss_begin.*)

        EXCLUDE_FILE(*dxe.o) *(.bss .bss.*)
        EXCLUDE_FILE(*dxe.o) *(COMMON)

        *(.bss_end .bss_end.*)
        . = ALIGN(4);
        _ln_bss_end__ = .;        /* standard newlib definition */

    } >RAM

    _ln_sect_sleep_data_retention_size__ = SIZEOF(.__sect_sleep_data_retention) ;

/*    ASSERT( (20K > (_ln_bss_size__)), "bss exceeds the limit")
The above ASSERT is temporarily disabled for the performance measurements, as it is known the BSS size will exceed 20K    */

/* ---------------------------------- Performance code - Text Section -------------------------------- */

	_ln_RF_start_addr_perf_txt__ = LOADADDR(.__sect_perf_txt);

    .__sect_perf_txt : ALIGN(4)
    {
        _ln_RAM_start_addr_perf_txt__ = ABSOLUTE(.) ;

        os\\freertos\\freertos_kernel\\*.o(.text .text.*)
        os\\freertos\\libraries\\3rdparty\\lwip\\src\\api\\*.o(.text .text.*)
        os\\freertos\\libraries\\3rdparty\\lwip\\src\\core\\*.o(.text .text.*)
        os\\freertos\\libraries\\3rdparty\\lwip\\src\\netif\\*.o(.text .text.*)
        os\\freertos\\libraries\\3rdparty\\lwip\\src\\portable\\*.o(.text .text.*)
        core\\wifi\\dpm\\*.o(.text .text.*)
        *\\libg_nano.a:*(.text .text.*)

        os\\freertos\\freertos_kernel\\*.o(.rodata .rodata.* .constdata .constdata.*)
        os\\freertos\\libraries\\3rdparty\\lwip\\src\\api\\*.o(.rodata .rodata.* .constdata .constdata.*)
        os\\freertos\\libraries\\3rdparty\\lwip\\src\\core\\*.o(.rodata .rodata.* .constdata .constdata.*)
        os\\freertos\\libraries\\3rdparty\\lwip\\src\\netif\\*.o(.rodata .rodata.* .constdata .constdata.*)
        os\\freertos\\libraries\\3rdparty\\lwip\\src\\portable\\*.o(.rodata .rodata.* .constdata .constdata.*)
        core\\wifi\\dpm\\*.o(.rodata .rodata.* .constdata .constdata.*)
        *\\libg_nano.a:*(.rodata .rodata.* .constdata .constdata.*)

        . = ALIGN(4);
        _ln_RAM_end_addr_perf_txt__ = ABSOLUTE(.) ;
    } >RAM AT> RRAM_FLASH

    _ln_perf_txt_size__ = SIZEOF(.__sect_perf_txt) ;

/* ---------------------------------- RAM resident Apps text section --------------------------- */

	_ln_RF_start_addr_app_txt__ = LOADADDR(.__sect_app_txt);

	.__sect_app_txt : ALIGN(4)
	{
		_ln_RAM_start_addr_app_txt__ = ABSOLUTE(.) ;

		os\\freertos\\libraries\\3rdparty\\lwip\\src\\apps\\*.o(.text .text.*)
		os\\freertos\\libraries\\ping\\*.o(.text .text.*)
		apps\\WiFi_App\\*.o(.text .text.*)

		os\\freertos\\libraries\\3rdparty\\lwip\\src\\apps\\*.o(.rodata .rodata.* .constdata .constdata.*)
		*ping.o(.rodata .rodata.* .constdata .constdata.*)
		apps\\WiFi_App\\*.o(.rodata .rodata.* .constdata .constdata.*)

		. = ALIGN(4) ;

		_ln_RAM_end_addr_app_txt__ = ABSOLUTE(.) ;
	} >RAM AT>RRAM_FLASH

	_ln_app_txt_size__ = SIZEOF(.__sect_app_txt) ;

/* ------------------------- WLAN Control & System Text Section -------------------------------- */

    .text : ALIGN(4)
    {
        *(.after_vectors .after_vectors.*)	/* Startup code and ISR */

/* Warning : The text, rodata and constdata sections of the excluded files are placed into different sections intentionally.
* Changing them will have effect on the performance and system operation */

        EXCLUDE_FILE(os\\freertos\\libraries\\3rdparty\\lwip\\*.o core\\wifi\\dpm\\*.o os\\freertos\\freertos_kernel\\*.o) *(.text .text.*)			/* all remaining code */
		EXCLUDE_FILE(*ping.o apps\\WiFi_App\\*.o *\\libg_nano.a:*) *(.text .text.*)

 		/* read-only data (constants) */
        EXCLUDE_FILE(os\\freertos\\libraries\\3rdparty\\lwip\\*.o core\\wifi\\dpm\\*.o os\\freertos\\freertos_kernel\\*.o) *(.rodata .rodata.* .constdata .constdata.*)
		EXCLUDE_FILE(*ping.o apps\\WiFi_App\\*.o *\\libg_nano.a:*) *(.rodata .rodata.* .constdata .constdata.*)

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
        . = ALIGN(4);
        _ln_text_end__ = . ;

    } >RRAM_FLASH

/* ----------------------------- Power Save/Minimal code - data section ------------------------ */

    _ln_RF_start_addr_ps_data__ = LOADADDR(.__sect_ps_data);

    .__sect_ps_data : ALIGN(4)
    {
        _ln_RAM_start_addr_ps_data__ = ABSOLUTE(.) ;

        *nt_socpm_sleep.o(.data .data.*)
        *nt_minimum_code.o(.data .data.*)
        *(.__sect_ps_data)                 /* includes RRI table, HW descriptors, Packet Memory(beacon), Statistics, Logs */

        . = ALIGN(4) ;
        _ln_RAM_end_addr_ps_data__ = ABSOLUTE(.) ;
    } >RAM AT> RRAM_FLASH

    _ln_ps_data_size__ = SIZEOF(.__sect_ps_data) ;

    ASSERT( (28K > (_ln_ps_data_size__)), "power save data exceeds the limit")

/* ------------------------------- Performance code - Data Section ------------------------------------ */

    _ln_RF_start_addr_perf_data__ = LOADADDR(.__sect_perf_data);

    .__sect_perf_data : ALIGN(4096)
    {
        _ln_RAM_start_addr_perf_data__ = ABSOLUTE(.) ;

        os\\freertos\\freertos_kernel\\*.o(.data .data.*)
        os\\freertos\\libraries\\3rdparty\\lwip\\src\\api\\*.o(.data .data.*)
        os\\freertos\\libraries\\3rdparty\\lwip\\src\\core\\*.o(.data .data.*)
        os\\freertos\\libraries\\3rdparty\\lwip\\src\\netif\\*.o(.data .data.*)
        os\\freertos\\libraries\\3rdparty\\lwip\\src\\portable\\*.o(.data .data.*)
        core\\wifi\\dpm\\*.o(.data .data.*)
        *\\libg_nano.a:*(.data .data.*)

        . = ALIGN(4);
        _ln_RAM_end_addr_perf_data__ = ABSOLUTE(.) ;
    } >RAM AT> RRAM_FLASH

    _ln_perf_data_size__ = SIZEOF(.__sect_perf_data) ;

/* ----------------------------------- RAM resident Apps data section --------------------------- */

	_ln_RF_start_addr_app_data__ = LOADADDR(.__sect_app_data) ;

	.__sect_app_data : ALIGN(4)
	{
		_ln_RAM_start_addr_app_data__ = ABSOLUTE(.) ;

		os\\freertos\\libraries\\3rdparty\\lwip\\src\\apps\\*.o(.data .data.*)
		*ping.o(.data .data.*)
		apps\\WiFi_App\\*.o(.data .data.*)

		. = ALIGN(4) ;

		_ln_RAM_end_addr_app_data__ = ABSOLUTE(.) ;
	} >RAM AT> RRAM_FLASH

	_ln_app_data_size__ = SIZEOF(.__sect_app_data) ;

/* ---------------------------- WLAN Control & System Data Section ----------------------------- */
    .data : ALIGN(4)
    {
    	FILL(0xFF)
        /* This is used by the startup code to initialise the .data section */

        _ln_data_start__ = . ;
		*(.data_begin .data_begin.*)

/* Warning : The data sections of the excluded files are placed into different sections intentionally.
* Changing them will have effect on the performance and system operation */

		EXCLUDE_FILE(os\\freertos\\libraries\\3rdparty\\lwip\\*.o core\\wifi\\dpm\\*.o os\\freertos\\freertos_kernel\\*.o) *(.data .data.*)
		EXCLUDE_FILE(*ping.o apps\\WiFi_App\\*.o *\\libg_nano.a:*) *(.data .data.*)

		*(.data_end .data_end.*)
	    . = ALIGN(4);

	    /* This is used by the startup code to initialise the .data section */
        _ln_data_end__ = . ;

    } >RRAM_FLASH

    .critical_log : ALIGN(4)
	{
		FILL(0x0)
		_logs_crit = ABSOLUTE(.);
	} >RRAM_FLASH

/* ----------------------------- Hardware Packet Memory ------------------------------------- */
/* Warning : Linker script reserves the memory for HW Packet Memory.
* The address is configured in hal_int_mmap.h file. */
	.__sect_dxe_desc (NOLOAD):ALIGN(4096)
	{
	   *dxe.o(.bss .bss.*)
	} >RAM

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

	ASSERT( ((0x7FFFF - _ln_RAM_addr_heap_start__) > 170K), "ERROR : Heap is less than 170 KB")

    /* Mandatory to be word aligned, _sbrk assumes this */
    PROVIDE ( end = _end_noinit ); /* was _ebss */
    PROVIDE ( _end = _end_noinit );
    PROVIDE ( __end = _end_noinit );
    PROVIDE ( __end__ = _end_noinit );

    /* After that there are only debugging sections. */

    /* This can remove the debugging information from the standard libraries */


    /* Stabs debugging sections.  */
    .stab          0 : { *(.stab) }
    .stabstr       0 : { *(.stabstr) }
    .stab.excl     0 : { *(.stab.excl) }
    .stab.exclstr  0 : { *(.stab.exclstr) }
    .stab.index    0 : { *(.stab.index) }
    .stab.indexstr 0 : { *(.stab.indexstr) }
    .comment       0 : { *(.comment) }
    /*
     * DWARF debug sections.
     * Symbols in the DWARF debugging sections are relative to the beginning
     * of the section so we begin them at 0.
     */
    /* DWARF 1 */
    .debug          0 : { *(.debug) }
    .line           0 : { *(.line) }
    /* GNU DWARF 1 extensions */
    .debug_srcinfo  0 : { *(.debug_srcinfo) }
    .debug_sfnames  0 : { *(.debug_sfnames) }
    /* DWARF 1.1 and DWARF 2 */
    .debug_aranges  0 : { *(.debug_aranges) }
    .debug_pubnames 0 : { *(.debug_pubnames) }
    /* DWARF 2 */
    .debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
    .debug_abbrev   0 : { *(.debug_abbrev) }
    .debug_line     0 : { *(.debug_line) }
    .debug_frame    0 : { *(.debug_frame) }
    .debug_str      0 : { *(.debug_str) }
    .debug_loc      0 : { *(.debug_loc) }
    .debug_macinfo  0 : { *(.debug_macinfo) }
    /* SGI/MIPS DWARF 2 extensions */
    .debug_weaknames 0 : { *(.debug_weaknames) }
    .debug_funcnames 0 : { *(.debug_funcnames) }
    .debug_typenames 0 : { *(.debug_typenames) }
    .debug_varnames  0 : { *(.debug_varnames) }
}
