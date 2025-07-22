/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _HAL_INT_TEMPLATE_H_
#define _HAL_INT_TEMPLATE_H_

#include <stdint.h>

#include "hal_int_mmap.h"

/* size of the template body */
#define HAL_TMPL_PS_POLL_BODY_SZ   sizeof(nt_hal_ps_poll_tmpl_body_t)
#define HAL_TMPL_QOS_NULL_BODY_SZ  sizeof(nt_hal_qos_null_tmpl_body_t)
#define HAL_TMPL_DATA_NULL_BODY_SZ sizeof(nt_hal_data_null_tmpl_body_t)
#define HAL_TMPL_CTS_BODY_SZ       sizeof(nt_hal_cts_tmpl_body_t)

/* protocal version */
#define HAL_MAC_PROTOCAL_VERSION 0

/* type and subtype of frames */
#define HAL_MAC_CTR_FRAM  0X1
#define HAL_MAC_DATA_FRAM 0X2

#define HAL_MAC_CTR_CTS       0XC
#define HAL_MAC_CTR_PS_POLL   0XA
#define HAL_MAC_DATA_NULL     0X4
#define HAL_MAC_DATA_QOS_NULL 0XC
#define HAL_MAC_CTR_ACK       0xD

/* @brief: frame header of cts_to_self, qos_null, ps_poll, data_null */
typedef struct nt_hal_tmpl_hdr_s {
#ifndef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t txpriority : 4;
    uint32_t ba_bcn : 1;
    uint32_t sb : 1;
    uint32_t psp : 1;
    uint32_t template_len : 12;
    uint32_t expected_resp_sub_type : 4;
    uint32_t expected_resp_type : 2;
    uint32_t resp_is_expected : 1;
    uint32_t template_sub_type : 4;
    uint32_t template_type : 2;
#else
    uint32_t template_type : 2;
    uint32_t template_sub_type : 4;
    uint32_t resp_is_expected : 1;
    uint32_t expected_resp_type : 2;
    uint32_t expected_resp_sub_type : 4;
    uint32_t template_len : 12;
    uint32_t psp : 1;
    uint32_t sb : 1;
    uint32_t ba_bcn : 1;
    uint32_t txpriority : 4;
#endif

#ifndef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t ftm : 1;
    uint32_t bwen : 1;
    uint32_t dm : 1;
    uint32_t staid : 8;
    uint32_t reserved3 : 1;
    uint32_t tx_power : 5;
    uint32_t lpe : 1;
    uint32_t min_tx_power : 5;
    uint32_t primary_data_rate_index : 9;
#else
    uint32_t primary_data_rate_index : 9;
    uint32_t min_tx_power : 5;
    uint32_t lpe : 1;
    uint32_t tx_power : 5;
    uint32_t reserved3 : 1;
    uint32_t staid : 8;
    uint32_t dm : 1;
    uint32_t bwen : 1;
    uint32_t ftm : 1;
#endif

#ifndef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t reserved4 : 16;
    uint32_t tsf_offset : 16;
#else
    uint32_t tsf_offset : 16;
    uint32_t reserved4 : 16;
#endif

#ifndef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t template_lsig : 16;
    uint32_t template_duration : 16;
#else
    uint32_t template_duration : 16;
    uint32_t template_lsig : 16;
#endif

    uint32_t tsf_lower;
    uint32_t tsf_upper;

} __attribute__((packed)) __attribute__((aligned(4))) nt_hal_tmpl_hdr_t;

