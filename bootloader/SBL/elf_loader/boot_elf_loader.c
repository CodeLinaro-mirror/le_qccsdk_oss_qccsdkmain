/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/
/*===========================================================================

                              ELF Image Loader

GENERAL DESCRIPTION
  This module performs generic ELF image loading.

===========================================================================*/

/*===========================================================================
                           EDIT HISTORY FOR FILE
  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

when       who        what, where, why
--------   ---        -------------------------------------------------------
07/04/23   bingzhe         Initial revision
===========================================================================*/

/*===========================================================================
                     INCLUDE FILES FOR MODULE
===========================================================================*/

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "boot_elf_loader.h"
#ifdef SBL_BUILD

#include "pbl_patch_table.h"
#include "boot_print.h"



#ifdef P_DEBUG
#define BOOT_ELF_LOADER_DBG
#endif

#ifdef BOOT_ELF_LOADER_DBG
void boot_elf_load_dump(boot_elf_loader *elf_loader, uint32_t dump_cfg);
#define ELF_LOADER_DUMP(loader, cfg)	boot_elf_load_dump(loader, cfg);
#else
#define ELF_LOADER_DUMP(loader, cfg)
#endif

boot_elf_loader elf_loader;

uint32_t pbl_running_check()
{
	uint32_t stack;

	__asm volatile ("MRS %0, msp" :"=r" (stack) : :"sp");

	//SYSTEM_PRINTF("Current stack 0x%08x\r\n", (unsigned int)stack);

	if (stack > PBL_STACK_ADDR)
		return 0;
	else
		return 1;
}

bl_error_type boot_elf_validate_elf_header(const boot_elf32_ehdr * elf_hdr)
{
	bl_error_type result = BL_ERR_NONE;

	if (NULL == elf_hdr)
		return BL_ERR_ELF_NULL_PTR;

    if (elf_hdr->e_ident[ELFINFO_MAG0_INDEX] != ELFINFO_MAG0
        || elf_hdr->e_ident[ELFINFO_MAG1_INDEX] != ELFINFO_MAG1
        || elf_hdr->e_ident[ELFINFO_MAG2_INDEX] != ELFINFO_MAG2
        || elf_hdr->e_ident[ELFINFO_MAG3_INDEX] != ELFINFO_MAG3)
		/* ELF file parse error in ELF loader */
		return BL_ERR_ELF_PARSE;

    if (elf_hdr->e_ident[ELFINFO_CLASS_INDEX] != BOOT_ELF_CLASS_32)
		return BL_ERR_ELF_CLASS_INVALID;

    /*Ensures size mentioned in ELF Header matches structure definition */
    if (elf_hdr->e_ehsize != sizeof(boot_elf32_ehdr))
		return BL_ERR_ELF_FORMAT;

    /*Ensures size mentioned in ELF Header matches structure definition */
    if (elf_hdr->e_phentsize != sizeof(boot_elf32_phdr))
		return BL_ERR_ELF_FORMAT;

	/*Ensures number of headers does not exceed Maximum allowed. */
	if(elf_hdr->e_phnum > MI_PBT_MAX_SEGMENTS)
		return BL_ERR_ELF_SEGMENT_COUNT;

	return result;
}

