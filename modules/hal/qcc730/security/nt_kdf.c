/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "nt_flags.h"
#include "nt_crypto.h"
#include "nt_hw.h"
#include "hal_int_sys.h"

#if 0
//dma mode for data out

uint32_t crypto_arm(uint32_t ce_data_in[], uint32_t ce_data_out[], uint32_t din_limit, uint32_t dout_limit, uint32_t timeout, uint32_t ceId)

{

	//Crypto FIFO is only 32 bytes in size
	int32_t i, j, val;

	uint32_t crypto_status=0, din_size_avail=0, dout_size_avail=0, din_rdy=0, dout_rdy=0, operation_done=0, din_cnt=0, dout_cnt=0;

	uint32_t return_status = 0;


	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_DATA_IN_BUF_BASE_REG, DATA_IN_BASE_ADDRESS);

	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_DATA_IN_BUF_LEN_REG, din_limit*4  );

	//HWIO_OUT(QCC_CRYPTO_DATA_IN_BUF_LEN, seg_size  );

	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_DATA_OUT_BUF_BASE_REG, DATA_OUT_BASE_ADDRESS);



	for(i = 0; i < din_limit; i++) {
		//init the data in buffer
		DATA_IN(i) = 0x00000000;
	}

	for(i = 0; i < din_limit; i++) {
		//init the data out buffer
		DATA_OUT(i) = 0x00000000;
	}

	if (ceId == 0) {
		for(i = 0; i < din_limit; i++) {
			//write  data in buffer in
			DATA_IN(i) = ce_data_in[i];
			//HWIO_OUT(QCC_CRYPTO_DATA_IN, ce_data_in[i]); //Sivaiah
		}

		// HWIO_OUT(QCC_CRYPTO_GOPROC, 1 << HWIO_QCC_CRYPTO_GOPROC_GO_SHFT);

		//Begin Process
			val = HAL_REG_RD (QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_GOPROC_REG);
			val |= ( 1 << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_GOPROC_GO_OFFSET);
			HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_GOPROC_REG, val);

		//Wait for Crypto to finish prossessing

		while( !(HWIO_INF(QCC_CRYPTO_STATUS, OPERATION_DONE)));

		dout_limit = HWIO_IN(QCC_CRYPTO_DATA_OUT_BUF_LEN);


		for(j = 0; j < dout_limit; j++) {

			ce_data_out[j] = DATA_OUT(j);
			//kbp vv_msg(SEV_INFO, ST_FUNCTION, "*** the num:%1d data out %08x expected out %08x ***", j, ce_data_out[j], expected_dout[j]);

			//vv_msg(SEV_INFO, ST_FUNCTION, "*** the num:%1d data out %08x  ***", i + j, ce_data_out[i + j]);
		}

	}

	return 0;

}