/* @brief: frame body of  ps poll */
typedef struct nt_hal_ps_poll_tmpl_body_s {
#ifndef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t protocol : 2;
    uint32_t type : 2;
    uint32_t subtype : 4;
    uint32_t tods : 1;
    uint32_t fromds : 1;
    uint32_t morefrag : 1;
    uint32_t retry : 1;
    uint32_t pm : 1;
    uint32_t moredata : 1;
    uint32_t protected_frame : 1;
    uint32_t order : 1;
    uint32_t aid : 16;
#else
    uint32_t aid : 16;

    uint32_t tods : 1;
    uint32_t fromds : 1;
    uint32_t morefrag : 1;
    uint32_t retry : 1;
    uint32_t pm : 1;
    uint32_t moredata : 1;
    uint32_t protected_frame : 1;
    uint32_t order : 1;

    uint32_t protocol : 2;
    uint32_t type : 2;
    uint32_t subtype : 4;
#endif

#ifndef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t bssidLo1 : 16;
    uint32_t bssidLo2 : 16;
#else
    uint32_t bssidLo2 : 16;
    uint32_t bssidLo1 : 16;
#endif

#ifndef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t bssidHi : 16;
    uint32_t taLo : 16;
#else
    uint32_t taLo : 16;
    uint32_t bssidHi : 16;
#endif

#ifndef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t taHi1 : 16;
    uint32_t taHi2 : 16;
#else
    uint32_t taHi2 : 16;
    uint32_t taHi1 : 16;
#endif

    uint32_t crc;
} __attribute__((packed)) __attribute__((aligned(4))) nt_hal_ps_poll_tmpl_body_t;

/* @brief: frame header and frame body*/
typedef struct nt_hal_ps_poll_tmpl_s {
    nt_hal_tmpl_hdr_t ps_hdr;
    nt_hal_ps_poll_tmpl_body_t ps_body;

} __attribute__((packed)) __attribute__((aligned(4))) nt_hal_ps_poll_tmpl_t;

/* @brief: frame body of qos null */
typedef struct nt_hal_qos_null_tmpl_body_s {
#ifndef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t protocol : 2;
    uint32_t type : 2;
    uint32_t subtype : 4;
    uint32_t tods : 1;
    uint32_t fromds : 1;
    uint32_t morefrag : 1;
    uint32_t retry : 1;
    uint32_t pm : 1;
    uint32_t moredata : 1;
    uint32_t protected_frame : 1;
    uint32_t order : 1;
    uint32_t duration : 16;
#else
    uint32_t duration : 16;

    uint32_t tods : 1;
    uint32_t fromds : 1;
    uint32_t morefrag : 1;
    uint32_t retry : 1;
    uint32_t pm : 1;
    uint32_t moredata : 1;
    uint32_t protected_frame : 1;
    uint32_t order : 1;

    uint32_t protocol : 2;
    uint32_t type : 2;
    uint32_t subtype : 4;

#endif

#ifndef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t raLo1 : 16;
    uint32_t raLo2 : 16;
#else
    uint32_t raLo2 : 16;
    uint32_t raLo1 : 16;
#endif

#ifndef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t raHi : 16;
    uint32_t taLo : 16;
#else
    uint32_t taLo : 16;
    uint32_t raHi : 16;
#endif

#ifndef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t taHi1 : 16;
    uint32_t taHi2 : 16;
#else
    uint32_t taHi2 : 16;
    uint32_t taHi1 : 16;
#endif

#ifndef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t bssidLo1 : 16;
    uint32_t bssidLo2 : 16;
#else
    uint32_t bssidLo2 : 16;
    uint32_t bssidLo1 : 16;
#endif

#ifndef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t bssidHi : 16;
    uint32_t seqctrl : 16;
#else
    uint32_t seqctrl : 16;
    uint32_t bssidHi : 16;
#endif

#ifndef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t tid : 4;
    ;
    uint32_t eosp : 1;
    uint32_t ackpolicy : 2;
    uint32_t reserved1 : 1;
    uint32_t appsbufferstate : 8;
    uint32_t reserved2 : 16;
#else
    uint32_t reserved2 : 16;

    uint32_t appsbufferstate : 8;

    uint32_t tid : 4;
    uint32_t eosp : 1;
    uint32_t ackpolicy : 2;
    uint32_t reserved1 : 1;
#endif

    uint32_t crc;

} __attribute__((packed)) __attribute__((aligned(4))) nt_hal_qos_null_tmpl_body_t;