bl_error_type boot_elf_load_generic_segment(boot_elf_loader *elf_loader,
							  uint32_t prog_idx, uint32_t offset,
                              uint32_t dest_addr, uint32_t bytes_to_load)
{
	bl_error_type result = BL_ERR_NONE;
	uint32_t bytes_to_read = 0;
	uint32_t bytes_to_set = 0;
	boot_elf32_phdr *prog_hdr;

	if (elf_loader == NULL || elf_loader->elf_addr == NULL)
		return BL_ERR_ELF_NULL_PTR;

	if (prog_idx >= elf_loader->phr_num)
		return BL_ERR_ELF_INVAL_PARAM;

	prog_hdr = &elf_loader->prog_hdrs[prog_idx];

	/* Calculate the bytes_to_read.  This will be the bytes that need to be
	 read from the storage device. */
	if (offset >= prog_hdr->p_filesz)
		bytes_to_read = 0;
	else if (bytes_to_load >= prog_hdr->p_filesz - offset)
		bytes_to_read = prog_hdr->p_filesz - offset;
	else
		bytes_to_read = bytes_to_load;

	/* Calculate bytes_to_set.  This will be the amount of bytes to memset to 0
	 based on bytes_to_read, bytes_to_load, and memsz. */
	if (bytes_to_read >= bytes_to_load)
	/* Read of filesz consumed all of the requested bytes to load */
		bytes_to_set = 0;
	else if ((bytes_to_load - bytes_to_read) >=
		   (prog_hdr->p_memsz - prog_hdr->p_filesz))
	/* Remaining bytes to load is greater or equal to ZI region, so truncate. */
		bytes_to_set = prog_hdr->p_memsz - prog_hdr->p_filesz;
	else
	/* Remaining bytes to load is not as big as ZI region so set remaining bytes. */
		bytes_to_set = bytes_to_load - bytes_to_read;

	/* Only process segments with length greater than zero */
	/* 0x0 is a valid address.  Hence no need to validate dest_addr for null pointer */
	if (prog_hdr->p_memsz && prog_hdr->p_filesz &&
			(prog_hdr->p_memsz >= prog_hdr->p_filesz) && bytes_to_read) {
	/* Copy the data segment flash into memory */
		ELF_LOADER_PRINTF("Boot elf load segment read mem from %08lx to 0x%08lx\r\n",
						(uint32_t)(elf_loader->elf_addr) + prog_hdr->p_offset + offset, dest_addr);

		if (pbl_running_check())
			result = PBL_IND_MOD_PFN(boot_elf_mem, boot_elf_mem_read)((uint32_t *)((uint32_t)(elf_loader->elf_addr) + prog_hdr->p_offset + offset),
								(uint32_t *) dest_addr, bytes_to_read, &elf_loader->mem_ops);
		else
			result = boot_elf_mem_read((uint32_t *)((uint32_t)(elf_loader->elf_addr) + prog_hdr->p_offset + offset),
								(uint32_t *) dest_addr, bytes_to_read, &elf_loader->mem_ops);

		if (result != BL_ERR_NONE) {
			ELF_LOADER_PRINTF("Boot elf load segment read mem err %u\r\n", (unsigned int)result);
			return result;
		}
	}

	/* If uninitialized data exists, make sure to zero-init */
	if ((prog_hdr->p_memsz > prog_hdr->p_filesz) &&
			((dest_addr + prog_hdr->p_filesz) >= dest_addr) && bytes_to_set) {
		ELF_LOADER_PRINTF("Boot elf load segment set uninitialized mem [0x%08lx : 0x%08lx]\r\n",
						(dest_addr + bytes_to_read),(dest_addr + bytes_to_read + bytes_to_set));

		if (pbl_running_check())
			result = PBL_IND_MOD_PFN(boot_elf_mem, boot_elf_mem_set)((uint32_t *)(dest_addr + bytes_to_read), 0, bytes_to_set, &elf_loader->mem_ops);
		else
			result = boot_elf_mem_set((uint32_t *)(dest_addr + bytes_to_read), 0, bytes_to_set, &elf_loader->mem_ops);

		if (result != BL_ERR_NONE) {
			ELF_LOADER_PRINTF("Boot elf load segment set uninitialized mem err %u\r\n", (unsigned int)result);
			return result;
		}
	}

	return result;
}

bl_error_type boot_elf_load_elf_header(boot_elf_loader *elf_loader)
{
	bl_error_type result = BL_ERR_NONE;
	boot_elf32_ehdr *elf_header;

	/* Verify pointers are valid */
	if ((elf_loader == NULL) || (elf_loader->elf_addr == NULL))
		return BL_ERR_ELF_NULL_PTR;

	elf_header = &elf_loader->elf_hdr;

	/* Copy the ELF header into our local structure */
	result = PBL_IND_MOD_PFN(boot_elf_mem, boot_elf_mem_read)((void *)elf_loader->elf_addr, (void *)elf_header,
							sizeof(boot_elf32_ehdr), &elf_loader->mem_ops);

	if(result != BL_ERR_NONE)
		return result;

	/* Validate the ELF header*/
	result = PBL_IND_MOD_PFN(boot_elf_loader, boot_elf_validate_elf_header)(&elf_loader->elf_hdr);

	return result;
}

bl_error_type boot_elf_load_prog_hdrs(boot_elf_loader *elf_loader)
{
	bl_error_type result = BL_ERR_NONE;
	boot_elf32_ehdr *elf_header;

	/* Verify all pointers */
	if((elf_loader == NULL) ||
		(elf_loader->elf_addr == NULL))
		return BL_ERR_ELF_NULL_PTR;

	elf_header = &elf_loader->elf_hdr;

	/* Load the program headers */
	result = PBL_IND_MOD_PFN(boot_elf_mem, boot_elf_mem_read)((void *)((uint32_t)(elf_loader->elf_addr) + elf_header->e_phoff),
							(void *)elf_loader->prog_hdrs,
							(elf_header->e_phnum * elf_header->e_phentsize), &elf_loader->mem_ops);

	return result;
}