int32_t kdf_op9_aes128_ecb(void)
{
	int32_t cnt=0;
	uint32_t read_data, write_data;
	uint32_t i;
	uint32_t initial_auth_iv[8], expected_auth_iv[8], actual_auth_iv[8], auth_key[16], encr_key[16], xts_key[8], cntr_iv[4], nonce[4];
	uint32_t encr_seg_size, auth_seg_size, encr_seg_start, seg_size, mac_size, input_size, output_size, num_nonce_words, encr_xts_du_size;
	uint32_t auth_done = 0;
	uint32_t pass_fail_reg = 0;
	uint32_t din[8], dout[8]={0}, expected_dout[8]={0};
	uint32_t ceId = 0;
	uint32_t crypto_result;
	uint32_t use_hw_key = 0;

	//declarations
	seg_size = 32;
	encr_seg_size = seg_size;
	auth_seg_size = 0;
	encr_seg_start = 0;
	mac_size = 0;
	num_nonce_words = 0;
	encr_xts_du_size = seg_size;
	input_size = seg_size;
	output_size = seg_size + mac_size;

	din[0] = 0x67452301;
	din[1] = 0xefcdab89;
	din[2] = 0x98badcfe;
	din[3] = 0x10325476;
	din[4] = 0x67452301;
	din[5] = 0xefcdab89;
	din[6] = 0x98badcfe;
	din[7] = 0x10325476;

	/* 1.x
  expected_dout[0] = 0x49ca251a;
  expected_dout[1] = 0xdd4e7d95;
  expected_dout[2] = 0x9668343e;
  expected_dout[3] = 0x3ce9a253;
  expected_dout[4] = 0x49ca251a;
  expected_dout[5] = 0xdd4e7d95;
  expected_dout[6] = 0x9668343e;
  expected_dout[7] = 0x3ce9a253;
	 */

	expected_dout[0] = 0x3acaf6b8;
	expected_dout[1] = 0xf280689;
	expected_dout[2] = 0x39269e3c;
	expected_dout[3] = 0x9ea760a1;
	expected_dout[4] = 0x3acaf6b8;
	expected_dout[5] = 0xf280689;
	expected_dout[6] = 0x39269e3c;
	expected_dout[7] = 0x9ea760a1;

	cntr_iv[0] = 0x00000000;
	cntr_iv[1] = 0x00000000;
	cntr_iv[2] = 0x00000000;
	cntr_iv[3] = 0x000000ff;

	nonce[0] = 0x00000000;
	nonce[1] = 0x00000000;
	nonce[2] = 0x00000000;
	nonce[3] = 0x00000000;

	ceId=0;
	//software reset-- all modules will be reset
	HWIO_OUT(QCC_CRYPTO_CORE_CFG,0x2);

	//segment configuration
	HWIO_OUT(QCC_CRYPTO_ENCR_SEG_CFG,
			1                                                        << HWIO_QCC_CRYPTO_ENCR_SEG_CFG_ENCODE_SHFT |
			CRYPTO_ENCR_SEG_CFG__ENCR_MODE__ECB                      << HWIO_QCC_CRYPTO_ENCR_SEG_CFG_ENCR_MODE_SHFT|
			CRYPTO_ENCR_SEG_CFG__ENCR_KEY_SZ__AES128 << HWIO_QCC_CRYPTO_ENCR_SEG_CFG_ENCR_KEY_SZ_SHFT |
			CRYPTO_ENCR_SEG_CFG__ENCR_ALG__AES                       << HWIO_QCC_CRYPTO_ENCR_SEG_CFG_ENCR_ALG_SHFT
	);

	//authorization segment configuration
	// formatted A_length + P_length

	//encryption segment configuration
	HWIO_OUTF(QCC_CRYPTO_ENCR_SEG_SIZE, ENCR_SIZE, encr_seg_size);


	//total segment size
	// formatted A_length + P_length
	HWIO_OUTF(QCC_CRYPTO_SEG_SIZE, SEG_SIZE, seg_size);

	//total segment size
	// formatted A_length + P_length
	HWIO_OUTF(QCC_CRYPTO_ENCR_SEG_START, ENCR_START, encr_seg_start);

	HWIO_OUT(QCC_CRYPTO_KEY_TABLE_CFG, 0x5);
	//vv_msg(SEV_INFO, ST_FUNCTION, "*** Starting crypto_arm2 ***");
	// crypto_result = crypto_arm(din, dout, input_size/4, output_size/4, 1000, ceId);

	HWIO_OUT(QCC_CRYPTO_GOPROC, 0x1); // Uncommented for asicDXECryptoDMA
	crypto_result =  asicDXECryptoDMA(0, 1, din, dout, input_size/4, output_size/4);

	//vv_msg(SEV_INFO, ST_FUNCTION, "*** crypto_arm2 finished ***");
	for(i=0;i<=7;i++)
		printf("dout : 0x%x expected_dout : 0x%x \n",dout[i],expected_dout[i]);
	//check for proper cipher output

	if (memcmp(dout, expected_dout, output_size/4)) {
		//    vv_test_fail();
		printf("Test Fail \n");
		return 1;
	} else if (crypto_result >> 31) {
		//    vv_msg(SEV_INFO, ST_FUNCTION, "*** crypto_arm2 timed out ***");
		//    vv_test_fail();
		printf("Test Fail \n");
		return 1;
	} else {
		//vv_test_pass();
		printf("Test Pass \n");
		return 0;
	}
}

#endif

#ifdef NT_FN_HW_CRYPTO

int lock_wrapper_req(void)
{
    uint32 data;

    HAL_DBG_PRINT("Lock wrapper request \n", 0, 0, 0);
    HAL_REG_WR(QWLAN_LOCK_WRAPPER_R_LOCK_REQUEST_REG_REG, 0x1);

    data = HAL_REG_RD(QWLAN_LOCK_WRAPPER_R_LOCK_STATUS_REG_REG);

    while (data != 0x80000004)
        data = HAL_REG_RD(QWLAN_LOCK_WRAPPER_R_LOCK_STATUS_REG_REG);
    return 0;
}

int lock_wrapper_release(void)
{
    HAL_DBG_PRINT("Lock wrapper release \n", 0, 0, 0);
    HAL_REG_WR(QWLAN_LOCK_WRAPPER_R_LOCK_RELEASE_REG_REG, 0x1);
    return 0;
}

int kdf_op9_operation(void)
{
    lock_wrapper_req();
#if 0
	oem_op9();
#endif
    // otp_qc_op9();

    HAL_REG_WR(QWLAN_KDF_CSR_R_INTR_EN_REG, 0x30000);

    HAL_REG_WR(QWLAN_KDF_CSR_R_OP_CODE_REG, 0x9);

    HAL_REG_WR(QWLAN_KDF_CSR_R_INPUT_0_REG, 0x01234567);

    HAL_REG_WR(QWLAN_KDF_CSR_R_INPUT_1_REG, 0X89abcdef);

    HAL_REG_WR(QWLAN_KDF_CSR_R_INPUT_2_REG, 0Xfedcba98);

    HAL_REG_WR(QWLAN_KDF_CSR_R_INPUT_3_REG, 0X76543210);

    HAL_REG_WR(QWLAN_KDF_CSR_R_OP_GO_REG, 0x1);

    HAL_DBG_PRINT("Before KDF_OP_STATUS \n", 0, 0, 0);
    while (HAL_REG_RD(QWLAN_KDF_CSR_R_OP_STATUS_REG) != 0x1)
        ;

    HAL_DBG_PRINT("After KDF_OP_STATUS \n", 0, 0, 0);

    lock_wrapper_release();

    return 0;
}

#endif
