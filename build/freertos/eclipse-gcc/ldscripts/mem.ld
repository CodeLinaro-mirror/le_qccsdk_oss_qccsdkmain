/*
*/

/*
 * Memory Spaces Definitions.
 */

MEMORY
{

/************************************************************************************************
*						RRAM (current) Partitioning. 									        *
*	+------------+----------+----------------+--------------+--------------------------------+	*
*	| PBL Memory |	  FDT   |   File System	 |	 CAL DATA   |        System Image 			 |	*
*	|  16 KB	 |	 1 KB	|      32 KB     |		4 KB    |          1483 KB				 |	*
*	+------------+----------+----------------+--------------+--------------------------------+	*
*																						        *
************************************************************************************************/

  OTPMEM (rw) : ORIGIN = 0x00080000, LENGTH = 0x00000220
  PBLMEM (xrw) : ORIGIN = 0x00200000, LENGTH = 0x00004000
  FDTMEM (rw) : ORIGIN = 0x00204000, LENGTH = 0x00000400
  FSMEM (rw) : ORIGIN = 0x00204400, LENGTH = 0x00008000
  CALMEM (rw) : ORIGIN = 0x0020C400, LENGTH = 0x00001000
  RRAM_FLASH (xrw) : ORIGIN = 0x0020D400, LENGTH = 0x00172C00

  RAM (xrw) : ORIGIN = 0x00000000, LENGTH = 0x00080000
}

/*Ram Bank address and their size*/
_ln_cMEM_Bank_size__ = 0x00020000;
_ln_cMEM_Bank_A_start_addr__ = ORIGIN(RAM);
_ln_cMEM_Bank_B_start_addr__ = _ln_cMEM_Bank_A_start_addr__ + _ln_cMEM_Bank_size__;
_ln_cMEM_Bank_C_start_addr__ = _ln_cMEM_Bank_B_start_addr__ + _ln_cMEM_Bank_size__;
_ln_cMEM_Bank_D_start_addr__ = _ln_cMEM_Bank_C_start_addr__ + _ln_cMEM_Bank_size__;

