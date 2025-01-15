/*
*
* Notifications and licenses are retained for attribution purposes only
*/


/*
 * This file is part of the µOS++ distribution.
 *   (https://github.com/micro-os-plus)
 * Copyright (c) 2014 Liviu Ionescu.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */


// ----------------------------------------------------------------------------

// For this to be called, the project linker must be configured without
// the startup sequence (-nostartfiles).

// ----------------------------------------------------------------------------

#include <assert.h>
#include <stdint.h>
#include <sys/types.h>
#include <string.h>

#include "nt_common.h"

#include "fwconfig_cmn.h"
#include "nt_flags.h"

#include "safeAPI.h"

/*********************** Symbols from Linker Script *********************************************/

// Begin address for the .bss section; defined in linker script
extern unsigned int _ln_bss_start__;
extern unsigned int _ln_sect_sleep_data_retention_size__;

extern unsigned int __Main_Stack_Limit;
extern unsigned int _ln_system_stack_size__;

extern unsigned int _ln_RF_start_addr_ps_txt__;					//start address of ps text in RRAM
extern unsigned int _ln_RAM_start_addr_ps_txt__;				//start address to load ps text in RAM
extern unsigned int _ln_ps_txt_size__;							//ps text size

extern unsigned int _ln_RF_start_addr_ps_data__;				//start address ps data in RRAM
extern unsigned int _ln_RAM_start_addr_ps_data__;				//start address to load ps data in RAM
extern unsigned int _ln_ps_data_size__;							//ps data size

#ifndef NT_FN_RRAM_PERF_BUILD
extern unsigned int _ln_RF_start_addr_app_txt__;				//start address of apps text in RRAM
extern unsigned int _ln_RAM_start_addr_app_txt__;				//start address to load apps text in RAM
extern unsigned int _ln_app_txt_size__;							//apps text size
#endif

extern unsigned int _ln_RF_start_addr_app_data__;				//start address apps data in RRAM
extern unsigned int _ln_RAM_start_addr_app_data__;				//start address to load apps data in RAM
extern unsigned int _ln_app_data_size__;						//apps data size

#ifndef NT_FN_RRAM_PERF_BUILD
extern unsigned int _ln_RF_start_addr_perf_txt__;				//start address of perf text in RRAM
extern unsigned int _ln_RAM_start_addr_perf_txt__;				//start address to load perf text in RAM
extern unsigned int _ln_perf_txt_size__;						//perf text size
#endif

extern unsigned int _ln_RF_start_addr_perf_data__;				//start address perf data in RRAM
extern unsigned int _ln_RAM_start_addr_perf_data__;				//start address to load perf data in RAM
extern unsigned int _ln_perf_data_size__;						//perf data size

extern unsigned int _ln_RF_start_addr_data__ ;                  //start address data section in RRAM
extern unsigned int _ln_RAM_start_addr_data__ ;                 //start address to load data in RAM
extern unsigned int _ln_data_size__ ;                           //data section size

//-----------------------------------------------------------------------------
#ifdef NT_MULTI_IMAGE
	typedef struct
	{
		uint32_t p_index_size;     /* Segment Index */
	}Elf32_pindex;
	typedef struct
	{
	   uint32_t p_type; /* Segment type */
	   uint32_t p_offset; /* Segment file offset */
	   uint32_t p_vaddr; /* Segment virtual address */
	   uint32_t p_paddr; /* Segment physical address */
	   uint32_t p_filesz; /* Segment size in file */
	   uint32_t p_memsz; /* Segment size in memory */
	   uint32_t p_flags; /* Segment flags */
	   uint32_t p_align; /* Segment alignment */
	} Elf32_Phdr;

	Elf32_Phdr PrgHdrArr[14] = {0};
#endif

extern char pbl_log_buff[256];
#define PBL_LOG_REGION_CMEMA 0x168
#define NT_IMG_UPDATE_CHECK_ADD	( 0x2043FC )
#define CHECK_BIT_SET(value, pos) ( value & (1 << pos) )

extern void
__initialize_args (int*, char***);

// main() is the entry point for newlib based applications.
// By default, there are no arguments, but this can be customised
// by redefining __initialize_args(), which is done when the
// semihosting configurations are used.
extern int
main (int argc, char* argv[]);

