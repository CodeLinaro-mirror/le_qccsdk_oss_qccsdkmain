/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#include <stdio.h>
#include <string.h>
#include "boot_elf_mem.h"
#include "boot_error_if.h"
#include "nt_bl_mem.h"
#include "nt_bl_rram_dxe.h"
#include "boot_print.h"
#include "pbl_patch_table.h"
#ifdef SBL_BUILD
#include "sbl_common.h"
#endif

#include "safeAPI.h"

#ifdef P_DEBUG
#define ELF_MEM_DUMP_DBG
#endif

#ifdef ELF_MEM_DUMP_DBG
void boot_elf_mem_dump(bl_elf_mem_ops *mem_ops, uint32_t dump_cfg);
#define ELF_MEM_DUMP(ops, cfg)	boot_elf_mem_dump(ops, cfg);
#else
#define ELF_MEM_DUMP(ops, cfg)
#endif

bl_elf_mem_op elf_mem_op[] = {
	{.type = BL_RRAM_RAM,
	 .region = {FRN_RAM_START, FRN_RAM_END, FRN_RRAM_START, FRN_RRAM_END},
	 .read = bl_elf_rram_read_to_ram,
	 .set = bl_elf_rram_set_ram,
	},
	{.type = BL_RRAM_RRAM, 
	 .region = {FRN_RRAM_START, FRN_RRAM_END, FRN_RRAM_START, FRN_RRAM_END},
	 .read = bl_elf_rram_read_to_rram_dxe,
	 .set = bl_elf_rram_set_rram_dxe, 
	},
#ifdef SBL_BUILD	 
	{.type = BL_FLASH_RAM,
	 .region = {FRN_RAM_START, FRN_RAM_END, FRN_FLASH_START, FRN_FLASH_END},
	 .read = bl_elf_flash_read_to_ram,
	 .set = NULL,
	 },
	{.type = BL_FLASH_RRAM, 
	 .region = {FRN_RRAM_START, FRN_RRAM_END, FRN_FLASH_START, FRN_FLASH_END},
	 .read = bl_elf_flash_read_to_rram_dxe,
	 .set = NULL,
	 },
#endif
};

uint32_t elf_mem_op_num = sizeof(elf_mem_op)/sizeof(bl_elf_mem_op); 

#ifdef SBL_BUILD
char rram_read_buf[RRAM_READ_BUF_SIZE] = {0};

bl_error_type bl_elf_flash_read_to_ram(void *address, void *data, uint32_t bytes)
{
	int32_t status = 0;	
	uint32_t done = 0;
	uint32_t last_bytes = 0;
	uint32_t nvm_add = (uint32_t)address - APP_IMAGE_FLASH_ADDRESS;
	uint8_t temp[RRAM_READ_BUF_SIZE]={0};
	
	//ELF_MEM_PRINTF("NVM->RAM add=%x, nvm=%x bytes=%x\r\n", (unsigned int)address, (unsigned int)nvm_add, (unsigned int)bytes);
	
	//ELF_MEM_PRINTF("flash read start bytes=%d, nvm=%d\r\n", (unsigned int)bytes, (unsigned int)nvm_add);
	if (bytes > RRAM_READ_BUF_SIZE) {
		while((done <= bytes) && (bytes-done>RRAM_READ_BUF_SIZE)) {
			status = boot_sbl_flash_read(nvm_add+done, RRAM_READ_BUF_SIZE, temp);
			
			if (status) {
				if((data+done)==NULL){
					ELF_MEM_PRINTF("null %x\r\n", (unsigned int)(data+done));
				}
				if((nvm_add+done+RRAM_READ_BUF_SIZE)>1536*1024){
					ELF_MEM_PRINTF("Exceed RRAM size %x\r\n",(unsigned int)(nvm_add+done+RRAM_READ_BUF_SIZE));
				}
				//ELF_MEM_PRINTF("flash read add=%x, dst=%x status=%d\r\n", (unsigned int)(nvm_add+done), (unsigned int)data,(int)status);
				return BL_ERR_ELF_MEM_READ;
			}

			/* Move flash read data to Dest CMem region */
			memscpy( data+done, RRAM_READ_BUF_SIZE, temp, RRAM_READ_BUF_SIZE );
			//ELF_MEM_PRINTF("CMEM=%x - [%x %x %x]\r\n", (unsigned int)(data+done), 
			//	*(unsigned int*)(data+done), *(unsigned int*)(data+done+1),*(unsigned int*)(data+done+2));
			done += RRAM_READ_BUF_SIZE;
		}

		/* Remaining bytes which is < RRAM_READ_BUF_SIZE*/
        last_bytes=bytes-done;

	} else {
		last_bytes = bytes;
	}

	if (last_bytes) {
		status = boot_sbl_flash_read(nvm_add+done, last_bytes, temp);

		if (status) {
			ELF_MEM_PRINTF("bytes=%u, status=%d\r\n", 
				(unsigned int)(last_bytes + done), (int)status);
			return BL_ERR_ELF_MEM_READ;
		}
		/* Move flash read data to Dest CMem region */
		memscpy( data+done, last_bytes, temp, last_bytes );
	}
	//ELF_MEM_PRINTF("flash read end bytes=%d\r\n", (unsigned int)bytes);

	return BL_ERR_NONE;
}

