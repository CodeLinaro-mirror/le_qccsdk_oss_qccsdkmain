/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/

/*
 * nt_env.h
 *
 *  Created on: Jun 8, 2020
 *      Author: Acer
 */


#ifndef NT_BL_ENV_H_
#define NT_BL_ENV_H_

#include <stdint.h>

#define NT_MAX_SECTIONS 14

typedef struct __attribute__((packed)) _PBL_IMG_DESC_{
	uint8_t flag;
	uint8_t img_type;
	uint16_t pad0;
	uint32_t img_start_add;
	uint32_t img_size;
	uint32_t e_entry;
}pbl_img_desc_t;

typedef struct __attribute__((packed)) _ELF_PRG_HEAD_{
	uint32_t p_type;
	uint32_t p_offset;
	uint32_t p_vaddr;
	uint32_t p_paddr;
	uint32_t p_filesz;
	uint32_t p_memsz;
	uint32_t p_flags;
	uint32_t p_align;
} Elf32_Phdr;

typedef struct __attribute__((packed)) _PBL_FW_DESC_{
	uint32_t p_index_size;
	uint32_t chksum[4];             // Name more intuitive than magic number.
	uint32_t major_ver;
	uint32_t minor_ver;
	uint32_t flags;
	uint32_t num_images;
	uint32_t pad0[2];
	pbl_img_desc_t img_desc; //16
	Elf32_Phdr img_ph[NT_MAX_SECTIONS];  // Pad to 388 bytes.
} pbl_fw_desc_t;

typedef enum _IMG_UPD_MODE_{
	JTAG_DOWNLOAD = 0,
	UART_DOWNLOAD,
	OTA_UPDATE,

	MAX_UPD_MODE,
}
nt_img_upd_mode;

/*
 * @FUNCTION   :nt_bl_default_offset
 * @DESCRIPTION: default start address.
 * @PARA   : none
 * @RETURN : none
 *
 */

void nt_bl_default_offset(void);
void nt_bl_section_header_table(uint8_t buf[],uint16_t length);

#if defined(SUPPORT_PBL_PATCH)
typedef void (*nt_bl_default_offset_t)(void);
typedef void (*nt_bl_section_header_table_t)(uint8_t*, uint16_t);

typedef struct nt_bl_env_s{
    nt_bl_default_offset_t nt_bl_default_offset_pfn;
    nt_bl_section_header_table_t nt_bl_section_header_table_pfn;
}nt_bl_env_t;
#endif /*SUPPORT_PBL_PATCH*/
#endif /* NT_BL_ENV_H_ */
