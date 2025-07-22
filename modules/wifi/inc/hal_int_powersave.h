/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef _HAL_INT_POWERSAVE_H_
#define _HAL_INT_POWERSAVE_H_

#include "wifi_cmn.h"
#include "nt_common.h"
#include "nt_sme_mlme.h"
#include "hal_int_bcn.h"

// WQ numbers
#define HAL_DPU_RX_WQ             3
#define HAL_DPU_TX_WQ             6
#define HAL_DXE_RX_WQ             11
#define HAL_DXE_RX_WQ_MGMT        12
#define HAL_BTQM_HIGH_PRIORITY_WQ 25
#define HAL_BTQM_LOW_PRIORITY_WQ  26

#define HAL_BMU_RRI_END 5
#define HAL_RXP_RRI_END 30

#ifndef NT_NVIC_ISER0
#define NT_NVIC_ISER0 0xE000E100  // Irq 0 to 31 Set Enable Register
#define NT_NVIC_ISER1 0xE000E104  // Irq 32 to 60 Set Enable Register
#define NT_NVIC_ISER2 0xE000E108  // Irq 32 to 60 Set Enable Register
#define NT_NVIC_ICER0 (0xE000E180)
#define NT_NVIC_ICER1 (0xE000E184)
#define NT_NVIC_ICER2 (0xE000E188)
#endif
#define ENABLE_DXE_IRQ  0xFFE00000  // Enable DXE 11 interrupts
#define ENABLE_DXE_IRQ1 0x00000001  // Enable DXE 12th interrupt

#define HAL_REG_FW_FILLED       1    // FW to fill the value of register
#define HAL_REG_HOST_FILLED     0    // Host to fill the value
#define HAL_REG_RESTORE_IGNORED 2    // fermion 2.0 only
#define HAL_REG_PRI_MASK        0x3  // bit0&bit1

#define HAL_RRI_MAX_DATA_ENTRY (300)
// RRI table command offset
#define HAL_RRI_POLL_CMD_OFFSET (0Xa0000000)
#define HAL_RRI_DMA_CMD_OFFSET  (0Xd0000000)
#define HAL_RRI_MOVE_CMD_OFFSET (0X91000000)  // move 1 DWORD
#define HAL_RRI_WAIT_CMD_OFFSET (0X80000000)  // fermion 2.0 only
#define HAL_RRI_SKIP_CMD_OFFSET (0X81000000)  // fermion 2.0 only
#define HAL_RRI_JMP_CMD_OFFSET  (0X82000000)  // fermion 2.0 only
#define HAL_RRI_END_CMD_OFFSET  (0XFF000000)

#define HAL_RRI_POLL_CMD_START_BIT_OFFSET 0X0
#define HAL_RRI_POLL_CMD_NUM_BIT_OFFSET   0X5
#define HAL_RRI_POLL_CMD_RETRY_TO_OFFSET  0X9
#define HAL_RRI_POLL_CMD_VALUE_OFFSET     0X10
#define HAL_WUR_PHY_DEBUG_MODE_ENABLE \
    0x8  // to enable PHY debug mode. We enabling to check, WUR receiving how many HDR and LDR PKT.

// WiFi driver
#define WUR_SLEEP                             0x4
#define WIFI_CFG                              0x2
#define WIFI_LISTEN                           0x5
#define WIFI_SLEEP                            0x6
#define QWLAN_WUR_WUR_HDM_TBTT_WAKE_COUNT_REG (QWLAN_WUR_BASE + 0xEC)

#define _HAL_WIFI_SS_WAKEUP_MAC_POWERON_TIMEOUT_US 600

extern void (*pre_beacon_interrupt_cb)();

extern void (*tsf_match_interrupt_cb)();
extern void (*tsf_match_beacon_interrupt_cb)();

#ifdef NT_FN_TWT
extern void (*tsf_match_twt_sleep_cb)();
extern void (*mcu_mtu_wakeup_cb)(uint32_t val);

#endif  // NT_FN_TWT

/* @struct	: rri_index_pair_t
 * @brief	: start-end index pair for region of registers in rri list
 */
typedef struct {
    uint16_t start_index; /* Start index for region in list */
    uint16_t end_index;   /* End index for region in list */
} rri_index_pair_t;

/* @struct	: hal_rri_struct_t
 * @brief	: info parameters for RRI save-restore operations
 */
