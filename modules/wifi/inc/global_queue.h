/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/
//#ifdef FEATURE_TX_COMPLETE
#ifndef GLOBAL_QUEUE_H
#define GLOBAL_QUEUE_H

#include "fwconfig_wlan.h"
#include "nt_flags.h"
#include "hal_int_modules.h"
#include "nt_timer.h"
#include "timers.h"
#include "nt_osal.h"
#include "task.h"
#include <assert.h>

#ifdef FEATURE_TX_COMPLETE

#define MAX_TXCOMPQ_SIZE  32
#define DIALOG_TOKEN_SIZE 4

typedef struct tx_comp_feedback_q_attributes {
    uint32_t dialog_token;
    /* Time down count associated with frame */
    uint32_t time_to_expire;
    /* Timeout value */
    uint16_t time_out;
    /* Retry count */
    uint8_t retry_count;
    /* Software retry is required */
    boolean retry;
    boolean valid;

    /*function pointer to callback function of source*/
    pfn_callback src_callback;
    /*tbd: params of the callback function*/
    void *params;
    /*thread id of the thread sending packet*/
    TaskHandle_t thread_id;
} tx_comp_feedback_q_attributes_t;

typedef struct tx_comp_feedback_q {
    /* Array to store the meta data of the frames getting transmitted */
    tx_comp_feedback_q_attributes_t attribute_array[MAX_TXCOMPQ_SIZE];
    /* First time Q full detected */
    uint64_t first_tx_comp_feedback_q_full_time;
    /* Number of time Q full detected */
    uint32_t tx_comp_feedback_q_full_count;
    /* Active entry in reliablequeue */
    uint8_t active_count;
    /* One mili sec timer */
    nt_osal_timer_handle_t tx_comp_feedback_q_timer;
} tx_comp_feedback_q_t;

extern tx_comp_feedback_q_t g_tx_comp_feedback_q;
void tx_complete_timer_handler();

void process_wq_frames(void);
void init_global_queue();
void deinit_global_queue();

void tx_comp_frame_register(uint16_t time_out, boolean retry, pfn_callback src_callback, void *params);
void tx_comp_src_callback(eTxFrmWithTxComplStatus tx_status, TaskHandle_t thread_id, pfn_callback src_callback,
                          void *params);

#endif  // FEATURE_TX_COMPLETE

#endif  // GLOBAL_QUEUE_H
