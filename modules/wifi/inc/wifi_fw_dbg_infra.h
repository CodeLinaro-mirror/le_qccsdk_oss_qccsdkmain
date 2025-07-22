/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/******************************************************************************
 * @file    ferm_debug_infra.h
 * @brief   Declarations of Fermion Debug Infra
 *
 *
 *****************************************************************************/
#ifndef _FERM_DEBUG_INFRA_H_
#define _FERM_DEBUG_INFRA_H_

#include "fwconfig_cmn.h"
#include "nt_flags.h"
#include <stdlib.h>
#include <stdint.h>
#include "fifo.h"
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "semphr.h"
#include "nt_logger_api.h"
#include "nt_hw_support.h"
#include "wifi_fw_dbg_infra_cmn.h"

/*************************
 * Defines
 *************************/

#define FDI_RESET                     (0)
#define FDI_SET                       (1)
#define FDI_INS_ATTR_TICK_TYPE_OFFSET (14)
#define FDI_NODE_ATTR_EN_BIT_MASK     (FDI_SET << FDI_NODE_ATTR_ENABLE_OFFSET)
#define FDI_NODE_ATTR_ENABLE_OFFSET   (15)
#define FDI_NODE_ATTR_LOG_LVL_OFFSET  (14)
#define FDI_NODE_ATTR_ID_MASK         (0x3FFF)

#define FDI_FIFO_MAX_WATERMARK (95)

#if FDI_EN_POST_PROCESS == FDI_SET
#define FDI_FIFO_DEPTH         (FDI_MAX_LOG / 2)
#define FDI_FIFO_DEF_WATERMARK (80)
/* FDI Thread Defines */
#define FDI_THREAD_STACK_SIZE (255)
#define FDI_THREAD_NAME       "FDI_TH"
#define FDI_THREAD_PRIOR      (1U)
#define FDI_WM_EVT_BIT_MASK   (FDI_SET << 5)

#else
#define FDI_FIFO_DEF_WATERMARK FDI_FIFO_MAX_WATERMARK
#define FDI_FIFO_DEPTH         (FDI_MAX_LOG)
#endif /* FDI_EN_POST_PROCESS */

#if FDI_EN_POST_PROCESS == FDI_SET
#if FDI_PRINT_ON_UT == FDI_RESET
#undef FDI_PRINT_ON_UT
#define FDI_PRINT_ON_UT FDI_SET
#endif
#endif

#define FDI_LOG_FIFO                (g_fdi.buffer_ptr_active)
#define FDI_FIFO_ABS_WATERMARK(_wm) (size_t)((FDI_FIFO_DEPTH * _wm) / 100)
#define FDI_NODE_ID(_attr)          (_attr & FDI_NODE_ATTR_ID_MASK)

#ifdef SUPPORT_HIGH_RES_TIMER
#include "timer.h"
#include "timer_internal.h"
extern int8_t timer_cvt_from_tick64(uint64_t time, time_unit_type unit, uint64_t *pTimeRet);
#endif

#ifdef FEATURE_FDI

#define FDI_INSERT_START(_node, __identifier) fdi_insert_log(_node, TICK_TYPE_START_TICK, __identifier)
#define FDI_INSERT_STOP(_node, __identifier)  fdi_insert_log(_node, TICK_TYPE_STOP_TICK, __identifier)

#define FDI_INSERT_START_NULL(_node) FDI_INSERT_START(_node, (uint32_t)NULL)
#define FDI_INSERT_STOP_NULL(_node)  FDI_INSERT_STOP(_node, (uint32_t)NULL)

#define FDI_NODE_START_NULL(_node)            FDI_INSERT_START_NULL(g_ferm_dbg_nodes[_node])
#define FDI_NODE_STOP_NULL(_node)             FDI_INSERT_STOP_NULL(g_ferm_dbg_nodes[_node])
#define FDI_NODE_START_ID(_node, _identifier) FDI_INSERT_START(g_ferm_dbg_nodes[_node], _identifier)
#define FDI_NODE_STOP_ID(_node, _identifier)  FDI_INSERT_STOP(g_ferm_dbg_nodes[_node], _identifier)

#define FDI_NODE2LOG(_node, _ins) fdi_node2log(g_ferm_dbg_nodes[_node], _ins)

#else

#define FDI_NODE_START_NULL(_node)
#define FDI_NODE_STOP_NULL(_node)
#define FDI_NODE_START_ID(_node, _identifier)
#define FDI_NODE_STOP_ID(_node, _identifier)

#define FDI_NODE2LOG(_node, _ins)

#endif /* FEATURE_FDI */

#define FDI_ASSERT_IF_FALSE(cond, err) \
    if (!(cond))                       \
    return err

