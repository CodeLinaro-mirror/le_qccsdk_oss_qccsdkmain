/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "nt_hw.h"
#include "nt_logger_api.h"
#include "nt_crypto.h"
#include "hal_int_sys.h"
#include "nt_flags.h"
#include "nt_hw.h"
#include "nt_logger_api.h"
#include "nt_crypto.h"
#include "hal_int_sys.h"

#ifdef NT_FN_HW_CRYPTO

extern void wifi_crypto_secip_request(uint8_t enable_secip_module);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* SHA 1 */
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32_t sha1_known_answer_initvector_piomode(const uint8_t *p_start, uint32_t *p_iv, uint32_t size, uint8_t *output)
#ifdef WIFI_HW_SHA_WPA2
{
    uint32_t *p_din = NULL;
    uint32_t final_auth_iv[5] = {0};
    uint32_t input_size;
    uint32_t val = 0;

    /*uint32_t initial_auth_iv[8] = { 0x01234567, \
                                    0x89ABCDEF, \
                                    0xFEDCBA98, \
                                    0x76543210, \
                                    0xF0E1D2C3, \
                                  };*/
    p_din = (uint32_t *)p_start;
    input_size = size / 4;

    (void)p_iv;
    (void)val;
    // SW mode for crypto core
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_CORE_CFG_REG, 0x2);
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_BYTECNT0_REG, 0x0);
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_BYTECNT1_REG, 0x0);

    // segment configuration - Hash SHA
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_CFG_REG, 0x30001);

    // segment configuration
    // HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_REG, );

    // authorization segment configuration
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_SIZE_REG, size);

    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SEG_SIZE_REG, size);

    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_START_REG, 0);

    // Authentication initial Vector - write counter IV
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_IV0_REG, p_iv[0]);
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_IV1_REG, p_iv[1]);
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_IV2_REG, p_iv[2]);
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_IV3_REG, p_iv[3]);
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_IV4_REG, p_iv[4]); /*
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_IV5_REG,  initial_auth_iv[5]);
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_IV6_REG,  initial_auth_iv[6]);
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_IV7_REG,  initial_auth_iv[7]);*/

    // lets start hashing
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_GOPROC_REG, 1 << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_GOPROC_GO_OFFSET);

    for (uint32_t i = 0; i < input_size; i++) {
        HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_DATA_IN_REG, p_din[i]);
    }
    /* last few bytes needs to push to HW */
    uint32_t byte_remain = size & 3;
    if (byte_remain) {
        HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_DATA_IN_REG, p_din[input_size]);
    }
    // Wait for Crypto to finish prossessing
    val = HAL_REG_RD(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_REG);
    // NT_LOG_PRINT(MLM, ERR, "Crypto done:%d\n",val);

    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_SIZE_REG, 0);
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_SIZE_REG, 0);
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SEG_SIZE_REG, 0);
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_START_REG, 0);
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_START_REG, 0);
    final_auth_iv[0] = HAL_REG_RD(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_IV0_REG);
    final_auth_iv[1] = HAL_REG_RD(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_IV1_REG);
    final_auth_iv[2] = HAL_REG_RD(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_IV2_REG);
    final_auth_iv[3] = HAL_REG_RD(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_IV3_REG);
    final_auth_iv[4] = HAL_REG_RD(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_IV4_REG);
    // HAL_DBG_PRINT("Abhishek- output 1\n",final_auth_iv[4],0,0);
    /*final_auth_iv[5] = HAL_REG_RD(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_IV5_REG);
    final_auth_iv[6] = HAL_REG_RD(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_IV6_REG);
    final_auth_iv[7] = HAL_REG_RD(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_IV7_REG);*/
    memscpy(output, 20, (uint8_t *)final_auth_iv, sizeof(final_auth_iv));
    return 0;
}
#else
{
    (void)p_start;
    (void)p_iv;
    (void)size;
    (void)output;
#if 0
	int32_t cnt=0;
	uint32_t read_data;
	uint32_t i;
	uint32_t initial_auth_iv[8], expected_auth_iv[8], actual_auth_iv[8], auth_key[16], encr_key[16], xts_key[4], cntr_iv[4], mask[4],nonce[4];
	uint32_t encr_seg_size, auth_seg_size, encr_seg_start, seg_size, mac_size, input_size, output_size, num_nonce_words, encr_xts_du_size, auth_seg_start;
	uint32_t auth_done = 0;
	uint32_t pass_fail_reg = 0;
	uint32_t din[16], dout[16]={0};
	uint32_t ceId = 0;
	uint32_t crypto_result;

	uint32_t expected_dout[16]={0};

	//declarations
	seg_size = 32;
	encr_seg_size = 0; //32;
	auth_seg_size = 32;//0;
	encr_seg_start = 0;
	auth_seg_start = 0;
	mac_size = 0;
	num_nonce_words = 0;

	input_size = seg_size / 4;
	output_size = (seg_size + mac_size) / 4;


	/*
//For Size =34
 input_size = 9;
 output_size = 9;
seg_size = 34;
auth_seg_size = 34;
	 */

	din[0]  = 0x706D6153;
	din[1]  = 0x6D20656C;
	din[2]  = 0x61737365;
	din[3]  = 0x66206567;
	din[4]  = 0x6B20726F;
	din[5]  = 0x656C7965;
	din[6]  = 0x6C623C6E;
	din[7]  = 0x6C6B636F;
	din[8]  = 0x00006E65;
	din[9]  = 0x00000000;
	din[10] = 0x00000000;
	din[11] = 0x00000000;
	din[12] = 0x00000000;
	din[13] = 0x00000000;
	din[14] = 0x00000000;
	din[15] = 0x00000000;

	expected_dout[0]  = 0x52FA1766;
	expected_dout[1]  = 0xA65145EF;
	expected_dout[2]  = 0x0E5CE23B;
	expected_dout[3]  = 0xAC606D1A;
	expected_dout[4]  = 0xBC484388;
	expected_dout[5]  = 0x3C77D0C4;
	expected_dout[6]  = 0x2569EB59;
	expected_dout[7]  = 0xE7D65900;
	expected_dout[8]  = 0x00000000;
	expected_dout[9]  = 0x00000000;
	expected_dout[10] = 0x00000000;
	expected_dout[11] = 0x00000000;
	expected_dout[12] = 0x00000000;
	expected_dout[13] = 0x00000000;
	expected_dout[14] = 0x00000000;
	expected_dout[15] = 0x00000000;

	expected_auth_iv[0] = 0x3c769cf1;
	expected_auth_iv[1] = 0x9517ca13;
	expected_auth_iv[2] = 0x3eb445a4;
	expected_auth_iv[3] = 0x31f35c44;
	expected_auth_iv[4] = 0xd87a49b6;
	expected_auth_iv[5] = 0x85ae67bb;
	expected_auth_iv[6] = 0x72f36e3c;
	expected_auth_iv[7] = 0x3af54fa5;

	initial_auth_iv[0] = 0x00000000;
	initial_auth_iv[1] = 0x00000000;
	initial_auth_iv[2] = 0x00000000;
	initial_auth_iv[3] = 0x00000000;
	initial_auth_iv[4] = 0x00000000;
	initial_auth_iv[5] = 0x00000000;
	initial_auth_iv[6] = 0x00000000;
	initial_auth_iv[7] = 0x00000000;

	auth_key[0]  = 0x03020100;
	auth_key[1]  = 0x07060504;
	auth_key[2]  = 0x0B0A0908;
	auth_key[3]  = 0x0F0E0D0C;
	auth_key[4]  = 0x13121110;
	auth_key[5]  = 0x17161514;
	auth_key[6]  = 0x1B1A1918;
	auth_key[7]  = 0x1F1E1D1C;
	auth_key[8]  = 0x00000000;
	auth_key[9]  = 0x00000000;
	auth_key[10] = 0x00000000;
	auth_key[11] = 0x00000000;
	auth_key[12] = 0x00000000;
	auth_key[13] = 0x00000000;
	auth_key[14] = 0x00000000;
	auth_key[15] = 0x00000000;

	encr_key[0]  = 0x00000000;
	encr_key[1]  = 0x00000000;
	encr_key[2]  = 0x00000000;
	encr_key[3]  = 0x00000000;
	encr_key[4]  = 0x00000000;
	encr_key[5]  = 0x00000000;
	encr_key[6]  = 0x00000000;
	encr_key[7]  = 0x00000000;
	encr_key[8]  = 0x00000000;
	encr_key[9]  = 0x00000000;
	encr_key[10] = 0x00000000;
	encr_key[11] = 0x00000000;
	encr_key[12] = 0x00000000;
	encr_key[13] = 0x00000000;
	encr_key[14] = 0x00000000;
	encr_key[15] = 0x00000000;

#if 0
	initial_auth_iv[0] = 0x67E6096A;
	initial_auth_iv[1] = 0x85AE67BB;
	initial_auth_iv[2] = 0x72F36E3C;
	initial_auth_iv[3] = 0x3AF54FA5;
	initial_auth_iv[4] = 0x7F520E51;
	initial_auth_iv[5] = 0x8C68059B;
	initial_auth_iv[6] = 0xABD9831F;
	initial_auth_iv[7] = 0x19CDE05B;
#endif

	initial_auth_iv[0] = 0X67452301;
	initial_auth_iv[1] = 0Xefcdab89;
	initial_auth_iv[2] = 0X98badcfe;
	initial_auth_iv[3] = 0X10325476;
	initial_auth_iv[4] = 0Xc3d2e1f0;
	initial_auth_iv[5] = 0x00000000;
	initial_auth_iv[6] = 0x00000000;
	initial_auth_iv[7] = 0x00000000;




	nonce[0] = 0x00000000;
	nonce[1] = 0x00000000;
	nonce[2] = 0x00000000;
	nonce[3] = 0x00000000;

	mask[0] = 0xFFFFFFFF;
	mask[1] = 0xFFFFFFFF;
	mask[2] = 0xFFFFFFFF;
	mask[3] = 0xFFFFFFFF;
#ifdef QCC
	/*
HWIO_OUTF(GCC_GCC_QCC_AHB_BCR_REG,BLK_ARES, 0x1);
while (i<10000)
{i++;}
HWIO_OUTF(GCC_GCC_QCC_AHB_BCR_REG,BLK_ARES, 0x0);
	 */


	ceId = 0;
	//software reset-- all modules will be reset
	HWIO_OUT(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_CORE_CFG_REG,0x2);//use the DMA mode of cryto, disable cpu sw mode

	read_data = HWIO_IN(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_CORE_CFG_REG);
	//vv_msg(SEV_INFO, ST_FUNCTION, "%s *** QCC_CRYPTO_CORE_CFG %08x ***",__FILE__, read_data);
	printf("%s *** QCC_CRYPTO_CORE_CFG %08x *** \n" ,__FILE__, read_data);


	//segment configuration - Hash SHA1
	HWIO_OUT(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_CFG_REG,
			QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_CFG_AUTH_POS_EBEFORE_ENCRYPTION         << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_CFG_AUTH_POS_OFFSET |
			//CRYPTO_AUTH_SEG_CFG__AUTH_SIZE__SHA1                     << HWIO_QCC_CRYPTO_AUTH_SEG_CFG_AUTH_SIZE_SHFT|
			QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_CFG_AUTH_ALG_ESHA                      << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_CFG_AUTH_ALG_OFFSET |
			//CRYPTO_AUTH_SEG_CFG__AUTH_MODE__HASH << HWIO_QCC_CRYPTO_AUTH_SEG_CFG_AUTH_MODE_SHFT |
			1 << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_CFG_FIRST_OFFSET |
			1 << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_CFG_LAST_OFFSET
			// 1 << HWIO_QCC_CRYPTO_AUTH_SEG_CFG_COMP_EXP_MAC_SHFT //CRYPTO_AUTH_SEG_CFG__COMP_EXP_MAC__HASH
	);
	//segment configuration
	HWIO_OUT(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_REG,
			1                                                        << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCODE_OFFSET |
			QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_MODE_ECTR                      << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_MODE_OFFSET|
			QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_ALG_ENONE                       << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_ALG_OFFSET
	);

	//authorization segment configuration
	// formatted A_length + P_length
	HWIO_OUTF(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_SIZE_REG, auth_seg_size);

	//encryption segment configuration
	HWIO_OUTF(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_SIZE_REG, encr_seg_size);

	//total segment size
	// formatted A_length + P_length
	HWIO_OUTF(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SEG_SIZE_REG, seg_size);

	//total segment size
	// formatted A_length + P_length
	//kbp HWIO_OUTF(QCC_CRYPTO_ENCR_SEG_START, ENCR_START, encr_seg_start);
	HWIO_OUTF(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_START_REG, auth_seg_start);

	//write initial IV values
	//for (i=0; i<8; i++) {
	//   HWIO_OUTI(QCC_CRYPTO_AUTH_IVn, (i), initial_auth_iv[i]);
	//}

	//Authentication initial Vector - write counter IV
	//This register does not have an index defined in the address file.
	//for (i=0; i<4; i++) HWIO_OUT(QCC_CRYPTO_ENCR_CNTR0_IV0 + (i*4), cntr_iv[i])
	HWIO_OUT(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_IV0_REG,  initial_auth_iv[0]);
	HWIO_OUT(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_IV1_REG,  initial_auth_iv[1]);
	HWIO_OUT(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_IV2_REG,  initial_auth_iv[2]);
	HWIO_OUT(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_IV3_REG,  initial_auth_iv[3]);
	HWIO_OUT(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_IV4_REG,  initial_auth_iv[0]);
	HWIO_OUT(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_IV5_REG,  initial_auth_iv[1]);
	HWIO_OUT(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_IV6_REG,  initial_auth_iv[2]);
	HWIO_OUT(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_IV7_REG,  initial_auth_iv[3]);

	//Expected MAC values
	/*  HWIO_OUT(QCC_CRYPTO_AUTH_EXP_MAC0, 0x31F48CA2);
   HWIO_OUT(QCC_CRYPTO_AUTH_EXP_MAC1, 0x6A69EE30);
   HWIO_OUT(QCC_CRYPTO_AUTH_EXP_MAC2, 0x374AF198);
   HWIO_OUT(QCC_CRYPTO_AUTH_EXP_MAC3, 0xBC568B67);
   HWIO_OUT(QCC_CRYPTO_AUTH_EXP_MAC4, 0xE5D9BDFC);
   HWIO_OUT(QCC_CRYPTO_AUTH_EXP_MAC5, 0x7F7169CF);
   HWIO_OUT(QCC_CRYPTO_AUTH_EXP_MAC6, 0x0F48F5EC);
   HWIO_OUT(QCC_CRYPTO_AUTH_EXP_MAC7, 0x90F7BD0E);
	 */
	//write Authentication keys
	/*  HWIO_OUT(QCC_CRYPTO_SW_KEY0_0, auth_key[0]);
   HWIO_OUT(QCC_CRYPTO_SW_KEY0_1, auth_key[1]);
   HWIO_OUT(QCC_CRYPTO_SW_KEY0_2, auth_key[2]);
   HWIO_OUT(QCC_CRYPTO_SW_KEY0_3, auth_key[3]);
   HWIO_OUT(QCC_CRYPTO_SW_KEY0_4, auth_key[4]);
   HWIO_OUT(QCC_CRYPTO_SW_KEY0_5, auth_key[5]);
   HWIO_OUT(QCC_CRYPTO_SW_KEY0_6, auth_key[6]);
   HWIO_OUT(QCC_CRYPTO_SW_KEY0_7, auth_key[7]); */

#if 0
	//write AUTH INFO NONCE
	//This register does not have an index defined in the address file.
	HWIO_OUT(QCC_CRYPTO_ENCR_CNTR_MASK, mask[0]);
	HWIO_OUT(QCC_CRYPTO_ENCR_CNTR_MASK0, mask[1]);
	HWIO_OUT(QCC_CRYPTO_ENCR_CNTR_MASK1, mask[2]);
	HWIO_OUT(QCC_CRYPTO_ENCR_CNTR_MASK2, mask[3]);
#endif


	HWIO_OUTF(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_KEY_TABLE_CFG_REG, 0x5);


	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_GOPROC_REG, 1 << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_GOPROC_GO_OFFSET);