bl_error_type boot_elf_load_init(boot_elf_loader * elf_loader, uint32_t * elf_addr, bl_elf_mem_op *op_cfg, uint32_t op_cfg_num)
{
	bl_error_type result = BL_ERR_NONE;

	/* Validate pointers */
	if ((elf_loader == NULL) || (elf_addr == NULL))
		return BL_ERR_ELF_NULL_PTR;

	/* zi-init elf header */
	memset((uint32_t *)(&(elf_loader->elf_hdr)),
		 0,
		 sizeof(boot_elf32_ehdr));

	/* zi-init prog-header */
	memset((uint32_t *)(&(elf_loader->prog_hdrs)),
		 0,
		 (sizeof(boot_elf32_phdr) * MI_PBT_MAX_SEGMENTS));

	result = PBL_IND_MOD_PFN(boot_elf_mem, boot_elf_mem_op_init)(&elf_loader->mem_ops, op_cfg, op_cfg_num);

	if (result != BL_ERR_NONE) {
		ELF_LOADER_PRINTF("Boot elf load init mem op err=%d\r\n", result);
		return result;
	}

	elf_loader->elf_addr = elf_addr;

	result = PBL_IND_MOD_PFN(boot_elf_loader, boot_elf_load_elf_header)(elf_loader);

	if (result != BL_ERR_NONE) {
		ELF_LOADER_PRINTF("Boot elf load elf header err=%d\r\n", result);
		return result;
	}

	result = PBL_IND_MOD_PFN(boot_elf_loader, boot_elf_load_prog_hdrs)(elf_loader);

	if (result != BL_ERR_NONE) {
		ELF_LOADER_PRINTF("Boot elf load program header err=%d\r\n", result);
		return result;
	}

	elf_loader->phr_num = elf_loader->elf_hdr.e_phnum;

	ELF_LOADER_DUMP(elf_loader, BOOT_ELF_DUMP_ALL);

	return BL_ERR_NONE;
}

bl_error_type boot_elf_load_image(boot_elf_loader *elf_loader, boot_elf_load_type type)
{
	bl_error_type result = BL_ERR_NONE;
	uint32_t prog_idx = 0;
	boot_elf32_phdr *prog_hdr;

	if (elf_loader == NULL || elf_loader->elf_addr == NULL)
		return BL_ERR_ELF_NULL_PTR;

	prog_hdr = elf_loader->prog_hdrs;

	for (prog_idx = 0; prog_idx < elf_loader->phr_num; prog_idx ++) {
		if (prog_hdr->p_type == PT_LOAD) {

			if (type != BOOT_ELF_LOAD_COPY) {
					if (((type == BOOT_ELF_LOAD_RAM) && (prog_hdr->p_vaddr < FRN_RAM_END)) ||
									(type == BOOT_ELF_LOAD_FULL)) {
						if (pbl_running_check())
								result = PBL_IND_MOD_PFN(boot_elf_loader, boot_elf_load_generic_segment)(elf_loader, prog_idx, 0,
												prog_hdr->p_vaddr, prog_hdr->p_memsz);
						else
								result = boot_elf_load_generic_segment(elf_loader, prog_idx, 0,
												prog_hdr->p_vaddr, prog_hdr->p_memsz);
					}

			} else {

				if (pbl_running_check())
					result = PBL_IND_MOD_PFN(boot_elf_loader, boot_elf_load_generic_segment)(elf_loader, prog_idx, 0,
								prog_hdr->p_paddr, prog_hdr->p_filesz);
				else
					result = boot_elf_load_generic_segment(elf_loader, prog_idx, 0,
								prog_hdr->p_paddr, prog_hdr->p_filesz);

			}

			if (result != BL_ERR_NONE) {
					ELF_LOADER_PRINTF("Load segment err index = %u\r\n", (unsigned int)prog_idx);
				return result;
			}
		}

		prog_hdr ++;
	}

	return result;
}

#ifdef BOOT_ELF_LOADER_DBG
void boot_elf_load_dump(boot_elf_loader *elf_loader, uint32_t dump_cfg)
{
	boot_elf32_ehdr *elf_hdr;
	boot_elf32_phdr *prog_hdr;
	uint32_t i = 0;

	if (elf_loader == NULL)
		return;

	if (dump_cfg & BOOT_ELF_DUMP_ELF_HDR)
	{
		elf_hdr = &elf_loader->elf_hdr;

		ELF_LOADER_PRINTF("Dump elf header\r\n");
		ELF_LOADER_PRINTF("e_type 0x%08x\r\n", (unsigned int)elf_hdr->e_type);
		ELF_LOADER_PRINTF("e_flag 0x%08x\r\n", (unsigned int)elf_hdr->e_flags);
		ELF_LOADER_PRINTF("e_entry 0x%08x\r\n", (unsigned int)elf_hdr->e_entry);
	}

	if (dump_cfg & BOOT_ELF_DUMP_ELF_HDR) {
		prog_hdr = elf_loader->prog_hdrs;
		for (i = 0; i < elf_hdr->e_phnum; i ++, prog_hdr ++) {
			ELF_LOADER_PRINTF("Dump prog header[%d]\r\n", (int)i);
			ELF_LOADER_PRINTF("p_type   0x%08x   p_offset 0x%08x\r\n",
						(unsigned int)prog_hdr->p_type, (unsigned int)prog_hdr->p_offset);
			ELF_LOADER_PRINTF("p_vaddr  0x%08x   p_paddr  0x%08x\r\n",
						(unsigned int)prog_hdr->p_vaddr, (unsigned int)prog_hdr->p_paddr);
			ELF_LOADER_PRINTF("p_filesz 0x%08x   p_memsz  0x%08x\r\n",
						(unsigned int)prog_hdr->p_filesz, (unsigned int)prog_hdr->p_memsz);
			ELF_LOADER_PRINTF("p_flags  0x%08x   p_align  0x%08x\r\n",
						(unsigned int)prog_hdr->p_flags, (unsigned int)prog_hdr->p_align);
		}
	}
}
#endif

#endif
