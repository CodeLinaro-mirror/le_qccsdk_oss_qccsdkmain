/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _HAL_API_SYS_H_
#define _HAL_API_SYS_H_

#include <stdint.h>
#include "ieee80211_defs.h"  // included for accessing wmm parameters structure
#include "nt_common.h"
//#include "hal_int_security.h" //included for accessing encryption type structure

#define HAL_STATUS(err) (NT_MODULE_HAL | (err))

#define HAL_NBSS_MAX            2   // max number of BSS supported
#define HAL_NSTA_MAX            7   // max number of STA's supported (includes self STA)
#define HAL_RETRY_RATES_NUM_MAX 3   // max number of retry rates supported
#define HAL_TIDS_MAX            16  // max number of TID's supported (<= 16)

#define HAL_MACADDR_SZ       6  // mac address size
#define HAL_MACADDR_LOW_WORD 4  // mac address size

// these are also used to index into the BSS table - do not change these defs
#define HAL_BSSMODE_AP  0  // BSS started as an infra-AP
#define HAL_BSSMODE_STA 1  // STA connecting to an infra AP
#ifndef SUPPORT_TWO_STA_CONC
#define HAL_BSSMODE_MAX 2

#define HAL_AP_START_STAIDX 3  // Staidx for connected stations starts from 3 on AP side
#else
#define HAL_BSSMODE_STA2 2
#define HAL_BSSMODE_MAX  3

#define HAL_AP_START_STAIDX 4  // Staidx for connected stations starts from 4 on AP side
#endif
#define DPU_IDX_STA1        0  // dpu index used for sta1
#define DPU_IDX_STA2        1  // dpu index used for sta2
#define DPU_IDX_INVALID     0xFF
#define HAL_BSSMODE_INVALID 0xFF
#define GET_STA_ID_FROM_HAL_MOD(x)               \
    x /* mode and sta_id has one to one mapping, \
       * and staid is initalized with mode       \
       * in nt_hal_bss_add function              \
       */
typedef struct nt_hal_ba_params_s {
    // Block ack supported parameters
    uint16_t ba_buffersize;
    uint16_t ba_tid;
    uint8_t ba_direction;
    uint16_t seq_num;
    uint8_t ba_enable;

} nt_hal_ba_params_t;