// The implementation for the exit routine; for embedded
// applications, a system reset will be performed.
extern void
__attribute__((noreturn))
_exit (int);

// ----------------------------------------------------------------------------

// Forward declarations

void
_start (void* arg);

// ----------------------------------------------------------------------------
inline void
__attribute__((always_inline))
__copy_from_rram_to_ram (
	unsigned int* source_address,
	unsigned int* dest_address,
	unsigned int* block_size)
{
  // Iterate and copy word by word.
  unsigned int *src = source_address;
  unsigned int *dst = dest_address;
  unsigned int size = block_size;
  unsigned int copy_count = 0;

  for(copy_count = 0; copy_count < size; copy_count = copy_count + 4)
  {
     *dst++ = *src++;
  }
}

#if CONFIG_PBL_PREES_RESET_FOR_DTIM
void early_reset();
#endif

// This is the place where Cortex-M core will go immediately after reset,
// via a call or jump from the Reset_Handler.

/**
 * @FUNCTION    :	_start
 * @description :	Control reaches here from the reset handler via jump from PBL.
 * 					Initialise the system, clear the BSS region,
 * 					Load the Power Save, Performance and Apps text & data from RRAM to RAM,
 *					Fill the system stack with known marker value, Config SysTick and then branch to main().
 * @Param      	:	NONE
 * @return     	:	NONE
 */
#define APP_ARGS_MAGIC	(0x55aa55aa)
//#define GET_APP_ARGS_BIN_MODE(args)	(((uint32_t)(args))&0xff)
//#define GET_APP_ARGS_MAGIC(args)	((((uint32_t)(args))>>8)&0xff)
enum ota_image_format {
	OTA_IMG_FORMAT_ELF,
	OTA_IMG_FORMAT_BIN,
};

#define SBL_SHARE_VER 1

#define PART_SIZE   10 /**< part map size */

// define struct with id and addr
typedef struct {
    uint32_t id;
    uint32_t addr;
} IDAddr;

typedef struct {
	uint32_t magic_num;
	uint8_t ver;
	uint8_t img_type;
	uint8_t rsv1;
	uint8_t rsv2;
	uint32_t bdf_addr;
	IDAddr fdt_part[PART_SIZE];
} boot_sbl_share;

uint32_t bdf_addr;
IDAddr fdt_part[PART_SIZE];