typedef struct {
    uint8_t mcuslp_rri_is_in_sync;   /* if full save of MCU sleep tables was done */
    uint8_t lightslp_rri_is_in_sync; /* if full save of light sleep table was done */
    rri_index_pair_t lightslp_dxe;   /* index pair for DXE entries in rri list light */
    rri_index_pair_t mcuslp_dxe;     /* index pair for DXE entries in rri list second */
} hal_rri_struct_t;

/*Time based on the profiling on chip*/
#define HAL_RRI_LIST_FIRST_RESTORE_TIMEOUT_US  150
#define HAL_RRI_LIST_SECOND_RESTORE_TIMEOUT_US 150
/*Time based on the profiling in emulation, to be profiled on chip*/
#define HAL_RRI_LIST_LIGHT_RESTORE_TIMEOUT_US 450

#define HAL_RRI_HDM_RRI_DELAY_TIME_US 5

typedef enum {
    HAL_RRI_LIST_FIRST = 0,
    HAL_RRI_LIST_SECOND,
    HAL_RRI_LIST_LIGHT,
} HAL_RRI_LIST_TYPE;

void hal_wmac_update_tbtt(uint32_t tbttLo, uint32_t tbttHi);

/*
 * @brief: API to configure beacon interval in MTU, for TBTT update
 * @param : bcn_intv -> BEacon interval in TU
 * @return : none
 */
void hal_wmac_update_bcn_intv(uint16_t bcn_intv);

/*
 * @brief : To check any frame available in TX WQ
 * @returns : Frames available in WQ return NT_EPARAM if not available return NT_OK
 */
nt_status_t nt_hal_tx_pending_wq(void);

/*
 * @brief : To check data available in each and every STA enabled QID queue
 * @returns : Data available in QID queue return NT_EPARAM if not available return NT_OK
 */
nt_status_t nt_hal_btqm_tx_pending_queue(void);

/*
 * @brief : To check no of buffer data in the station
 * @param parameter1: stdid to check no of buffer data
 * @param parameter2: array pointer to store data available status for each queue
 * @returns : no of buffer data present in the station
 */
uint8_t nt_hal_btqm_staid_buffer_data(uint8_t stdid, uint8_t *queue_id_list);

/*
 * @brief : To check any frame available in RX WQ
 * @returns : Frames available in WQ return NT_EPARAM if not available return NT_OK
 */
nt_status_t nt_hal_rx_pending_wq(void);

/*
 * @brief : To disable RX
 * @returns : None
 */
void nt_hal_rx_disable();

/*
 *  @brief : To get the tx and rx count
 *  @param parameter1 : tx_count is the pointer
 *  @oaram parameter2 : rx count is the pointer
 *  @returns : None
 */
void nt_hal_get_tx_rx_count(uint16_t *tx_count, uint16_t *rx_count);
void hal_get_tx_rx_count(uint16_t *tx_count, uint16_t *rx_count);

/*
 *  @brief : To update the beacon in pre beacon interrupt
 *  @param parameter1 :all updated info related to the BSS
 *  @return : None
 */
void nt_hal_bcn_update(nt_hal_bss_t *bss);

/*
 *  @brief : To get the tsf timer value
 *  @param : None
 *  @return : uint64_t timer value
 */
uint64_t nt_hal_tsf_get(void);

/*
 *  @brief : To get the tsf timer value
 *  @param : None
 *  @return : uint64_t timer value
 */
uint32 nt_hal_global_tsf_get(void);

/*
 *  @brief : To get the power save station count
 *  @param : None
 *  @return : uint16_t power save station count
 */
uint16_t nt_hal_get_pwrsave_sta_count(void);

/*
 *  @brief : To get the power save staid list
 *  @param : pointer to point the staid list
 *  @return : uint8_t power save staid list
 */
uint8_t nt_hal_get_pwrsave_staid_list(uint8_t *staid_list);

/*
 *  @brief : To read the number of entries in WQ
 *  @param : pass the wq number
 *  @return : uint32_t return the wq entries
 */
uint32_t nt_hal_read_wq_nr(uint8_t wq);

/*
 *  @brief : To restore all register to receive only beacon
 *  @param : None
 *  @return : None
 */
void nt_hal_rri_restore_first(void);

/*
 *  @brief : To restore all register to hardware wake up
 *  @param : None
 *  @return : None
 */
void nt_hal_rri_restore_second(void);

/*
 *  @brief : To get the index of rri table
 *  @param : pointer to store the register address
 *  @return : index value
 */