bl_error_type bl_elf_flash_read_to_rram_dxe(void *address, void *data, uint32_t bytes)
{
	int32_t status = 0;	
	uint32_t done = 0;
	uint32_t last_bytes = 0;
	uint32_t nvm_add = (uint32_t)address - APP_IMAGE_FLASH_ADDRESS;
	/*ELF_MEM_PRINTF("NVM->RRAM flash add=%x, nvm=%x, dst=%x, bytes=%x\r\n", (unsigned int)address, (unsigned int)nvm_add,
			(unsigned int)(data),(unsigned int)bytes); */

	if (bytes > RRAM_READ_BUF_SIZE) {
			while((done <= bytes) && (bytes-done>RRAM_READ_BUF_SIZE)) {
				memset((uint8_t *)(rram_read_buf), 0, sizeof(rram_read_buf));
				status = boot_sbl_flash_read(nvm_add+done, RRAM_READ_BUF_SIZE, (uint8_t*)rram_read_buf);
				
				if (status) {
					ELF_MEM_PRINTF("bytes=%u, status=%d\r\n", (unsigned int)done, (int)status);
					return BL_ERR_ELF_MEM_READ;
				}
				status = boot_sbl_rram_write((uint32_t)(data+done), (uint8_t*)rram_read_buf, RRAM_READ_BUF_SIZE);
				if (status) {
					ELF_MEM_PRINTF("SBL RRAM dxe err in write status=%d\r\n", (int)status);
				}
				
				done += RRAM_READ_BUF_SIZE;
			}
	
			/* Remaining bytes which is < RRAM_READ_BUF_SIZE*/
            last_bytes=bytes-done;

		} else {
			last_bytes = bytes;
		}
	
		if (last_bytes)
		{
			memset((uint8_t *)(rram_read_buf), 0, sizeof(rram_read_buf));
			status = boot_sbl_flash_read(nvm_add+done, last_bytes, (uint8_t*)rram_read_buf);
	
			if (status) {
				ELF_MEM_PRINTF("bytes=%u, status=%d\r\n", (unsigned int)(last_bytes + done), (int)status);
				return BL_ERR_ELF_MEM_READ;
			}

			status = boot_sbl_rram_write((uint32_t)(data+done), (uint8_t*)rram_read_buf, last_bytes);
			if (status) {
				ELF_MEM_PRINTF("SBL RRAM dxe err in write status=%d\r\n", (int)status);
			}
		}
	//ELF_MEM_PRINTF("flash dxe end bytes=%d\r\n", (unsigned int)bytes);

	return BL_ERR_NONE;
}