#endif

#ifdef NT_FN_REG_MODE
	crypto_result = crypto_arm(din, dout, input_size, output_size, 1000, ceId);
#endif

#ifdef NT_FN_DXE_MODE
	crypto_result =  asicDXECryptoDMA(0, 1, din, dout, input_size/4, output_size/4);
#endif


	HWIO_OUTF(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_SIZE_REG,0);

	//encryption segment configuration
	HWIO_OUTF(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_SIZE_REG, 0);

	//total segment size
	// formatted A_length + P_length
	HWIO_OUTF(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SEG_SIZE_REG, 0);

	//total segment size
	// formatted A_length + P_length

	HWIO_OUTF(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_START_REG,  0);

	HWIO_OUTF(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_START_REG, 0);


	//actual_auth_iv[0] = HWIO_IN(QCC_CRYPTO_AUTH_IV0);
	//actual_auth_iv[1] = HWIO_IN(QCC_CRYPTO_AUTH_IV1);
	//actual_auth_iv[2] = HWIO_IN(QCC_CRYPTO_AUTH_IV2);
	//actual_auth_iv[3] = HWIO_IN(QCC_CRYPTO_AUTH_IV3);
	//actual_auth_iv[4] = HWIO_IN(QCC_CRYPTO_AUTH_IV4);
	//actual_auth_iv[5] = HWIO_IN(QCC_CRYPTO_AUTH_IV5);
	//actual_auth_iv[6] = HWIO_IN(QCC_CRYPTO_AUTH_IV6);
	//actual_auth_iv[7] = HWIO_IN(QCC_CRYPTO_AUTH_IV7);

#endif
    return 0;
}
#endif  // WIFI_HW_SHA_WPA2

#endif  // NT_FN_HW_CRYPTO