uint16_t nt_hal_get_index(uint32_t *reg_address);

/*
 * @brief  : Initiate RRI second list restoration
 * @param  : None
 * @return : None
 */
void nt_hal_rri_trigger_restore_second(void);

/*
 * @brief  : Perform actions to complete RRI second list restoration
 * @param  : None
 * @return : None
 */
void nt_hal_rri_finish_restore_second(void);

/*
 * @brief  : Initiate RRI light list restoration
 * @param  : None
 * @return : None
 */
void nt_hal_rri_trigger_restore_list_light(void);

/*
 * @brief  : Perform actions to complete RRI light list restoration
 * @param  : None
 * @return : None
 */
void nt_hal_rri_finish_restore_list_light(void);

/*
 * @brief  : Check if RRI restoration started for list
 * @param  : list -> RRI list to check start for
 * @return : TRUE if RRI restoration started specified list
 */
uint8_t nt_hal_rri_check_restore_started(HAL_RRI_LIST_TYPE list);

/*
 * @brief  : Check if RRI restoration complete for list
 * @param  : list -> RRI list to check completion for
 * @return : TRUE if RRI restoration complete for specified list
 */
uint8_t nt_hal_rri_check_restore_complete(HAL_RRI_LIST_TYPE list);

/*
 * @brief  : Get RRI restore timeout for list
 * @param  : list -> RRI list to get timeout for
 * @return : uint16_t -> Timeout value for list
 */
uint16_t nt_hal_get_rri_restore_timeout(HAL_RRI_LIST_TYPE list);

/*
 * @brief  : Soft reset RRI module
 * @param  : None
 * @return : None
 */
void nt_hal_rri_soft_reset_rri_engine(void);

/*
 *  @brief : Trimmed version of hal_wlan_sleep to put wifi sleep before soc sleep incase
 * it was turned on inadvertently
 *  @param : None
 *  @return : None
 */
void hal_wlan_sleep_trimmed(void);

/*
 *  @brief : Enter into wifi sleep
 *  @param : uint64_t sleep time value
 *  @return : None
 */
void hal_wlan_sleep(uint32_t sleep_mode);
void nt_wlan_sleep();
void nt_wlan_lightsleep();
void nt_wlan_sleep_driver(/*uint64_t sleep_time*/);

/*
 *  @brief : Take the back up of dxe register before enter into slepp
 *  @param : None
 *  @return : None
 */
void nt_hal_reg_backup_dxe(void);

/*
 *  @brief : Take the back up of rri list first register before enter into sleep
 *  @param : None
 *  @return : None
 */
void nt_hal_rri_list_first_backup(void);

/*
 *  @brief : Take the back up of rri list second register before enter into sleep
 *  @param : None
 *  @return : None
 */
void nt_hal_rri_table_two_backup(void);

/*
 *  @brief : Initialize control parameters used for future RRI backups
 *  @param : None
 *  @return : None
 */
void nt_hal_rri_ctrl_init(void);

/*
 *  @brief : Request resync of complete tables on next RRI backup
 *  @param : None
 *  @return : None
 */
void nt_hal_rri_request_full_resync(void);

/*
 *  @brief : Take back up of rri list corresponding to the sleep mode selected
 *  @param mode : SOC sleep mode for which RRI backup is to be done
 *  @return : None
 */
void nt_hal_rri_backup(sleep_mode mode);

void mtu_fiq_handler(void);
uint32_t nt_hal_pwrsave_bcn_fetch(uint8_t *buf, uint32_t buflen);
uint32_t nt_hal_pwrsave_packets_fetch(uint8_t *buf, uint32_t buflen, uint32_t *pkt_next_idx);
void hal_wlan_sleep_timer_disable(uint32_t sleep_mode);
void nt_twt_wakeup_rri_restore(uint32_t mode);

void nt_wlan_sleep_timer_disable(void);

/*
 *  @brief : To get the tx and rx data packet counts
 *  @param parameter1 : tx_count is the pointer
 *  @oaram parameter2 : rx count is the pointer
 *  @returns : None
 */
void hal_get_tx_rx_data_count(uint16_t *tx_count, uint16_t *rx_count);
void nt_hal_get_tx_rx_data_count(uint16_t *tx_count, uint16_t *rx_count);

/*
 *  @brief : To get the tx and rx Manegement packet counts
 *  @param parameter1 : tx_count is the pointer
 *  @oaram parameter2 : rx count is the pointer
 *  @returns : None
 */
