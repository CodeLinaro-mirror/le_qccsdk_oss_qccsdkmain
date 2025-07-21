#ifndef MGMT_FILTER_DEMO_H
#define MGMT_FILTER_DEMO_H
/*
#Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
#SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define MGMT_FRAME_MAX_NUM  10
#define MGMT_FRAME_MAX_SIZE 1024

#define MGMT_FILTER_MASK_START 0x01

typedef struct {
    uint8_t *data;
    uint32_t len;
} mgmt_frame_t;

extern uint32_t mgmt_frame_recv_enabled;
extern uint32_t mgmt_frame_recv_filter;
extern mgmt_frame_t mgmt_frame_recv_buf[MGMT_FRAME_MAX_NUM];
extern unsigned int mgmt_filter_start;
/**
 * Initialize this mgmt filter demo:
 * - start the mgmt_frame_recv_thread
 */
void Initialize_Mgmt_Filter_Demo(void);

#endif /* MGMT_FILTER_DEMO_H */