/* @brief: frame header and frame body*/
typedef struct nt_hal_qos_null_tmpl_s {
    nt_hal_tmpl_hdr_t qos_hdr;
    nt_hal_qos_null_tmpl_body_t qos_body;

} __attribute__((packed)) __attribute__((aligned(4))) nt_hal_qos_null_tmpl_t;

/* @brief: frame body of data null */
typedef struct nt_hal_data_null_tmpl_body_s {
#ifndef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t protocol : 2;
    uint32_t type : 2;
    uint32_t subtype : 4;
    uint32_t tods : 1;
    uint32_t fromds : 1;
    uint32_t morefrag : 1;
    uint32_t retry : 1;
    uint32_t pm : 1;
    uint32_t moredata : 1;
    uint32_t protected_frame : 1;
    uint32_t order : 1;
    uint32_t duration : 16;
#else
    uint32_t duration : 16;

    uint32_t tods : 1;
    uint32_t fromds : 1;
    uint32_t morefrag : 1;
    uint32_t retry : 1;
    uint32_t pm : 1;
    uint32_t moredata : 1;
    uint32_t protected_frame : 1;
    uint32_t order : 1;

    uint32_t protocol : 2;
    uint32_t type : 2;
    uint32_t subtype : 4;
#endif

#ifndef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t raLo1 : 16;
    uint32_t raLo2 : 16;
#else
    uint32_t raLo2 : 16;
    uint32_t raLo1 : 16;
#endif

#ifndef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t raHi : 16;
    uint32_t taLo : 16;
#else
    uint32_t taLo : 16;
    uint32_t raHi : 16;
#endif

#ifndef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t taHi1 : 16;
    uint32_t taHi2 : 16;
#else
    uint32_t taHi2 : 16;
    uint32_t taHi1 : 16;
#endif

#ifndef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t bssidLo1 : 16;
    uint32_t bssidLo2 : 16;
#else
    uint32_t bssidLo2 : 16;
    uint32_t bssidLo1 : 16;
#endif

#ifndef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t bssidHi : 16;
    uint32_t seqctrl : 16;
#else
    uint32_t seqctrl : 16;
    uint32_t bssidHi : 16;
#endif

    uint32_t crc;

} __attribute__((packed)) __attribute__((aligned(4))) nt_hal_data_null_tmpl_body_t;

/* @brief: frame header and frame body*/
typedef struct nt_hal_data_null_tmpl_s {
    nt_hal_tmpl_hdr_t data_hdr;
    nt_hal_data_null_tmpl_body_t data_body;

} __attribute__((packed)) __attribute__((aligned(4))) nt_hal_data_null_tmpl_t;

/* @brief: frame body of cts to self */
typedef struct nt_hal_cts_tmpl_body_s {
#ifndef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t subtype : 4;
    uint32_t type : 2;
    uint32_t protocol : 2;

    uint32_t order : 1;
    uint32_t protected_frame : 1;
    uint32_t moredata : 1;
    uint32_t pm : 1;
    uint32_t retry : 1;
    uint32_t morefrag : 1;
    uint32_t fromds : 1;
    uint32_t tods : 1;
    uint32_t duration : 16;
#else
    uint32_t duration : 16;

    uint32_t tods : 1;
    uint32_t fromds : 1;
    uint32_t morefrag : 1;
    uint32_t retry : 1;
    uint32_t pm : 1;
    uint32_t moredata : 1;
    uint32_t protected_frame : 1;
    uint32_t order : 1;

    uint32_t protocol : 2;
    uint32_t type : 2;
    uint32_t subtype : 4;
#endif

#ifndef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t raLo1 : 16;
    uint32_t raLo2 : 16;
#else
    uint32_t raLo2 : 16;
    uint32_t raLo1 : 16;
#endif

#ifndef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t raHi : 16;
    uint32_t reserved : 16;
#else
    uint32_t reserved : 16;
    uint32_t raHi : 16;
#endif

    uint32_t crc;

} __attribute__((packed)) __attribute__((aligned(4))) nt_hal_cts_tmpl_body_t;