void nt_hal_get_tx_rx_mgmt_count(uint16_t *tx_count, uint16_t *rx_count);
void hal_get_tx_rx_mgmt_count(uint16_t *tx_count, uint16_t *rx_count);

#if (defined NT_FN_WUR_AP) || (defined NT_FN_WUR_STA)
/*
 *  @brief : To enable the WUR clock and WUR Rx
 *  @param : c_bssid - 16 bit compressed bssid to check CRC for WUR beacon and WUR wakeup frame
 *  @param : next_twbt - next twbtt value
 *  @return : return NT_OK
 */

void nt_hal_wur_enable(uint16_t c_bssid, uint64_t next_twbt);

/*
 *  @brief : This function is used for incomming 11ba packets again programmable AID's
 *  @param parameter1: aid_address - array of id uc/bc/mc (only LSB 12bits are valid)
 *  @return : unicast_stdid, munticast_gid if this is ID is no larger than 12 bits return NT_OK otherwise return
 * NT_EPARAM
 */
nt_status_t nt_hal_wur_cfg_frame_filter_set(uint16_t aid_address[4]);

/*
 *  @brief : This function is used to configure the DC
 *  @param parameter1: pdc_interval - period of duty cycle interval(uint - usesc)
 *  @param parameter2: dc_start_time - start of first duty cycle (uint - usesc)
 *  @param parameter3: sp_duration - Length of time WUR should stay awake for each duty cycle (uint - usesc)
 *  @return : pdc_interval, dc_start_time, sp_duration if this is 0 return NT_EPARAM otherwise return NT_OK
 */
nt_status_t nt_hal_wur_cfg_dc_set(uint32_t pdc_interval, uint64_t dc_start_time, uint32_t sp_duration);

/*
 *  @brief : Configure the hardware dtim for wur mode.
 *  @param parameter1: pre_wakeup_time - before TBTT/DC do we need to wake up in (uint - usecs)
 *  @param parameter2: min_sleep_time - Minimum sleep time in usecs before HDM will switchoff NPS (uint - usecs)
 *  @param parameter3: sleep_time - SW on first power down to allow HW to restore TSF Set by HW on subsequent power
 * downs (uint - usecs)
 *  @return : return NT_OK
 */
nt_status_t nt_hal_wur_hdm_enable();  // uint16_t pre_wakeup_time, uint16_t min_sleep_time, uint32_t sleep_time);

/*
 *  @brief : To disable the WUR clock and WUR RX
 *  @param : NONE
 *  @return : return NT_OK
 */
nt_status_t nt_hal_wur_disable(void);

/*
 *  @brief : To read the WUR packet
 *  @param : None
 *  @return : 64 bit WUR packet
 */
uint64_t nt_hal_wur_packet_rd(void);

#endif

#if (defined NT_FN_WUR_AP || (defined NT_FN_WUR_STA))

/*
 *  @brief : This function is used to  configure the 11ba LUTs in the PHY in the TXer
 *  @param : none
 *  @return : None
 */
void nt_hal_wur_lut_init(void);

/*
 *  @brief : This function is used to generate the interrupt when tsf matchs
 *  @param : uint32_t timer value to load mtu timer. only valid lower(0 to 23) 24bits
 *  @return : None
 */
void nt_hal_tsf_match(uint32_t next_twbtt);

/*
 *  @brief : This function is used to generate the interrupt when tsf matchs for wur beacon
 *  @param : uint32_t timer value to load mtu timer. only valid lower(0 to 23) 24bits
 *  @return : None
 */
void nt_hal_tbtt_tsf_match(uint32_t next_twbtt);

/*
 *  @brief : This function is used to put wifi and wur into sleep after confiured HW DTIM
 *  @param : none
 *  @return : none
 */
void nt_hal_wur_enable_sleep();

/*
 *  @brief : This function is used to re-enable hw dtim and enable wifi sleep. Because I Neutrino_1 if station received
 *           wur_vendor frame, along with cpu wake up it will wake wifi module and disable wur_hdtim too. So,
 * programmatically We re-enabling HW dtim  again.
 *  @param : uint32_t timer value to load mtu timer. only valid lower(0 to 23) 24bits
 *  @return : None
 */
void nt_hal_wur_re_enable_hwdtim();

#endif

