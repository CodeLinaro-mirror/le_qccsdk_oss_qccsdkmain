/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef CORE_WIFI_HAL_INC_HAL_RTT_H_
#define CORE_WIFI_HAL_INC_HAL_RTT_H_
#include "nt_common.h"
#include "wifi_cmn.h"

#include "fwconfig_wlan.h"
#include "nt_flags.h"
#include "hal_api_sys.h"

#ifdef NT_FN_FTM

#define NT_ENABLE           1
#define NT_DISABLE          0
#define NT_PARTIAL_TSF_MASK 0x3FFFC00

typedef enum {
    RTT_FTM_INITIATOR = 0,
    RTT_FTM_RESPONDER = 1,
} RTT_FTM_MODE;

typedef enum {
    FTMR_TRIGGER_STOP = 0,
    FTMR_TRIGGER_START = 1,
} FTMR_TRIGGER;

typedef enum {
    LEGACY_ACK_RATE = 0,
    HT_ACK_RATE = 1,
} ACK_RATE_MODE;

typedef struct ieee80211_ftm_action_PAYLOAD {
    uint8_t ia_category;       // 1 byte
    uint8_t ia_action;         // 1 byte
    uint8_t dialog_token;      // 1 byte
    uint8_t fUp_dialog_token;  // 1 byte
    // uint64_t time_of_depart;//6 byte
    // uint64_t time_of_arrival;//6 byte
    uint8_t time_of_depart[6];
    uint8_t time_of_arrival[6];
    uint16_t tod_error;  // 2 byte
    uint16_t toa_error;  // 2 byte
    uint32_t CRC;        // 4 byte
    // FTM_IE_T ftm_ie_params;
    // FTM_SYNC_IE ftm_sync_info_ie;

} IEEE80211_FTM_PAYLOAD_FRAME;

typedef struct ieee80211_ftmr_action_PAYLOAD {
    uint8_t ia_category;  // 1 byte
    uint8_t ia_action;    // 1 byte
    uint8_t trigger;      // 1 byte
    uint32_t CRC;         // 4 byte
    // FTM_IE_T ftm_ie_params;
    // FTM_SYNC_IE ftm_sync_info_ie;

} IEEE80211_FTMR_PAYLOAD_FRAME;

typedef struct ieee80211_RTT_action_frame {
    struct ieee80211_frame ftm_mac;
    uint8_t ia_category;
    uint8_t ia_action;
    uint8_t dialog_token;
    uint8_t fUp_dialog_token;
    uint8_t time_of_depart[6];
    uint8_t time_of_arrival[6];
    uint16_t tod_error;
    uint16_t toa_error;
} IEEE80211_RTT_ACTION_FRAME;

/*
 *  @brief  : To clear rtt configuration
 *  @param  : None
 *  @return : status
 */
nt_status_t nt_hal_rtt_ftm_off(void);

/*
 *  @brief  : To configure PHY for RTT3 Initiator/Responder mode
 *  @param  : rtt_ftm_mode_e initiator/responder
 *  @param  : bss hal_bss structure
 *  @return : status
 */
nt_status_t nt_hal_rtt_ftm_init(RTT_FTM_MODE rtt_ftm_mode_e, ACK_RATE_MODE ack_rate_mode_e, nt_hal_bss_t *bss,
                                uint8_t *macaddr);

/*
*  @brief  : To configure PHY for RTT3 responder mode
*  @param  : None
*  @return : status

nt_status_t nt_hal_init_rtt3_as_responder(void);*/

/*
 *  @brief  : To do clock configuration to enable/disable RTT
 *  @param  : None
 *  @return : none
 */
void nt_hal_rtt_ftm_set_rtt_clk_state(uint8_t state);

/*
*  @brief  : To do clock configuration to disable RTT
*  @param  : None
*  @return : none


void nt_hal_rtt_clk_disable(void);*/

/*
 *  @brief  : To enable/disable t4 capture interrupt in TPE
 *  @param  : None
 *  @return : status
 */
nt_status_t nt_hal_rtt_ftm_enable_t4_capture_interrupt(uint8_t intterupt_state);

nt_status_t nt_hal_rtt_ftm_enable_t2_capture_interrupt(uint8_t intterupt_state);

/*
*  @brief  : To get T1 and T4 value
*  @param  : t1 - address of t1
*  @param  : t4 - address of t4
*  @return : status

nt_status_t nt_hal_get_t1_t4_value(uint32_t *t1, uint32_t *t4);*/

/*
 *  @brief  : To get t4 and t1 delta value, after received t4 capture interrupt
 *  @param  : None
 *  @return : (t4-t1) value
 */
uint16_t nt_hal_rtt_ftm_get_t4_t1_delta_value(void);

/*
 *  @brief  : To get t1 absolute value, after received t4 capture interrupt
 *  @param  : None
 *  @return : t1 absolute value
 */
uint32_t nt_hal_rtt_ftm_get_t1_absolute_value(void);

/*
 *  @brief  : To get the Fac value, after received t4 capture interrupt
 *  @param  : None
 *  @return : raw_fac_value
 */
uint32_t nt_hal_rtt_ftm_get_FAC_value(void);

/*
 *  @brief  : To get the Valid RTT count
 *  @param  : None
 *  @return : valid count
 */
uint16_t nt_hal_rtt_ftm_valid_count_check(void);

/*
 *  @brief  : To get the TSF SYNC value for rtt IE
 *  @param  : None
 *  @return : 4 least significant octets of the value of TSF
 */
uint32_t nt_hal_get_ftm_tsf_sync(void);

/*
 *  @brief  : To get the partial tsf timer value for FTM params
 *  @param  : None
 *  @return : 25:10 bits  of the value of 64 bit TSF timer value
 */
uint16_t nt_hal_get_ftm_partial_tsf(void);

#ifdef NT_FN_RTT_FTM_DBG
uint16_t nt_che_rtt(void);
uint16_t nt_delta_meas_init(void);
#endif  // NT_FN_RTT_FTM_DBG

#endif /* NT_FN_FTM */

#endif /* CORE_WIFI_HAL_INC_HAL_RTT_H_ */
