/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#include <stdint.h>
#include "sbl_common.h"

#define MAX_FW_IMAGE_ENTRIES                        30

/* FWD_SIGNATURE (=0x46574454 "FWDT") */
#define FW_UPGRADE_MAGIC_V1                                0x54445746
#define APP_IMG_ID    10
#define SBL_IMG_ID    1
//#define MAX_FWD_NUM   3
/*Default 4KB Block, 256 bytes per page */
#define PAGES_PER_BLOCK  16 
#define PAGE_SIZE_IN_BYTES   256
#define BLOCK_SIZE_IN_BYTES  4096

/* sbl boot error bit*/
//#define SBL_FLASH_INIT_FAILURE      (1)
#define SBL_OTA_IMAGE_EXIST      (1)


#define FWD_TABLE_IS_VALID(type)  \
    (g_sbl_info.flash_fwd_table[type].signature==FW_UPGRADE_MAGIC_V1 && \
    g_sbl_info.flash_fwd_table[type].status == 1)

enum{
    INDEX_TRIAL,
    INDEX_CURRENT,
    INDEX_GOLDEN,
    MAX_FWD_NUM = 3,
};

enum{
    ERROR_SBL_NONE,
    ERROR_SBL_LOAD_INIT,
    ERROR_SBL_IMAGE_AUTH,
    ERROR_SBL_IMAGE_LOAD,
    ERROR_SBL_VERIFY_HASH,
};
/* FW Image Entry as described in the FW Descriptor Table of APP */
typedef struct {
    uint32_t  image_id;
    uint32_t  start_block;
    uint32_t  total_blocks;
    uint32_t  img_version;
} image_entry;

/* Firmware Descriptor Table in flash mainly for APP. 512 bytes in size */
typedef struct {
    uint32_t  signature;
    uint32_t  version;
    uint32_t  rank;
    uint8_t   status;
    uint8_t   total_images;
    uint8_t   reserved[18];
    image_entry image_entries[MAX_FW_IMAGE_ENTRIES];  
} fwd;

typedef struct {
    fwd flash_fwd_table[MAX_FWD_NUM];
    /* Indicates FWD position on flash, like 0, 1, 2
	 * boot_fwd[INDEX_TRIAL]=2 means trial image is at 3rd fwd on flash
	 */
    uint8_t boot_fwd[MAX_FWD_NUM];
    /* Indicates which FWD is valid, like trial, current, golden. value is 0 1 2 */
    uint8_t valid_fwd;
	uint8_t valid_fwd_mask;
    uint8_t load_type; 
    uint8_t ota_sbl_exist;
    uint8_t err_code;
} sbl_info_s;

bl_error_type boot_sbl_get_fwd_info();
bl_error_type boot_sbl_prepare_set_fde(fdt_entry *fde, uint32_t *addr, uint32_t index);
uint8_t boot_sbl_select_validFwd(sbl_info_s *info);
uint8_t boot_sbl_find_fwd_image(uint32_t fwd_idx, uint32_t img_id);
bl_error_type boot_sbl_update_fwd(uint8_t sector);