typedef enum {
    FDI_RET_SUCCESS,
    FDI_RET_FAILED_NODE_MAX,
    FDI_RET_FAILED_ASSERT_PARAM,
    FDI_RET_FAILED_ASSERT_MOD_NOT_ENABLE,
    FDI_RET_FAILED_ASSERT_NODE_NOT_ENABLE,
    FDI_RET_FAILED_ASSERT_PROD_NOT_ENABLE,
    FDI_RET_FAILED_ASSERT_SEMAPHORE_GET,
    FDI_RET_FAILED_RMC_CONTEXT,
} fdi_ret_t;

typedef enum { TICK_TYPE_START_TICK, TICK_TYPE_STOP_TICK } tick_type_t;

typedef enum { LOG_LEVEL_DEBUG = 0x00, LOG_LEVEL_PROD = (1 << FDI_NODE_ATTR_LOG_LVL_OFFSET) } log_level_t;

typedef struct fdi_reg fdi_reg_t;
typedef fdi_reg_t *fdi_interface_t;
typedef struct fdi_node_ins fdi_node_ins_t;

typedef void (*post_ins_cb)(fdi_node_ins_t *p_fdi_ins);

/* Structure to define node attributes */
typedef struct attr {
    uint8_t log_level : 1; /* Log Level according to log_level_t */
    uint8_t Enable : 1;    /* Node default Enable/Disable */
    uint32_t module_bmap;  /* Module Enable bitmap field */
} attr_t;

/* Sructure for registration into node list */
typedef struct fdi_nodes {
    uint16_t id;       /* Node ID or Enum */
    post_ins_cb p_cb;  /* Post insertion callback */
    attr_t attributes; /* Node default attributes */
} fdi_nodes_t;

/* Structure for Node Registration */
struct fdi_reg {
    uint16_t attr;        /* 15 << Enable | 14 << Log_Level | id & 0x3FFF*/
    uint32_t module_bmap; /* Module Enable bitmap field */
    post_ins_cb p_cb;     /* Callback post log insertion */
#if FDI_EN_POST_PROCESS
    uint32_t evt_start; /* last Event Start Tick */
    uint32_t event_ctr; /* Number of times the Event Occured */
    uint32_t tick_last; /* Last Recorded Tick diff */
    uint32_t tick_peak; /* Peak Recorded Tick diff */
    uint32_t tick_avrg; /* Average Recorded Tick diff */
    uint32_t tick_min;  /* Minimum Recorded Tick diff */
#endif
};

/* Structure as an Event insertion to Log Buffer */
struct fdi_node_ins {
    uint32_t sq_no;      /* Event sequence number */
    uint16_t attr;       /* 14 << tick_type | id & 0x3FFF */
    uint32_t ticks;      /* Tick stamp captured */
    uint32_t identifier; /* Place holder for node identifier */
};

/* FDI Control Instance */
typedef struct fdi_instance {
    uint8_t open;    /* Flag to tell whether the instance is initialised */
    uint32_t sq_ctr; /* Global sequence counter */
    void *p_queue;   /* Pointer to the queue/buffer being used for pushing logs */
} fdi_instance_t;

typedef struct fdi {
    uint32_t module_bmap_en;     /* Store the value of bitmaps enabled */
    fifo_t *buffer_ptr_active;   /* Pointer to the active FIFO */
    fdi_interface_t p_reg_table; /* Pointer to the node registration table */
#if FDI_EN_POST_PROCESS == FDI_SET
    size_t watermark;            /* Value of the FIFO watermarks */
    fifo_t *buffer_ptr_inactive; /* Pointer to the inactive FIFO */
    fifo_t *buffer_ptr_idfs;     /* Pointer to the unprocessed START FIFO */
#endif
} fdi_t;

#ifdef FEATURE_FDI

extern fdi_interface_t g_ferm_dbg_nodes[FDI_DBG_MAX];

/******************************************************************************
 * User Function Declaration
 *
 *****************************************************************************/
void fdi_init(void);
fdi_ret_t fdi_print_log(uint8_t clear);
fdi_ret_t fdi_mod_en(uint32_t bmap, uint8_t en_flag);
fdi_ret_t fdi_node_en(uint16_t node_id, uint8_t en_flag);
fdi_ret_t fdi_reg_node(uint16_t id, attr_t attr, post_ins_cb p_cb, fdi_reg_t **p_ret);
fdi_ret_t fdi_insert_log(const fdi_reg_t *p_reg, tick_type_t tick_type, uint32_t identifier);
fdi_ret_t fdi_node2log(const fdi_reg_t *p_reg, fdi_node_ins_t *ins_node);
#if FDI_EN_POST_PROCESS == FDI_SET
fdi_ret_t fdi_trigger_pp(void);
fdi_ret_t fdi_set_wm(uint8_t wm_percent);
#endif
uint32_t fdi_get_time_stamp(void);

#endif
#endif /*_FERM_DEBUG_INFRA_H_*/
