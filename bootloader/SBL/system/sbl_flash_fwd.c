/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "sbl_common.h"
#include "sbl_flash_fwd.h"
#include "safeAPI.h"

sbl_info_s g_sbl_info={0};

bl_error_type boot_sbl_get_fwd_info()
{
    uint32_t i_sector;
    fwd tempfwd;
    bl_error_type status=BL_ERR_INVALID_PARAM;
		
    memset(&(g_sbl_info.flash_fwd_table[0]),0, MAX_FWD_NUM*sizeof(fwd));
    memset(&tempfwd, 0, sizeof(fwd));
    
    for (i_sector = 0; i_sector < MAX_FWD_NUM; i_sector++) 
	{
        if(boot_sbl_flash_read((i_sector * BLOCK_SIZE_IN_BYTES), sizeof(fwd), (uint8_t *)&tempfwd) != BL_ERR_NONE)
      	{
      		sbl_printf("read flash fwd error\n\n");
      		return status;
      	}
        if (tempfwd.signature == FW_UPGRADE_MAGIC_V1 &&
            tempfwd.status == OTA_IMG_STATE_VALID) {
            	
            /******************************
			Valid FWD mask.
			Bit 0: Golden FWD. 0: invalid. 1: valid.
			Bit 1: Current FWD. 0: invalid, 1: valid.
			Bit 2: Trial FWD. 0: invalid. 1: valid.
			*******************************/
     
	        /* Global table has fixed order for 3 FWD table */        	
            if (tempfwd.rank == 0) { /* This is Golden flag from FWD on flash table */
                memscpy(&g_sbl_info.flash_fwd_table[INDEX_GOLDEN], sizeof(fwd), &tempfwd, sizeof(fwd));
				g_sbl_info.valid_fwd_mask |= 1;
                g_sbl_info.boot_fwd[INDEX_GOLDEN] = i_sector;
            } else if (tempfwd.rank == 0xFFFFFFFF) { /* This is Trial flag from FWD on flash table */
                g_sbl_info.valid_fwd_mask |= (1 << 2);
                memscpy(&g_sbl_info.flash_fwd_table[INDEX_TRIAL], sizeof(fwd), &tempfwd, sizeof(fwd));
                g_sbl_info.boot_fwd[INDEX_TRIAL] = i_sector;
            } else { /* This is other flag from FWD on flash table */
                g_sbl_info.valid_fwd_mask |= (1 << 1);
                memscpy(&g_sbl_info.flash_fwd_table[INDEX_CURRENT], sizeof(fwd), &tempfwd, sizeof(fwd));
                g_sbl_info.boot_fwd[INDEX_CURRENT] = i_sector;
            }
            //sbl_printf("flash table index=%x rank=%x\r\n", g_sbl_info.boot_fwd, tempfwd.rank);
            status = BL_ERR_NONE;
        }
        else{
        	  //sbl_printf("no fwd:index=%x sign=0x%x status=0x%x\r\n", i_sector, tempfwd.signature,tempfwd.status);
        	  //status = BL_ERR_INVALID_PARAM;
        }
    }
/*    
    sbl_printf("kkk fwd[0]: s:%x rank:%x sts:%x imgs:%x reserve:%x \r\n", g_sbl_info.flash_fwd_table[INDEX_TRIAL].signature,
    		g_sbl_info.flash_fwd_table[INDEX_TRIAL].rank,
    		g_sbl_info.flash_fwd_table[INDEX_TRIAL].status,
    		g_sbl_info.flash_fwd_table[INDEX_TRIAL].total_images,
    		g_sbl_info.flash_fwd_table[INDEX_TRIAL].reserved[0] );
    sbl_printf("kkk fwd[1]: s:%x rank:%x sts:%x imgs:%x reserve:%x \r\n", g_sbl_info.flash_fwd_table[INDEX_CURRENT].signature,
    		g_sbl_info.flash_fwd_table[INDEX_CURRENT].rank,
    		g_sbl_info.flash_fwd_table[INDEX_CURRENT].status,
    		g_sbl_info.flash_fwd_table[INDEX_CURRENT].total_images,
    		g_sbl_info.flash_fwd_table[INDEX_CURRENT].reserved[0] );
    sbl_printf("kkk fwd[2]: s:%x rank:%x sts:%x imgs:%x reserve:%x \r\n", g_sbl_info.flash_fwd_table[INDEX_GOLDEN].signature,
    		g_sbl_info.flash_fwd_table[INDEX_GOLDEN].rank,
    		g_sbl_info.flash_fwd_table[INDEX_GOLDEN].status,
    		g_sbl_info.flash_fwd_table[INDEX_GOLDEN].total_images,
    		g_sbl_info.flash_fwd_table[INDEX_GOLDEN].reserved[0] );    		    		
*/                
    return status;
}