typedef struct nt_hal_sta_s {
    // First two fields bssid and assocId are used to find staid for sta.
    uint16_t aid;                     // assoc id - on a per bss basis
    uint8_t sta_mac[HAL_MACADDR_SZ];  // MAC Address of STA
    uint16_t listen_interval;         // Listen interval.

    // these are assigned for internal use by HAL
    uint8_t sta_idx;
    uint8_t rmf;

    uint8_t p_rate;  // primay tx rate table index
    uint8_t s_rate;  // secondary tx rate table index
    uint8_t t_rate;  // tertiary tx rate table index

#if 0
		// Field to indicate if this is sta entry for itself STA adding entry for itself
		// or remote (AP adding STA after successful association.
		// This may or may not be required in production driver.
		tANI_U8 staType;       // 0 - Self, 1 other/remote, 2 - bssid

		tANI_U8 shortPreambleSupported;

		// Support for 11e/WMM
		tANI_U8 wmmEnabled;

		//
		// U-APSD Flags: 1b per AC
		// Encoded as follows:
		// b7 b6 b5 b4 b3 b2 b1 b0
		// X  X  X  X  BE BK VI VO
		//
		// For DVT debug this flag shall
		// be encoded as follows:
		// b7 b6 b5 b4 b3 b2 b1 b0
		// BE BK VI VO BE BK VI VO
		// LSB four bits are used as
		// delivery enabled flags
		// MSB four bits are used as
		// trigger enabled flags
		// To configure an AC as both delivery
		// and trigger enabled set both the
		// corresponding bits to 1
		tANI_U8 uAPSD;

		// Max SP Length
		// encoding is same as WMM spec
		// 0 - deliver all buffered frames
		// 1 - deliver max of 2 buffered frames
		// 2 - deliver max of 4 buffered frames
		// 3 - deliver max of 6 buffered frames
		tANI_U8 maxSPLen;

		// 11n HT capable STA
		tANI_U8 htCapable;

		// 11n Green Field preamble support
		// 0 - Not supported, 1 - Supported
		// Add it to RA related fields of sta entry in HAL
		tANI_U8 greenFieldCapable;

		// TX Width Set: 0 - 20 MHz only, 1 - 20/40 MHz
		tANI_U8 txChannelWidthSet;

		// MIMO Power Save
		tSirMacHTMIMOPowerSaveState mimoPS;

		// RIFS mode: 0 - NA, 1 - Allowed
		tANI_U8 rifsMode;

		// L-SIG TXOP Protection mechanism
		// 0 - No Support, 1 - Supported
		// SG - there is global field.
		tANI_U8 lsigTxopProtection;

		// delayed ba support
		tANI_U8 delBASupport;
		// delayed ba support... TBD

		// FIXME
		//Add these fields to message
		tANI_U8 us32MaxAmpduDuration;                //in units of 32 us.
		tANI_U8 maxAmpduSize;                        // 0 : 8k , 1 : 16k, 2 : 32k, 3 : 64k
		tANI_U8 maxAmpduDensity;                     // 3 : 0~7 : 2^(11nAMPDUdensity -4)
		tANI_U8 maxAmsduSize;                        // 1 : 3839 bytes, 0 : 7935 bytes

		// TC parameters
		tTCParams staTCParams[SMAC_STACFG_MAX_TC];

		// Compression and Concat parameters for DPU
		tANI_U16 deCompEnable;
		tANI_U16 compEnable;
		tANI_U16 concatSeqRmv;
		tANI_U16 concatSeqIns;

		// station attribute: taurus/titan/polaris/legacy11a/b/g
		tStaRateMode rateMode;


		//11n Parameters

		/**
			HT STA should set it to 1 if it is enabled in BSS
			HT STA should set it to 0 if AP does not support it.
			This indication is sent to HAL and HAL uses this flag
			to pickup up appropriate 40Mhz rates.
		*/
		tANI_U8 fDsssCckMode40Mhz;


		//short GI support for 40Mhz packets
		tANI_U8 fShortGI40Mhz;

		//short GI support for 20Mhz packets
		tANI_U8 fShortGI20Mhz;

		/*
		* All the legacy and airgo supported rates.
		*/
		tSirSupportedRates supportedRates;

		/*
			* Following parameters are for returning status and station index from HAL to PE
			* via response message. HAL does not read them.
			*/
			// The return status of SIR_HAL_ADD_STA_REQ is reported here
		eHalStatus status;
		// Station index; valid only when 'status' field value is eHAL_STATUS_SUCCESS
		tANI_U16 staIdx;

		//BSSID of BSS to which the station is associated.
		//This should be filled back in by HAL, and sent back to LIM as part of
		//the response message, so LIM can cache it in the station entry of hash table.
		//When station is deleted, LIM will make use of this bssIdx to delete
		//BSS from hal tables and from softmac.
		tANI_U16 bssIdx;
		/* this requires change in testDbg. I will change it later after coordinating with Diag team.
			tANI_U8 fFwdTrigerSOSPtoHost; //trigger to start service period
			tANI_U8 fFwdTrigerEOSPtoHost; //trigger to end service period
		*/
		//A flag to indicate to HAL if the response message is required.
		tANI_U8 respReqd;
		// lifetime
		tANI_U8 enableLifetimeQid00;
		tANI_U16 lifetimeThresholdQid00;
		tANI_U8 enableLifetimeQid01;
		tANI_U16 lifetimeThresholdQid01;
		tANI_U8 enableLifetimeQid02;
		tANI_U16 lifetimeThresholdQid02;
		tANI_U8 enableLifetimeQid03;
		tANI_U16 lifetimeThresholdQid03;
		tANI_U8 enableLifetimeQid04;
		tANI_U16 lifetimeThresholdQid04;
		tANI_U8 enableLifetimeQid05;
		tANI_U16 lifetimeThresholdQid05;
		tANI_U8 enableLifetimeQid06;
		tANI_U16 lifetimeThresholdQid06;
		tANI_U8 enableLifetimeQid07;
		tANI_U16 lifetimeThresholdQid07;

#endif

    nt_hal_ba_params_t hal_ba_params;

} nt_hal_sta_t;