#endif
bl_error_type bl_elf_rram_set_rram_dxe(void *address, int32_t value, uint32_t bytes)
{
	bl_error_type status = BL_ERR_NONE;	

	status = PBL_IND_MOD_PFN(nt_bl_rram_dxe, rram_memset_dxe)((uint32_t)address, value, bytes);
			
	if (status) {
		ELF_MEM_PRINTF("Set to RRAM DXE err in writes bytes = %u, status = %u\r\n",
							(unsigned int)(bytes), (unsigned int)(status));
	}

	return status;
}

bl_error_type bl_elf_rram_read_to_rram_dxe(void *address, void *data, uint32_t bytes)
{
	bl_error_type status = BL_ERR_NONE;

	status = PBL_IND_MOD_PFN(nt_bl_rram_dxe, rram_write_dxe)((uint32_t)data, (uint8_t*)address, bytes);

	if (status) {
		ELF_MEM_PRINTF("Read to RRAM DXE err in write bytes = %u, status = %u\r\n",
							(unsigned int)(bytes), (unsigned int)status);
	}

	return status;

}

bl_error_type bl_elf_rram_set_ram(void *address, int32_t value , uint32_t bytes)
{
	bl_error_type status = BL_ERR_NONE;

	memset(address, value, bytes);

	return status;
}

bl_error_type bl_elf_rram_read_to_ram(void *address, void *data, uint32_t bytes)
{
	int32_t status = 0;	
	uint32_t done = 0;
	uint32_t last_bytes = 0;
	
	if (bytes > RRAM_READ_BUF_SIZE) {
		while(done <= bytes) {
			status = PBL_IND_MOD_PFN(nt_bl_mem, nt_bl_rram_read)(address + done, data + done, RRAM_READ_BUF_SIZE);
			
			if (status) {
				ELF_MEM_PRINTF("Read to RAM err in bytes=%u, status=%d\r\n",
							(unsigned int)done, (int)status);
				return BL_ERR_ELF_MEM_READ;
			}

			done += RRAM_READ_BUF_SIZE;
		}

		if (done > bytes) {
			last_bytes = RRAM_READ_BUF_SIZE - (done - bytes);
			done = done - RRAM_READ_BUF_SIZE;
		}
	} else {
		last_bytes = bytes;
	}

	if (last_bytes) {
		status = PBL_IND_MOD_PFN(nt_bl_mem, nt_bl_rram_read)(address + done , data + done, last_bytes);

		if (status) {
			ELF_MEM_PRINTF("Read to RAM err in bytes=%u, status=%d\r\n",
								(unsigned int)(last_bytes + done), (int)status);
			return BL_ERR_ELF_MEM_READ;
		}
	}
	
	return BL_ERR_NONE;
}

bl_error_type boot_elf_mem_read(void *address, void *data, uint32_t bytes, bl_elf_mem_ops *mem_ops)
{
	bl_error_type status = BL_ERR_NONE;
	bl_elf_mem_op *op;
	bl_elf_mem_region *region;
	uint32_t i = 0;

	if (mem_ops == NULL || mem_ops->num == 0)
		return BL_ERR_ELF_MEM_INVAL_PARAM;

	for (i = 0; i < mem_ops->num; i ++) {
		op = &mem_ops->op[i];
		region = &op->region;

		if (BL_ELF_MEM_READ_REGION_CHECK((uint32_t)address, (uint32_t)data, bytes,
						region->p_start, region->p_end, region->v_start, region->v_end)) {
			ELF_MEM_PRINTF("MEM read check succss:\r\n");
			ELF_MEM_PRINTF("src 0x%08lx  dst 0x%08lx\r\n",
						(uint32_t)address, (uint32_t)data);
			ELF_MEM_PRINTF("mem_type %d      bytes %d\r\n",
						(unsigned int)op->type, (unsigned int)bytes);

			status = op->read(address, data, bytes);
			
			break;
		}
	}
	
	return status;
}

