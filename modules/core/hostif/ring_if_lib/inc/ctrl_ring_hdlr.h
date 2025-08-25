/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================

* @file ctrl_ring_hdlr.h
* @brief Control ring header parameters and function declarations
* ======================================================================*/
#ifndef CTRL_RING_HDLR_H
#define CTRL_RING_HDLR_H
/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
#include "fwconfig_cmn.h"
#include "nt_flags.h"

#if defined(SUPPORT_RING_IF) || defined(SUPPORT_RING_IF_ONLY)
#include "ring_svc_api.h"

/*------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/
#define FTM_CTRL_RING_BUFF_SIZE         1568 /* 28[tlv2.0 header]+1536[payload]+4[fermion wifi header] */
#define CTRL_RING_BUFF_SIZE             256
#define MAX_NUM_A2F_CTRL_RING_ELEMS     16
#define MAX_NUM_F2A_CTRL_RING_ELEMS     16
#define MAX_NUM_A2F_FTM_CTRL_RING_ELEMS 2
#define MAX_NUM_F2A_FTM_CTRL_RING_ELEMS 2
#define RING_IF_QUEUE_SIZE              MAX_NUM_F2A_CTRL_RING_ELEMS
#define RING_IF_TASK_STACK_SIZE         400 /* Same as Data_path task stack size. */
#define RING_IF_TASK_PRIORITY           6   /* Same as Data_path task priority. */

typedef ring_element_t a2f_ctrl_ring_elem_t;
typedef ring_element_t f2a_ctrl_ring_elem_t;

/*------------------------------------------------------------------------
 * Function Declarations and Documentation
 * ----------------------------------------------------------------------*/
/* API to be registered to the config ring */
void create_config_rings(void);

/* APIs of the below layers used by Config Ring */
extern bool process_wifi_config_pkt(uint32_t *p_buf, uint16_t len);
#endif /* SUPPORT_RING_IF */
#endif /* CTRL_RING_HDLR_H */