typedef struct nt_hal_bss_s {
    uint8_t bss_id[HAL_MACADDR_SZ];     // MAC Address/BSSID/same as wifi local mac address
    uint8_t local_mac[HAL_MACADDR_SZ];  // local wifi mac address, ignored for BSSMODE_AP,
                                        // for BSSMODE_STA this is used to configure STA desc

    uint8_t bss_mode;  // HAL_BSSMODE_* (AP or STA)

    uint8_t bcn_dtim;       // AP: DTIM count (number of beacons, 0/1 => all bcns are DTIM)
    uint16_t bcn_interval;  // AP: delta between beacons (number of TU's)
    uint8_t bss_bcn_rate;   // AP: 0 => default, [0, HAL_RT_IDX_MAX_RATES)
    uint8_t bss_uc_rate;    // AP: 0 => default  [0, HAL_RT_IDX_MAX_RATES)
    uint8_t bss_mcbc_rate;  // AP: 0 => default  [0, HAL_RT_IDX_MAX_RATES)
    uint8_t *bcn;           // AP: pointer to beacon mgmt frame (not including template header)
    uint16_t bcn_len;       // AP: number of bytes in the beacon

    // these are assigned for internal use by HAL
    uint8_t bss_sta_idx : 1;  // assigned sta id for the BSS itself
    uint8_t bss_num_sta : 3;  // number of sta's in the BSS
    uint8_t bss_sta_map;      // bitmap of sta's indices assigned in the BSS
                              // 0/1 are reserved for BSS AP/STA; rest are for STA's in AP

    // Broadcast DPU descriptor index allocated by HAL and used for broadcast/multicast packets.
    uint8_t bcast_dpu_desc_idx;

#ifdef ENABLE_MCS4_RX
    uint8_t mcs4_rx_enabled;
#endif /*ENABLE_MCS4_RX*/

#if 0
		// MAC Rate Set
		// Review FIXME - Does HAL need this?
		tSirMacRateSet rateSet;

		// 802.11n related HT parameters that are dynamic

		// Enable/Disable HT capabilities
		tANI_U8 htCapable;

		// HT Operating Mode
		// Review FIXME - Does HAL need this?
		tSirMacHTOperatingMode htOperMode;

		// Dual CTS Protection: 0 - Unused, 1 - Used
		tANI_U8 dualCTSProtection;

		// TX Width Set: 0 - 20 MHz only, 1 - 20/40 MHz
		tANI_U8 txChannelWidthSet;

		// Current Operating Channel
		tANI_U8 currentOperChannel;

		// Current Extension Channel, if applicable
		tANI_U8 currentExtChannel;
#endif

#if 0
		/*
		* Following parameters are for returning status and station index from HAL to PE
		* via response message. HAL does not read them.
		*/
		// The return status of SIR_HAL_ADD_BSS_REQ is reported here
		eHalStatus status;
		// BSS index allocated by HAL.
		// valid only when 'status' field is eHAL_STATUS_SUCCESS
		tANI_U16 bssIdx;

		// DPU signature to be used for broadcast/multicast packets
		// valid only when 'status' field is eHAL_STATUS_SUCCESS
		tANI_U8    bcastDpuSignature;

		// Lsig Txop Protection supported for all stations in this BSS
		tANI_U8 fLsigTXOPProtectionFullSupport;

		//HAL will send the response message to LIM only when this flag is set.
		//LIM will set this flag, whereas DVT will not set this flag.
		tANI_U8 respReqd;
#endif

    void (*pre_beacon_callback)(void);                   // call back function
    void (*bad_decrypt_error_interrupt_callback)(void);  // call back function for bad decrypt error
    void (*mic_error_interrupt_callback)(void);          // call back function for mic error
    void (*eosp_interrupt_callback)(uint8_t tid, uint8_t more_bit,
                                    uint8_t staid);  // Call back function for eosp interrupt
    void (*more_bit_interrupt_callback)(void);       // More bit interrupt callback

    void (*tsf_match_callback)();         // call back function for tsf match
    void (*tsf_match_beacon_callback)();  // call back function for twbtt tsf match for wur beacon
#if (defined NT_FN_TWT)
    void (*tsf_match_twt_sleep_callback)();
#endif  // NT_FN_TWT
#if (defined NT_FN_WUR_STA)
    /*! @function : wur_packet_interrupt_callback
     * 	@Brief	:	call back funtion for received wur frame
     * 	@Param	:	received frame byte
     * */
    void (*wur_packet_interrupt_callback)(uint64_t payload);
    void (*wake_main_radio_interrupt_callback)();
    void (*wur_beacon_miss_interrupt_callback)();
    void (*wur_packet_crc_error_interrupt_callback)();  // Wur packet crc error interrupt callback
#endif
#ifdef NT_FN_FTM
    void (*nt_rtt_t4_capture_interrupt_callback)();
    void (*nt_rtt_t2_capture_interrupt_callback)();
#endif

} nt_hal_bss_t;