#ifdef NT_FN_TWT
void nt_hal_twt_timer_disable(void);
NT_BOOL nt_hal_twt_tsf_match_to_sleep_from_wake(devh_t *dev, uint64_t *p_next_twbtt);
#ifdef SUPPORT_STA_TWT_RENEG
NT_BOOL hal_twt_reneg_frame_hdlr(devh_t *dev, uint64_t curr_time, uint64_t *next_twbtt, uint64_t *next_twt_sp);
#endif
#endif  // nt_fn_twt
void hal_rri_restore_list_light(void);
void nt_hal_rri_list_light_backup(void);

#if (defined NT_FN_WMM_PS_AP) || (defined NT_FN_WMM_PS_STA)
/**
 * @brief 	: This function used to hardware config for uapsd support on ap side
 * @param 	: staid - station id
 * @param	: service_period - configured service period for this station
 * @param	: deliver_enable_ac - deliver enabled access category for this station
 * @param	: tigger_enable_ac	- trigger enabled access category for this station
 * @return	: none
 */
void nt_hal_init_uapsd(nt_hal_sta_t *sta, uint8_t service_period, uint8_t deliver_enabled_ac,
                       uint8_t trigger_enabled_ac);
/**
 * @brief 	: This function used to create queue id mask for both deliver and trigger enabled access category
 * @param 	: enable_ac -  Set bit for required Ac. Such us set 1st for VO , 2nd bit for VI, 3rd bit for BK and 4th bit
 * for BE
 * @return	: Required queue id enabled mask
 */
uint8_t halBmu_getQidMask(uint8_t enabled_ac);
/**
 * @brief 	: This function used to enabel more bit interrupt
 * @param 	: none
 * @return	: none
 */
void nt_hal_enable_more_bit_interrupt(void);
/**
 * @brief 	: This function used to disable more bit interrupt
 * @param 	: none
 * @return	: none
 */
void nt_hal_disable_more_bit_interrupt(void);

/*
 * @brief: This function is used to enable/disable pm bit for all data frames
 * @param parameter1: bss pointer to the bss structure
 * @param parameter2: sta pointer to the sta structure
 * @param parameter3: pm_bit value 0 - disable, 1 - enable pm bit
 */
void _hal_tpe_pm_bit_desc_update(nt_hal_bss_t *bss, nt_hal_sta_t *sta, uint8_t pm_bit);

/*
 * @brief: This function used to get connected station power status
 * @param staid - station id
 * @param it will return either 0/1. 0 - This sta_id station not in power save; 1 - This sta_id station is in power save
 */
uint8_t nt_hal_get_connected_station_power_status(uint8_t sta_id);
typedef enum _mtu_hdm_wakeup_reason {
    HDM_WAKEUP_MISS_BEACON = 1,
    HDM_WAKEUP_TSF_OOR,
    HDM_WAKEUP_BEACON_LENGTH,
    HDM_WAKEUP_BEACON_IE,
    HDM_WAKEUP_BEACON_TIM_BC,
    HDM_WAKEUP_BEACON_TIM_UC,
    HDM_WAKEUP_TBTT_COUNT,
    HDM_WAKEUP_DC_COUNT
} mtu_hdm_wakeup_reason;

typedef struct hal_hdm_config {
    // AON
    uint8_t use_slp_tmr;
    uint8_t use_xodiv;
    uint8_t use_hwdtim_with_cpu_on;
    uint8_t use_xo_clk_det;
    uint32_t disable_sleep_mode_en;
    uint32_t xo_settle_time;
    uint32_t lightsleep_en;
    uint32_t aonldo_input_sel;
    uint32_t slp_clk_cal_en;
    uint32_t mx_supply_settle_time;
    uint32_t xo_dtop_on;
    uint32_t pmic_dtop_on;
    // Dynamic Retention during sleep
    uint8_t wmac_mem_ret_en;
    uint8_t phyrx_reg_ret_en;
    uint8_t phyrxa_reg_ret_en;
    uint8_t phytx_reg_ret_en;
    // wakeup
    uint8_t glbcnt_retain;
    uint8_t resv;
    uint8_t check_tbtt_count;
    uint8_t check_dc_count;
    uint32_t tbtt_count;
    uint32_t dc_count;
    uint32_t check_beacon_length;
    uint32_t beacon_length;
    uint32_t check_beacon_tim_bc;
    uint32_t check_beacon_tim_uc;
    uint32_t check_tsf_oor;
    uint32_t tsf_oor;
    uint32_t missed_beacon_window;
    uint32_t missed_beacon_threshold;
    uint32_t missed_beacon_timeout;
    // RRI
    uint32_t rri_delay;
    uint16_t rri_en;
    uint16_t rri_addr_mode;
    uint32_t rri_address;
    // TBTT
    uint64_t next_tbtt;
    uint32_t tbtt_interval;
    // DC
    uint64_t next_dc;
    uint32_t dc_interval;
    uint32_t dc_up_time;
    // power up delays
    uint32_t tsf_adjust;
    uint32_t pre_wake_time;
    uint32_t minimum_sleep_time;
    // initial tsf
    uint32_t tsf;
    uint32_t slp_tmr;
} tHDM_CFG, *pHDM_CFG;

