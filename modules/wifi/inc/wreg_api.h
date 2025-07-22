/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

//
// This file contains the API exported by the wireless regulatory
// module.
//
// $Id: //depot/sw/branches/olca/target/include/wreg_api.h#7 $
//

#ifndef _WREG_API_H_
#define _WREG_API_H_

typedef uint32_t NT_REG_CODE; /* Regulatory code */

/*
****************************************************************
**    Interpretation of wreg_channel_t.c_attrib bit field..
**
**    Freq Pwr Adhoc Active
**              0      0    : No Adhoc. INF mode ONLY. No active scan.
**              0      1    : No Adhoc. INF mode only. ACTIVE scan OK
**              1      0    : Adhoc Mode and INF Mode OK. In INF mode,
**                               NO ACTIVE scan.
**              1      1    : Adhoc Mode and INF Mode OK. In INF mode,
**                                   ACTIVE scan OK.
**
*****************************************************************
*/
typedef struct {
    uint32_t c_attrib;
    void *c_reserved; /* reserved to support potential future changes */
    uint16_t c_freq;
    uint8_t c_tx_pwr;
} wreg_channel_t;

#ifdef ATH_KF
#define KEY_LEN 80
typedef struct keyType_t {
    char keyLen;    /* len of the key.. used when querying(for ex:get-op)*/
    char keyOffset; /* Where is the key located in a table-row? */
    union {
        char keyBuf[KEY_LEN]; /* Actual key */
        uint16_t uint16;
    } u;
} __ATTRIB_PACK KEY_TYPE;

typedef struct reg_query_resp_t {
    char op;  /* Operation: get, get-resp... */
    char tag; /* Table to operate on */
    union {
        uint8_t buf[KEY_LEN];
        KEY_TYPE key;
        REG_DMN_PAIR_MAPPING dmnPair;
        COUNTRY_CODE_TO_ENUM_RD ccCode;
        REG_DMN_FREQ_BAND freqBand;
        REG_DOMAIN regDmn;
    } u;
} __ATTRIB_PACK REG_QUERY_RESP_CMD;
#endif  // ATH_KF
/*
 * c_attrib definitions
 */
#define CHANNEL_DFS              0x0040  /* DFS on this channel */
#define CHANNEL_WLANMODE_MASK    0x0003  /* use HAL_PHY_MODE enum */
#define CHANNEL_ACTIVE_SCAN      0x8000  /* active scanning allowed */
#define CHANNEL_FIRST_IN_SUBBAND 0x00080 /* first channel in the band */
#define CHANNEL_HT20_ALLOWED     0x10000 /* HT20 mode allowed */
#define CHANNEL_HT40_ALLOWED     0x20000 /* HT40 mode allowed */
#define CHANNEL_QUARTER_RATE     0x40000 /* Quarter rate channel */
#define CHANNEL_HALF_RATE        0x80000 /* Half rate channel */

#define CHANNEL_PHYMODE(ch) ((ch)->c_attrib & CHANNEL_WLANMODE_MASK)
#endif /* _WREG_API_H_ */