/**
 * @brief Start a BSS
 * Initializes all the resources needed for the BSS - only one of each type is supported, caller beware
 * A BSS is started as an infra AP or as infra STA
 * An AddBss adds a STA as part of the bss addition
 * This STA should always be the correponding AP
 * Thus in the case of an infra AP
 *      AddBss - adds local BSS (i.e AP)
 *             - adds local sta (the AP itself)
 * In the case of infra STA
 *      AddBss - adds the local BSS (i.e the BSS we are associating with)
 *             - adds remote sta (i.e the AP itself)
 * In the case of IBSS (deprecated/unsupported)
 *      AddBss - either STA is starting IBSS or associating with an IBSS.
 *
 * @param bss      all info related to the BSS being added
 * @return eNT_OK  on success
 */
nt_status_t nt_hal_bss_add(nt_hal_bss_t *bss);

/**
 * @brief Delete a BSS
 * Deletes/frees all the resources associated with the BSS
 *
 * @param bss      		all info related to the BSS being added
 * @param conc_mode 	concurrency mode
 * @return eNT_OK  on success
 */
nt_status_t nt_hal_bss_del(nt_hal_bss_t *bss, uint8_t conc_mode);

/**
 * @brief Add a new STA
 * Updates hardware resource info for the specified STA (belonging to a previously added BSS)
 * @param bss      info about the BSS to which the STA is added
 * @param sta      info about the STA being added
 * @return eNT_OK  on success
 */
nt_status_t nt_hal_sta_add(nt_hal_bss_t *bss, nt_hal_sta_t *sta);

/**
 * @brief Delete an existing STA
 * Deletes/frees hardware resource info for the specified STA
 * @param bss      info about the BSS from which the STA is deleted
 * @param sta      info about the STA being added
 * @return eNT_OK  on success
 */
nt_status_t nt_hal_sta_del(nt_hal_bss_t *bss, nt_hal_sta_t *sta);

/**
 * @brief Control enable/disable of rx/tx
 * Use this to enable scans
 * Temporarily enables/disables rx/tx - will be removed and replaced with a scan api soon
 * @param enable   disable (0) or enable (1)
 * @return eNT_OK  on success
 */
void hal_modules_txrx_enable(uint8_t enable);
void hal_mod_bmu_sta_disable(uint8_t staidx);
void _hal_mod_bmu_sta_enable(uint8_t staidx);
void hal_mod_rxp_desc_set_mac(uint8_t *mac);
nt_status_t nt_hal_ba_add(nt_hal_bss_t *bss, nt_hal_sta_t *sta);
nt_status_t nt_hal_ba_del(nt_hal_bss_t *bss, nt_hal_sta_t *sta);

#if (FERMION_CHIP_VERSION == 2)
nt_status_t nt_hal_wmm_params_set(struct chanAccParams *wmm, uint32_t phyRateFullHalfQuarter);
#else
nt_status_t nt_hal_wmm_params_set(struct chanAccParams *wmm);
#endif
nt_status_t nt_hal_mod_wmmparam_cw(uint8_t qid, uint16_t min, uint16_t max);
nt_status_t nt_hal_mod_ba_win_size(uint16_t ack_timeout, uint16_t delay);
nt_status_t nt_hal_mod_slot_time(uint32_t slot_time);

uint32_t hal_find_seq_num(nt_hal_bss_t *bss, uint32_t staid, uint32_t batid);
uint32_t nt_hal_get_seq_num(nt_hal_bss_t *bss, uint32_t staid, uint32_t batid);
uint8_t hal_dpu_descidx_get(uint8_t mode, uint8_t staid);
void hal_rpe_blockandflush_cache(uint8_t staidx, uint8_t tid, uint8_t blockreq);
void hal_rpe_updateblockreq(uint8_t staidx, uint8_t batid, uint8_t blockreq);
void hal_rpe_flushbitmap_cache(void);
void hal_rpe_flushsrc_entry(uint8_t staidx, uint8_t batid);
/**
 * @brief  Enables eosp interrupt
 * @params none
 * @retun  none
 */
void nt_hal_enable_eosp_interrupt(void);
/**
 * @brief  Disables eosp interrupt
 * @params none
 * @retun  none
 */
void nt_hal_disable_eosp_interrupt(void);

/* To update the RMF bit after keys are generated.
 * @param bss      info about the BSS to which rmf bit should update
 * @param sta      info about the STA to which rmf bit should update
 * @param rmf      rmf bit to be updated.
 */
void hal_modules_rmf_update(nt_hal_bss_t *bss, nt_hal_sta_t *sta, uint8_t rmf);
uint8_t hal_tblidx_get(uint8_t mode, uint8_t staid);
void hal_rxp_filter_manipulation(uint8_t testvar);

/* To get device status info by sta_id.
 * @param bss      sta id of any connected station
 */
void nt_hal_get_uapsd_get_info_(uint8_t sta_id);

#endif  // _HAL_API_SYS_H_