typedef struct hal_hdm_config hal_tbtt_config;
typedef struct hal_hdm_config hal_dc_config;

void hal_wmac_enable_sleep(pHDM_CFG);
void hal_wmac_enable_hwdtim(hal_tbtt_config *tbtt);
void hal_wmac_set_hwdelay(uint32_t tsf_adjust, uint32_t pre_wakeup);              // MTU_MTU_HDM_TSF_ADJUST
void hal_hdm_set_tsf_oor_thres(uint32_t tsf_oor_en, uint32_t tsf_oor_threshold);  // MTU_MTU_HDM_TSF_OOR_CONFIG
void hal_hdm_set_bcn_missed_config(uint32_t missed_beacon_timeout, uint32_t missed_beacon_window,
                                   uint32_t missed_beacon_threshold);
void hal_hdm_set_bcn_ie_hash_check(uint16_t index, uint16_t ie_check_id, uint32_t ie_hash);
void hal_hdm_set_bcn_len_check(uint32_t en, uint32_t bcn_len);
void hal_hdm_set_bcn_tim_uc(uint32_t en);
void hal_hdm_set_bcn_tim_bc(uint32_t en);

void hal_hdm_set_bcn_tsf_adjust(uint32_t beacon_tsf_adjust);
uint64_t wakeup_beaconProcess(__unused uint32_t wkup_delay);
uint16_t hal_get_index(sleep_mode mode, uint32_t reg_address);
void hal_reg_backup_dxe(sleep_mode mode);
uint16_t hal_wlan_in_sleep();
void hal_enable_cxc_wakeup(uint32_t en);
void hal_wmac_hw_rri_restore(uint32_t rri_tbl_sel);

/*
 * @brief  : Set RRI start delay in HDM
 * @param  : delay_us -> delay time in microseconds
 * @return : None
 */
void hal_set_hdm_rri_delay(uint8_t delay_us);
void hal_wmac_hw_rri_save(uint32_t rri_tbl_sel);
uint32_t hal_create_hw_rri_tbl(uint32_t rri_tbl_sel);
void hal_reg_backup_rest(sleep_mode mode);
void hal_reg_restore_rest(sleep_mode mode);
uint32_t hal_get_light_rri_addr(void);
uint32_t hal_get_light_first_rri_addr(void);
uint32_t hal_get_first_rri_addr(void);

typedef enum _twt_overwrite_md_ack_ba {
    SW_MODE_SET_LOW = 1,
    SW_MODE_SET_HIHG,
    SW_MODE_SET_STAID_LOW,
    SW_MODE_SET_STAID_HIGH,
    HW_MODE_SET,
    HW_MODE_SET_STAID,
} twt_over_mode;
/*
 * @brief: This function used enabble feature in FERMTWO-41
 * @param mode: list in twt_over_mode
 * @param staId: which staId use this overwrite feature just only apply STAID check mode
 * @param backEngMask: which backoff enginer apply this feature, just only in HW mode
 */
void hal_mod_tpe_mb_reg_set(twt_over_mode mode, uint32_t staId, uint32_t backEngMask);
void nt_wlan_deepsleep();

#ifdef SUPPORT_BMU_ERROR_RECOVERY
/* Recipe to reset WLAN via power off and power on to recover from BMU error */
void hal_wlan_power_cycle_for_bmu_recovery(void);
#endif /* SUPPORT_BMU_ERROR_RECOVERY */

#define HAL_BEACON_TIME_UNITS           (1024)
#define HAL_BEACON_TBTT_CALC(bcn_intvl) (bcn_intvl * HAL_BEACON_TIME_UNITS)
#define HAL_AON_CLK_TICK                (31)  //(1/32.768KHZ)*X=1000
#endif
#endif  //_HAL_INT_POWERSAVE_H_
