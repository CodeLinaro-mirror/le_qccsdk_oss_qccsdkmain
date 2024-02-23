/*========================================================================
*
* @file coex_brsim.h
* @brief BTSIM related function headerfile
*========================================================================*/

#ifndef COEX_BTSIM_H
#define COEX_BTSIM_H

#ifdef SUPPORT_COEX
#ifdef PLATFORM_FERMION
#ifdef SUPPORT_COEX_SIMULATOR
/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/
enum mci_msg_inject
{
	MCI_CONT_INFO_MESSAGE = 0,
	MCI_CONT_INFO_V2_MESSAGE,
	MCI_CONT_RST_MESSAGE,
	MCI_SCHD_INFO_MESSAGE,
	MCI_INVALID_MESSAGE,
};

/*-------------------------------------------------------------------------
 * Function Declarations
 * ----------------------------------------------------------------------*/

void coex_set_btsim_timers_seq0(uint32_t * timer_values);
void coex_set_btsim_timers_seq1(uint32_t * timer_values);

void coex_msg_ctrl_seq0(uint32_t * msg_ctrl);
void coex_msg_ctrl_seq1(uint32_t * msg_ctrl);

void coex_config_and_enable_btsim(uint8_t seq0_en, uint8_t seq1_en);
void coex_config_and_enable_btsim_for_timer(void);

void coex_inject_mci_msg(uint8_t mci_msg_type, uint8_t is_rx, uint8_t is_crx_allowed, uint8_t bt_rssi);

#endif /* SUPPORT_COEX_SIMULATOR */
#endif /* PLATFORM_FERMION */
#endif /* SUPPORT_COEX */
#endif /* COEX_BTSIM_H */
