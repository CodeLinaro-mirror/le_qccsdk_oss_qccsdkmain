/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef HFC_DEMO_H
#define HFC_DEMO_H
/*========================================================================
 * @brief Function definitions for hfc connections
 *=======================================================================*/

#ifdef CONFIG_QCSPI_HFC_TEST
typedef enum {
    HFC_TEST_DATA_START,
    HFC_TEST_DATA_END,
} hfc_ctrl_msg_id_t;

typedef enum {
    HFC_TEST_DEMO_DATA,
} hfc_data_msg_id_t;

typedef enum {
    HFC_DATA_TEST_TX,
    HFC_DATA_TEST_RX,
    HFC_DATA_TEST_LOOP_BACK,
} hfc_data_test_mode;

typedef struct {
    hfc_msg_hdr hdr;
    uint8_t mode;
    uint8_t reserved[3];
    uint32_t f2a_pkt_count;
    uint32_t f2a_pkt_size;
} hfc_data_test_ctrl;

typedef struct {
    hfc_msg_hdr hdr;
    uint8_t mode;
    uint8_t reserved[3];
    uint32_t f2a_pkt_count;
    uint32_t f2a_pkt_size;
    uint32_t recv_bytes;
    uint32_t send_bytes;
} hfc_data_test_stats;

/**
 * Initialize this qcspi hfc module:
 * - start the qcspi_hfc_thread
 */
void Initialize_qcspi_hfc_Demo(void);
int qcspi_hfc_send_test_end_event(void);
#endif
#endif /* HFC_DEMO_H */