bl_error_type boot_sbl_prepare_set_fde(fdt_entry *app_fde, uint32_t *app_load_addr, uint32_t index)
{
    uint32_t num=0;
	
    if (g_sbl_info.flash_fwd_table[index].signature == FW_UPGRADE_MAGIC_V1 &&
                    g_sbl_info.flash_fwd_table[index].status == OTA_IMG_STATE_VALID) 
    {
        if (!app_fde || !app_load_addr)
    	    return BL_ERR_NULL_PTR;
    			
        for (num = 0; num < MAX_FW_IMAGE_ENTRIES; num++) 
		{
            if (g_sbl_info.flash_fwd_table[index].image_entries[num].image_id == APP_IMG_ID) 
            {
                *app_load_addr = g_sbl_info.flash_fwd_table[index].image_entries[num].start_block*BLOCK_SIZE_IN_BYTES;
                if(app_fde->addr != g_sbl_info.boot_fwd[index])
                {
                    g_sbl_info.load_type = BOOT_ELF_LOAD_FULL;
                }
                else 
                {
                    g_sbl_info.load_type = BOOT_ELF_LOAD_RAM;
                }
                app_fde->addr = g_sbl_info.boot_fwd[index];
                //app_fde->rank = (index == 0) ? OTA_IMG_RANK_TRIAL : ((index==1)?OTA_IMG_RANK_CURRENT:OTA_IMG_RANK_GOLDEN);
                app_fde->state = OTA_IMG_STATE_VALID;
                

                switch (index)
                {
                	case INDEX_TRIAL:
                		app_fde->rank = OTA_IMG_RANK_TRIAL;
                		//app_fde->reserve[1] = 0x36;
                		break;
                	case INDEX_CURRENT:
                		app_fde->rank = OTA_IMG_RANK_CURRENT;
                		//app_fde->reserve[1] = 0x37;
                		break;
                	case INDEX_GOLDEN:
                		app_fde->rank = OTA_IMG_RANK_GOLDEN;
                		//app_fde->reserve[1] = 0x38;
                		break;	
                }

                //sbl_printf("SBL valid mask %x, %x\r\n", app_fde->reserve[0], index);
                break;
            }
            else 
            {
            	sbl_printf("not found APP image\r\n");
            }
        }
		app_fde->reserve[0] = g_sbl_info.valid_fwd_mask;
    }
    
    return BL_ERR_NONE;
}

uint8_t boot_sbl_find_fwd_image(uint32_t fwd_idx, uint32_t img_id)
{
    uint8_t num;
    for (num = 0; num < MAX_FW_IMAGE_ENTRIES; num++) {
        if (g_sbl_info.flash_fwd_table[fwd_idx].image_entries[num].image_id == img_id) 
        {
  	        return num;
        }
    }
    return MAX_FW_IMAGE_ENTRIES;
}

bl_error_type boot_sbl_update_fwd(uint8_t sector)
{
	fwd flash_fwd={0};
	if(boot_sbl_flash_read((sector * BLOCK_SIZE_IN_BYTES), sizeof(fwd), (uint8_t *)&flash_fwd) != BL_ERR_NONE)
	{
	    if(flash_fwd.status)
		{
		    flash_fwd.status = 0; //set invalid
			
	        if(boot_sbl_flash_write((sector * BLOCK_SIZE_IN_BYTES), sizeof(fwd), (uint8_t *)&flash_fwd)!= BL_ERR_NONE)
			{
			    sbl_printf("update fwd status error\r\n");
				return BL_ERR_INVALID_PARAM;
			}
		}
	}
	return BL_ERR_NONE;
}

