/*========================================================================
* * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* * SPDX-License-Identifier: BSD-3-Clause-Clear

* @file fdt.h
* @brief The fw descriptor table which tells the high level info regarding\
* all the FW in Fermion.
*========================================================================*/
#ifndef _FDT_H_
#define _FDT_H_

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "nt_bl_uart.h"


typedef enum fdt_status
{
	FDT_FAILURE,
	FDT_SUCCESS,
}fdt_status_t;

typedef enum dfu_status
{
	DFU_STARTED_FROM_PBL = 1, /* To indicate the tiny loader started the DFU */
	DFU_STARTED_FROM_SBL,     /* To indicate the SBL started the DFU */
	DFU_FINISHED,             /* To indicate the DFU has finished */
	DFU_VALIDATED,            /* To indicate the resource has validated using the hash */
	DFU_STATUS_INVALID,       /* To indicate the resource is not valid */
}dfu_status_t;

typedef struct resource_desc
{
	uint32_t version;         /* Version of the resource */
	uint32_t hash[8];         /* #256 of the image */
	uint32_t reserved1;       /* reserved */
	uint32_t img_length;      /* image length */
	uint32_t dfu_status;      /* status of DFU */
	uint32_t reserved2[4];    /* reserved */
}resource_desc_t;

typedef struct sys_img_desc
{
	uint32_t version;         /* Version of the resource */
	uint32_t hash[8];         /* #256 of the image */
	uint32_t reserved1;       /* reserved */
	uint32_t img_length;      /* image length */
	uint32_t dfu_status;      /* status of DFU */
	uint32_t wlan_mode;       /* wlan img mode, factory or mission mode */
	uint32_t reserved2[3];    /* reserved */
}sys_img_desc_t;

typedef struct misc_desc
{
	uint32_t sbl_index;       /* SBL A/B index which is in use, this is not used curretly*/
	uint32_t reserved1[31];   /* reserved */
}misc_desc_t;

typedef struct  fw_desc_tbl
{
	resource_desc_t sbl_a;    /* SBL A descriptor */
	resource_desc_t sbl_b;    /* SBL B descriptor, which is not used currently*/
	resource_desc_t bdf;      /* BDF descriptor */
	resource_desc_t reg_db;   /* REG DB descriptor */
	sys_img_desc_t wlan_img;  /* Wlan IMG desctriptor */
	resource_desc_t wlan_ini; /* Reserved */
	misc_desc_t misc;         /* miscellaneous items */
	uint32_t reserved2[128];  /* reserved */
}fw_desc_tbl_t;
#endif /* _FDT_H_ */
