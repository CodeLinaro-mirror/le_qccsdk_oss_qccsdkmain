/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================
 * @file data_ring_hdlr.h
 * @brief Data ring header parameters and function declarations
 * ======================================================================*/
#ifndef DATA_RING_HDLR_H
#define DATA_RING_HDLR_H
/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
#include "fwconfig_cmn.h"
#include "nt_flags.h"

#if defined(SUPPORT_RING_IF) || defined(SUPPORT_RING_IF_ONLY)
#include "ring_svc_api.h"
#include "data_svc_internal_api.h"

/*------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/
#define DATA_RING_BUFF_SIZE 1600

/* Keeping A2F elements to minimum to avoid allocating too many buffers */
/* in the beginning. anyway during refill more will be allocated */
#define MAX_NUM_A2F_DATA_RING_ELEMS 20
#define MAX_NUM_F2A_DATA_RING_ELEMS 32

#define TOTAL_NUM_DATA_RING_ELEMS    (MAX_NUM_A2F_DATA_RING_ELEMS + MAX_NUM_F2A_DATA_RING_ELEMS)
#define TOTAL_NUM_DATA_RING_ELEMS_AT MAX_NUM_A2F_DATA_RING_ELEMS

typedef ring_element_t a2f_data_ring_elem_t;
typedef ring_element_t f2a_data_ring_elem_t;

/* Enumeration for Data rings */
typedef enum {
    DATA_RING_UDP,
    DATA_RING_RAWETH,
    DATA_RING_TCP,
    DATA_RING_HFC,
    DATA_RING_MAX,
} DATA_RING_TYPE_ENUM;
/*------------------------------------------------------------------------
 * Function Declarations and Documentation
 * ----------------------------------------------------------------------*/
/* API to create the data ring */
void create_data_rings(void);

#endif /* SUPPORT_RING_IF || SUPPORT_RING_IF_ONLY */
#endif /* DATA_RING_HDLR_H */
