/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#ifndef _PHY_RESET_H_
#define _PHY_RESET_H_

typedef enum cal_type_id {
    NO_CAL = 0x00000000,
    PROCESS_CAL = 0x00000001,
    RXDCO_CAL = 0x00000002,
    TXLO_CAL = 0x00000004,
    TXIQ_CAL = 0x00000008,
    RXIQ_CAL = 0x00000010,
    DPD_CAL = 0x00000020,
    EDET_CAL = 0x00000040,
    RTT_CAL = 0x00000080,
    ALL_CALS = (RXDCO_CAL | TXLO_CAL | TXIQ_CAL | RXIQ_CAL | DPD_CAL)
} cal_type_id_t;

#if !defined(BIT_0)
#define BIT_NUM(x) BIT_##x = (1 << x)
typedef enum bit_number_s {
    BIT_NUM(0),
    BIT_NUM(1),
    BIT_NUM(2),
    BIT_NUM(3),
    BIT_NUM(4),
    BIT_NUM(5),
    BIT_NUM(6),
    BIT_NUM(7),
    BIT_NUM(8),
    BIT_NUM(9),
    BIT_NUM(10),
    BIT_NUM(11),
    BIT_NUM(12),
    BIT_NUM(13),
    BIT_NUM(14),
    BIT_NUM(15),
    BIT_NUM(16),
    BIT_NUM(17),
    BIT_NUM(18),
    BIT_NUM(19),
    BIT_NUM(20),
    BIT_NUM(21),
    BIT_NUM(22),
    BIT_NUM(23),
    BIT_NUM(24),
    BIT_NUM(25),
    BIT_NUM(26),
    BIT_NUM(27),
    BIT_NUM(28),
    BIT_NUM(29),
    BIT_NUM(30),
    BIT_NUM(31),

} bit_number_t;

#define BIT(x) BIT_##x
/* below definition for userParam2 of Q5_Reset */
//#define PARAM2_XLNA_EN	BIT_0
//#define PARAM2_COEX_EN	BIT_1
#endif
void logResetParams(PHYDEVLIB_PHY_INPUT *input, PHYDEVLIB_RESET_INPUT *resetInput);
uint16_t phyrf_get_current_freq(void);
void phyrf_otp_trim_rfa(uint32_t board_id, uint8_t bandCode, uint16_t mhz);
void phyrf_otp_trim_rfa_by_bandcode(uint8_t bandCode, uint16_t mhz);
void phyrf_otp_trim_pmu(void);
void ferm_cpr_init(void);
void ferm_reenable_cpr(void);
void ferm_disable_cpr(void);

#if defined(EMULATION_BUILD) && defined(PLATFORM_FERMION)
void inject_impairment(cal_type_id_t calMask);  // for FPGA emulation
void remove_impairment(cal_type_id_t calMask);  // for FPGA emulation
int8_t bFermion20Emu(void);                     // return true for F2.0 emulation
#else
#define inject_impairment(calMask)
#define remove_impairment(calMask)
#endif
#endif  // _PHY_RESET_H_