bl_error_type boot_elf_mem_set(void *address, int32_t value, uint32_t bytes, bl_elf_mem_ops *mem_ops)
{
	bl_error_type status = BL_ERR_NONE;
	bl_elf_mem_op *op;
	bl_elf_mem_region *region;
	uint32_t i = 0;

	if (mem_ops == NULL || mem_ops->num == 0)
		return BL_ERR_ELF_MEM_INVAL_PARAM;

	for (i = 0; i < mem_ops->num; i ++) {
		op = &mem_ops->op[i];
		region = &op->region;

		if (BL_ELF_MEM_SET_REGION_CHECK((uint32_t)address, bytes,
						 region->v_start, region->v_end)) {
			ELF_MEM_PRINTF("MEM set check succss:\r\n");
			ELF_MEM_PRINTF("dst 0x%08lx	value %ld\r\n",
						(uint32_t)address, (int32_t)value);
			ELF_MEM_PRINTF("mem_type %d      bytes %d\r\n",
						(unsigned int)op->type, (unsigned int)bytes);

			if(op->set){
				status = op->set(address, value, bytes);
			}
			else{
				ELF_MEM_PRINTF("boot_elf Type=%d %08lx, %ld :\r\n", op->type, (uint32_t)address, (int32_t)bytes);
			}
			
			break;
		}

	}
	
	return status;
}


bl_error_type boot_elf_mem_op_register(bl_elf_mem_ops *mem_ops, bl_elf_mem_op mem_op)
{
	/* Validate pointers */
	if (mem_ops == NULL)
		return BL_ERR_ELF_MEM_NULL_PTR;
	
	if (mem_ops->num >= BL_MEM_MAX_NUM)
		return BL_ERR_ELF_MEM_INVAL_PARAM;

	mem_ops->op[mem_ops->num] = mem_op;

	mem_ops->num ++;

	return BL_ERR_NONE;
}

bl_error_type boot_elf_mem_op_init(bl_elf_mem_ops *mem_ops,
					bl_elf_mem_op *op_cfg, uint32_t op_cfg_num)
{
	bl_error_type status;
	uint32_t i = 0;

	if (mem_ops == NULL)
		return BL_ERR_ELF_MEM_NULL_PTR;
	
	memset((uint32_t *)(mem_ops), 0, sizeof(bl_elf_mem_ops));

	for (i = 0; i < op_cfg_num; i ++) {
		status = boot_elf_mem_op_register(mem_ops, op_cfg[i]);
		
		if (status != BL_ERR_NONE)
			return BL_ERR_ELF_MEM_INIT;
	}

	ELF_MEM_DUMP(mem_ops, BOOT_ELF_MEM_DUMP_ALL);

	return BL_ERR_NONE;
}

#ifdef ELF_MEM_DUMP_DBG
void boot_elf_mem_dump(bl_elf_mem_ops *mem_ops, uint32_t dump_cfg)
{
	uint32_t i;

	if (mem_ops == NULL)
		return;

	if (dump_cfg & BOOT_ELF_MEM_DUMP_OP){
		ELF_MEM_PRINTF("Dump ELF mem OP:\r\n");

		for (i = 0; i < mem_ops->num; i ++) {
			ELF_MEM_PRINTF("Dump ELF mem OP[%u]\r\n", (unsigned int)i);
			ELF_MEM_PRINTF("mem_read 0x%08x mem_type %u\r\n",
							(unsigned int)mem_ops->op[i].read, (unsigned int)mem_ops->op[i].type);
			ELF_MEM_PRINTF("v_start 0x%08x  v_stop 0x%08x\r\n",
							(unsigned int)mem_ops->op[i].region.v_start, (unsigned int)mem_ops->op[i].region.v_end);
			ELF_MEM_PRINTF("p_start 0x%08x  p_stop 0x%08x\r\n",
							(unsigned int)mem_ops->op[i].region.p_start, (unsigned int)mem_ops->op[i].region.p_end);

		}
	}

	return;
}
#endif