/* @brief: frame header and frame body*/
typedef struct nt_hal_cts_tmpl_s {
    nt_hal_tmpl_hdr_t cts_hdr;
    nt_hal_cts_tmpl_body_t cts_body;

} __attribute__((packed)) __attribute__((aligned(4))) nt_hal_cts_tmpl_t;

typedef struct nt_hal_frame_tmpl_s {
    nt_hal_tmpl_hdr_t data_hdr;
    uint8_t template_data[256];

} __attribute__((packed)) __attribute__((aligned(4))) nt_hal_frame_tmpl_t;

/*
 * @brief: Initialize the software template for ps poll frame
 * @return NONE
 */
void nt_hal_ps_poll_init();

/*
 * @brief: Initialize the software template for qos null frame
 * @return NONE
 */
void nt_hal_qos_null_init(uint8_t tid);

/*
 * @brief: Initialize the software template for data null frame
 * @return NONE
 */
void nt_hal_data_null_init();

/*
 * @brief: Store the software template address and transmit the ps poll frame
 * @param parameter1: Destinationaddress
 * @param parameter2: Sourceaddress
 * @return NONE
 */
void nt_hal_ps_poll_frame_tx(uint8_t *destinationaddress, uint8_t *sourceaddress, uint8_t pm);

/*
 * @brief: Store the software template address and transmit the qos null frame
 * @param parameter1: Destinationaddress
 * @param parameter2: Sourceaddress
 * @param parameter3 : PM bit to set 0 or 1
 * @return NONE
 */
void nt_hal_qos_null_frame_tx(uint8_t *destinationaddress, uint8_t *sourceaddress, uint8_t pm, NT_BOOL ToDS);

/*
 * @brief: Store the software template address and transmit the data null frame
 * @param parameter1: Destinationaddress
 * @param parameter2: Sourceaddress
 * @param parameter3 : PM bit to set 0 or 1
 * @return NONE
 */
void nt_hal_data_null_frame_tx(uint8_t *destinationaddress, uint8_t *sourceaddress, uint8_t pm, NT_BOOL ToDS);

/*
 * @brief: Store the software template address and transmit the CTS to Self frame
 * @param parameter1: duration
 * @param parameter2: Destinationaddress
 * @return NONE
 */
void nt_hal_cts_to_self_frame_tx(uint16_t dur, uint8_t *destinationaddress);
void nt_hal_cts_to_self_init(void);
/*
 *  @brief : This function is used for sending all wur frame types frames
 *  @param parameter1: pointer to the  frame body
 *  @param parameter2: length frame body length
 *  @param parameter3: rate LDR - 0, HDR - 1
 *  @return : NT_OK
 */
nt_status_t nt_hal_wur_frame_send(void *frame, uint16_t length, uint8_t rate);

/*
 *  @brief : This function is used for sending all frame types frames
 *  @param parameter1: type - frame type
 *  @param parameter2: subtype - frame subtype
 *  @param parameter3: pointer to the  frame body
 *  @param parameter4: length frame body length
 *  @return : NT_OK
 */
nt_status_t nt_hal_frame_send(uint8_t type, uint8_t sub_type, void *frame, uint16_t length);
nt_status_t hal_frame_send(uint8_t type, uint8_t sub_type, uint16_t phyrate, void *frame, uint16_t length,
                           uint16_t pkt_flags, uint8_t tx_priority, uint8_t min_tx_pwr);

void hal_data_null_init(uint16_t rate_idx);
void hal_ps_poll_init(uint16_t rate_idx);
void hal_qos_null_init(uint16_t rate_idx, uint8_t tid);
void hal_cts_to_self_init(uint16_t rate_idx);

typedef enum _sw_template_flg {
    PKT_TYPE_RAW = 1 << 0,
    PKT_TYPE_FTM = 1 << 1,
} sw_temp_flg_t;

#endif  //_HAL_INT_TEMPLATE_H_