void __attribute__ ((section(".after_vectors"),noreturn,weak))
_start (void* arg)
{
   // int int_number;
    extern  uint32_t load_r13[2];
    uint32_t temp = load_r13[0];
    boot_sbl_share sbl_share = *((boot_sbl_share *)arg);
    uint8_t temp_log_arr[256] = {0};
    uint32_t pbl_log_count = 0;
    uint8_t * pbl_log_addr = PBL_LOG_REGION_CMEMA;

#if CONFIG_PBL_PREES_RESET_FOR_DTIM
    early_reset();
#endif

    __initialize_hardware_early ();

    temp_log_arr[0] = *pbl_log_addr;

    if(temp_log_arr[0]>1)
    {
    for( pbl_log_count = 1; pbl_log_count <= ( temp_log_arr[0] - 1 ); pbl_log_count++ )
    {
    	temp_log_arr[pbl_log_count] = *(pbl_log_addr + pbl_log_count);
    }
    }

    nt_mem_bank_on(0); // power on without tests

	#ifdef  NT_MULTI_IMAGE
    #error "ERROR due to cleaning _ln_bss_start__ missed"
		uint8_t img_load_method = NT_REG_RD(NT_IMG_UPDATE_CHECK_ADD);

		if( CHECK_BIT_SET(img_load_method, 1) )
		{
			uint8_t indx = 0;
			/* Read the total number of program headers for the image from FDT
			* and load to the cMEM
			* */
			Elf32_pindex p_indx;
			uint32_t start_addr = 0x20403c;
			p_indx = *(Elf32_pindex*)0x204000;
			for( indx=0; indx < (uint8_t*)p_indx.p_index_size; indx++){
				PrgHdrArr[indx]=*(Elf32_Phdr*) (start_addr + (indx*0x20));
			}
			indx = 0;
			while(indx < (uint8_t*)p_indx.p_index_size)// loop to traverse program headers and load the code/data
			{
				 if (PrgHdrArr[indx].p_vaddr != PrgHdrArr[indx].p_paddr)
				 {
					 memscpy(PrgHdrArr[indx].p_vaddr, PrgHdrArr[indx].p_filesz, PrgHdrArr[indx].p_paddr, PrgHdrArr[indx].p_filesz);
				 }
				 indx++;
			}
		}
		else
	#endif // NT_MULTI_IMAGE
		{
			/* this may come from wdog reset */
			if(!(sbl_share.ver >= SBL_SHARE_VER))
				nt_system_sw_reset();
				
			uint8_t app_arg_bin_mode = sbl_share.img_type;
			uint32_t app_arg_magic = sbl_share.magic_num;

			uint8_t do_memload = 1;
			if ((APP_ARGS_MAGIC==app_arg_magic) && (OTA_IMG_FORMAT_ELF==app_arg_bin_mode)) {
				do_memload = 0;
			}
			if (do_memload==1) {
				#if !(defined NT_CMEM_BUILD)
					__copy_from_rram_to_ram(&_ln_RF_start_addr_ps_txt__, &_ln_RAM_start_addr_ps_txt__, &_ln_ps_txt_size__); //copying ram isr vector and ps text
					__copy_from_rram_to_ram(&_ln_RF_start_addr_ps_data__, &_ln_RAM_start_addr_ps_data__, &_ln_ps_data_size__); //copying ps data
#ifndef NT_FN_RRAM_PERF_BUILD
					__copy_from_rram_to_ram(&_ln_RF_start_addr_app_txt__, &_ln_RAM_start_addr_app_txt__, &_ln_app_txt_size__); //copying apps txt
#endif
					__copy_from_rram_to_ram(&_ln_RF_start_addr_app_data__, &_ln_RAM_start_addr_app_data__, &_ln_app_data_size__); //copying apps data
#ifndef NT_FN_RRAM_PERF_BUILD
					__copy_from_rram_to_ram(&_ln_RF_start_addr_perf_txt__, &_ln_RAM_start_addr_perf_txt__, &_ln_perf_txt_size__); //copying perf text
#endif
					__copy_from_rram_to_ram(&_ln_RF_start_addr_perf_data__, &_ln_RAM_start_addr_perf_data__, &_ln_perf_data_size__); //copying perf data
					__copy_from_rram_to_ram(&_ln_RF_start_addr_data__, &_ln_RAM_start_addr_data__, &_ln_data_size__);                   //copying data section
				#endif //NT_CMEM_BUILD
				// Zero fill the BSS section (inlined).
				memset(&_ln_bss_start__,0x0,&_ln_sect_sleep_data_retention_size__);
			}
		}

	bdf_addr = sbl_share.bdf_addr;
	
	memcpy(fdt_part, sbl_share.fdt_part, sizeof(sbl_share.fdt_part));

	// Copying of pbl_log from temp_buffer to pbl_log_buffer
	memscpy(&pbl_log_buff[0], temp_log_arr[0], &temp_log_arr[0], temp_log_arr[0] );
    // Fill the stack with known value
	memset(&__Main_Stack_Limit,0xa5a5a5a5,&_ln_system_stack_size__);

#if defined(CONFIG_HEAP_STATISTIC)
{
	#ifdef PLATFORM_FERMION
	#define HEAP_END_ADDR   ( 0x9FFFF )
	#else
	#define HEAP_END_ADDR	( 0x7FFFF )
	#endif //PLATFORM_FERMION

	extern unsigned char _ln_RAM_addr_heap_start__;

	uint32_t i, *heap_addr;

       heap_addr = &_ln_RAM_addr_heap_start__;

 	for (i = 0; i < ((HEAP_END_ADDR+1)-(uint32_t)&_ln_RAM_addr_heap_start__)/4; i++)
 	{
 	    *heap_addr = 0xDEADBEEF;
 	    heap_addr++;
 	}
}
#endif

	load_r13[0] = temp;

	// Get the argc/argv (useful in semihosting configurations).
	int argc;
	char** argv;
	__initialize_args (&argc, &argv);

	int code = main (argc, argv);

	_exit (code);

	// Should never reach this, _exit() should have already
	// performed a reset.
	for (;;);

}

// ----------------------------------------------------------------------------
