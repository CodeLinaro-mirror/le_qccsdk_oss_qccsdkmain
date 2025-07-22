/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*
 * nt_lfs.h
 *
 *  Created on: Jul 16, 2020
 *      Author: nchakkar
 */

#ifndef CORE_SYSTEM_INC_NT_LFS_H_
#define CORE_SYSTEM_INC_NT_LFS_H_

#ifdef NT_FN_LFS

#include "lfs.h"
#include "stdio.h"

extern lfs_t lfs_init;

#define nt_uart_app_printf printf

#ifdef SUPPORT_FERMION_LOGGER
#define nt_uartprints_app(ptr, n) DBG_STR_PRINT("", ptr, n)
#endif  // SUPPORT_FERMION_LOGGER

int nt_lfs_init(void);
void nt_lfs_deinit(struct lfs_config deint);
/**
 * @FUNCTION : nt_lfs_pbl_logs ()
 * @brief    :
 *             This function will save PBL logs from cMEM bank A
 *             (location : 0x168) to File System.
 * @description :
 * 				In this function, PBL_log.txt file(size <= 1kb) is
 * 				created in File System. The first 4 bytes are resevered
 * 				for meta-data -> first 2 bytes for overlap index and
 * 				next 2 for sequencing index of the file. The sequencing
 * 				index is incremented every time when a pbl log is
 * 				saved in the file. it will increment till the file
 * 				size reaches the max size(~1kb). After that, sequencing
 * 				index is reset to 0(overlap happened) for next
 * 				iteration till again file reaches the max size.
 * 				The overlap index is incremented every time when
 * 				the file size reaches to the max size(~1kb).
 *
 * @param    :
 * 			   void
 * @return   :
 *             return 0 on success.
 *             return negative error code on failure
 */
int nt_lfs_pbl_logs(void);

uint32_t nt_find_filesize(const char *file);
int32_t nt_factory_reset(void);
#endif

#endif /* CORE_SYSTEM_INC_NT_LFS_H_ */
