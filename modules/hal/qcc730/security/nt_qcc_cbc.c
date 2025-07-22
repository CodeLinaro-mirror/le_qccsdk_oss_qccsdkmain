/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*****************************************************************************
                                                                    --
-- QUALCOMM Proprietary                                                     --

-- Copyright (c) 1999-2008, 2023-2024  QUALCOMM Incorporated.  All rights reserved.    --

--                                                                          --

-- All data and information contained in or disclosed by this document are  --

-- confidential and proprietary information of QUALCOMM Incorporated, and   --

-- all rights therein are expressly reserved. By accepting this material,   --

-- the recipient agrees that this material and the information contained    --

-- therein are held in confidence and in trust and will not be used,        --

-- copied, reproduced in whole or in part, nor its contents revealed in     --

-- any manner to others without the express written permission of QUALCOMM  --

-- Incorporated.                                                            --

--                                                                          --

-- This technology was exported from the United States in accordance with   --

-- the Export Administration Regulations. Diversion contrary to U.S. law    --

-- prohibited.                                                              --


*****************************************************************************/

/** include files **/

#include "nt_osal.h"
#include "nt_flags.h"
#include "nt_hw.h"
#include "nt_crypto.h"
#include "hal_int_sys.h"
#include "hal_int_modules.h"
#ifdef WIFI_HW_AES
/*------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/
#define AES128_KEY_SIZE_IN_BYTES 16
#define AES256_KEY_SIZE_IN_BYTES 32
#define AES_SW_IO                0x2
#define AES128_CBC_ENCRYPT       0x442
#define AES256_CBC_ENCRYPT       0x452
#define AES128_CBC_DECRYPT       0x42
#define AES256_CBC_DECRYPT       0x52

/*------------------------------------------------------------------------
 * Function Definitions
 * ----------------------------------------------------------------------*/
/**
 * @brief  Driver for configuring HW crypt to CBC
 * @param  key_size : Size of key /8 i.e, 16 for AES 128 and 32 for AES 256
 * @param  mode 	: Encryption or Decryption
 * @param  p_key 	: Pointer to key
 * @param  p_iv 	: Pointer to IV
 * @param  p_in 	: Pointer to in data
 * @param  size 	: Size of Data in
 * @param  p_out 	: Pointer to in data
 * @return Error codes
 */
int32_t hcal_aes_cbc(uint8_t key_size, uint8_t mode, const uint8_t *p_key, const uint8_t *p_iv, const uint8_t *p_in,
                     uint32_t size, uint8_t *p_out)
{
    uint32_t *key = (uint32_t *)p_key;
    uint32_t *iv = (uint32_t *)p_iv;
    uint32_t dout[32] = {0};
    uint8_t secip_power_domain = 0;
    if (!(HAL_REG_RD(QWLAN_PMU_SECIP_GDSCR_REG) & QWLAN_PMU_SECIP_GDSCR_GDS_CTL_PWR_STATUS_MASK)) {
        NT_LOG_PRINT(SECURITY, ERR, "SecIP is power domain is turned off, turning SecIP power domain on locally");
        hal_secip_sw_power_req(1);
        secip_power_domain = 1;
    }
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_CORE_CFG_REG, AES_SW_IO);  // use the SW mode of cryto, disable HW mode
    // NT_LOG_PRINT(SECURITY, ERR, "QWLAN_PMU_SECIP_GDSCR_REG: %d", HAL_REG_RD(QWLAN_PMU_SECIP_GDSCR_REG));
    // NT_LOG_PRINT(SECURITY, ERR, "QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_VERSION_REG: %d",
    // HAL_REG_RD(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_VERSION_REG)); NT_LOG_PRINT(SECURITY, ERR,
    // "QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_CORE_CFG_REG: %d", HAL_REG_RD(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_CORE_CFG_REG));
    if (AES_ENCRYPT == mode) {
        if (AES128_KEY_SIZE_IN_BYTES == key_size)
            HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_REG, AES128_CBC_ENCRYPT);
        else
            HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_REG, AES256_CBC_ENCRYPT);
    } else {
        if (AES128_KEY_SIZE_IN_BYTES == key_size)
            HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_REG, AES128_CBC_DECRYPT);
        else
            HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_REG, AES256_CBC_DECRYPT);
    }
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_SIZE_REG, size);
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_START_REG, 0);
    // total segment size
    //  formatted A_length + P_length
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SEG_SIZE_REG, size);

    // write encryption keys
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SW_KEY0_0_REG, key[0]);
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SW_KEY0_1_REG, key[1]);
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SW_KEY0_2_REG, key[2]);
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SW_KEY0_3_REG, key[3]);
    if (AES256_KEY_SIZE_IN_BYTES == key_size) {  // AES256
        HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SW_KEY0_4_REG, key[4]);
        HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SW_KEY0_5_REG, key[5]);
        HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SW_KEY0_6_REG, key[6]);
        HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SW_KEY0_7_REG, key[7]);
    }
    // write IV vactor
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_CNTR0_IV0_REG, iv[0]);
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_CNTR1_IV1_REG, iv[1]);
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_CNTR2_IV2_REG, iv[2]);
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_CNTR3_IV3_REG, iv[3]);

    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_GOPROC_REG, 0x1);

    if (size & 0x3) {
        size += 4;
    }
    crypto_arm3((const uint8_t *)p_in, dout, (uint32_t)(size >> 2), (uint32_t)(size >> 2));
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_REG, 0);
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_SIZE_REG, 0);
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_START_REG, 0);
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SEG_SIZE_REG, 0);
    if (1 == secip_power_domain) {
        hal_secip_sw_power_req(1);
    }

    memcpy(p_out, &dout, size * sizeof(uint8_t));
    return 0;
}

#endif
#ifdef NT_FN_HW_CRYPTO
#define QCC 1

#if 0 /* Disabling unused function to avoid KW issues. If this function needs to be used in future, KW issues need to \
         be addressed */
int32_t aes128_cbc_known_answer_piomode(int mode, const unsigned char iv[16], const unsigned char *input,const uint32_t key[8],unsigned char *output)
{
  //uint32_t i;
  uint32_t encr_key[16], cntr_iv[4], nonce[4];
  uint32_t encr_seg_size, encr_seg_start, seg_size, mac_size, input_size, output_size;
  uint32_t din[16], dout[16]={0};
  //uint32_t expected_dout[16]={0};
  uint32_t ceId = 0;
  //uint32_t crypto_result;

  //declarations
  seg_size = 64;
  encr_seg_size = 64;
  encr_seg_start = 0;
  mac_size = 0;
  input_size = seg_size/4;
  output_size = (seg_size + mac_size)/4;


  // copy input
  	memcpy( &din[0], (uint32_t *)(&input), sizeof(din));

  	// copy key
  	for(uint8_t i = 0 ; i < 8; i++) {
  		encr_key[i] = key[i];
  	}

  	//copy counter iv
  	memcpy( &cntr_iv[0], (uint32_t *)(&iv[0]), sizeof(cntr_iv));

  nonce[0] = 0x00000000;
  nonce[1] = 0x00000000;
  nonce[2] = 0x00000000;
  nonce[3] = 0x00000000;

#ifdef QCC
  ceId = 0;
  //software reset-- all modules will be reset
  HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_CORE_CFG_REG,0x2);//enable the PIO mode of cryto, use cpu sw mode

  if(mode == encrypt) {
  		HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_REG,
  				1                                                        			<< QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCODE_OFFSET |
  				QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_MODE_ECBC 		<< QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_MODE_OFFSET|
  				//QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_KEY_SZ_EAES256 	<< QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_KEY_SZ_OFFSET |
  				QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_ALG_EAES        << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_ALG_OFFSET
  		);
  	}
  	else if (mode == decrypt) {
  		HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_REG,
  				0                                                        			<< QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCODE_OFFSET |
  				QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_MODE_ECBC 		<< QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_MODE_OFFSET|
  				//QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_KEY_SZ_EAES256 	<< QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_KEY_SZ_OFFSET |
  				QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_ALG_EAES        << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_ALG_OFFSET
  		);
  	}

  //authorization segment configuration
  // formatted A_length + P_length
  //HAL_REG_WRF(QCC_CRYPTO_AUTH_SEG_SIZE, AUTH_SIZE, auth_seg_size);

  //encryption segment configuration
  HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_SIZE_REG, encr_seg_size);

  //total segment size
  // formatted A_length + P_length
  HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SEG_SIZE_REG, seg_size);

  //total segment size
  // formatted A_length + P_length
  HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_START_REG, encr_seg_start);


  //write encryption keys

   HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SW_KEY0_0_REG, encr_key[0]);
   HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SW_KEY0_1_REG, encr_key[1]);
   HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SW_KEY0_2_REG, encr_key[2]);
   HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SW_KEY0_3_REG, encr_key[3]);

   //write counter IV
  //This register does not have an index defined in the address file.
   HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_CNTR0_IV0_REG,  cntr_iv[0]);
   HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_CNTR1_IV1_REG,  cntr_iv[1]);
   HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_CNTR2_IV2_REG,  cntr_iv[2]);
   HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_CNTR3_IV3_REG,  cntr_iv[3]);

  //write AUTH INFO NONCE
  //This register does not have an index defined in the address file.
   HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_INFO_NONCE0_REG, nonce[0]);
   	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_INFO_NONCE1_REG, nonce[1]);
   	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_INFO_NONCE2_REG, nonce[2]);
   	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_INFO_NONCE3_REG, nonce[3]);

#endif

	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_GOPROC_REG, 1 << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_GOPROC_GO_OFFSET);

  //HAL_DBG_PRINT("*** Starting crypto_arm *** \n",0,0,0);

    nt_qcc_register_transfer(din, dout, input_size, output_size, 500, ceId);


  //HAL_DBG_PRINT("*** crypto data dma mode finished *** \n",0,0,0);

  memcpy( &output, (uint8_t *)(&dout[0]), sizeof(dout));


return 0;
}

#endif

#if 0
int32_t nt_aes256_cbc(int mode,
        size_t length,
        unsigned char iv[16],
        const unsigned char *din,
        unsigned char *dout)

{

	int32_t cnt=0;
	uint32_t read_data;
	uint32_t i;
	uint32_t initial_auth_iv[8], expected_auth_iv[8], actual_auth_iv[8], auth_key[16], encr_key[16], xts_key[4], cntr_iv[4], nonce[4];
	uint32_t encr_seg_size, auth_seg_size, encr_seg_start, seg_size, mac_size, input_size, output_size, num_nonce_words, encr_xts_du_size;
	uint32_t auth_done = 0;
	uint32_t pass_fail_reg = 0;
	//uint32_t din[16], dout[16]={0};
	uint32_t expected_dout[16]={0};
	uint32_t ceId = 0;
	uint32_t crypto_result;

	//declarations
	seg_size = 64;
	encr_seg_size = 64;
	auth_seg_size = 0;
	encr_seg_start = 0;
	mac_size = 0;
	num_nonce_words = 0;
	input_size = seg_size/4;
	output_size = seg_size + mac_size/4;


	initial_auth_iv[0] = 0x00000000;
	initial_auth_iv[1] = 0x00000000;
	initial_auth_iv[2] = 0x00000000;
	initial_auth_iv[3] = 0x00000000;
	initial_auth_iv[4] = 0x00000000;
	initial_auth_iv[5] = 0x00000000;
	initial_auth_iv[6] = 0x00000000;
	initial_auth_iv[7] = 0x00000000;

	cntr_iv[0] = 0x00010203;
	cntr_iv[1] = 0x04050607;
	cntr_iv[2] = 0x08090A0B;
	cntr_iv[3] = 0x0C0D0E0F;

	nonce[0] = 0x00000000;
	nonce[1] = 0x00000000;
	nonce[2] = 0x00000000;
	nonce[3] = 0x00000000;

#ifdef QCC

	ceId = 0;


	//software reset-- all modules will be reset
	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_CORE_CFG_REG,0x2);//enable the PIO mode of cryto, use cpu sw mode

	read_data = HAL_REG_RD(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_CORE_CFG_REG);

	//segment configuration

	HWIO_OUT(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_CFG_REG,
			QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_CFG_AUTH_POS_EBEFORE_ENCRYPTION         << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_CFG_AUTH_POS_OFFSET |
//			CRYPTO_AUTH_SEG_CFG__AUTH_SIZE__SHA1                     << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_CFG_AUTH_SIZE_OFFSET|
			QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_CFG_AUTH_ALG_ENONE                      << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_CFG_AUTH_ALG_OFFSET
	);


	//segment configuration
	if(mode == encrypt) {
		HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_REG,
				1                                                        			<< QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCODE_OFFSET |
				QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_MODE_ECBC 		<< QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_MODE_OFFSET|
				QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_KEY_SZ_EAES256 	<< QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_KEY_SZ_OFFSET |
				QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_ALG_EAES        << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_ALG_OFFSET
		);
	}
	else if (mode == decrypt) {
		HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_REG,
				0                                                        			<< QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCODE_OFFSET |
				QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_MODE_ECBC 		<< QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_MODE_OFFSET|
				QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_KEY_SZ_EAES256 	<< QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_KEY_SZ_OFFSET |
				QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_ALG_EAES        << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_ALG_OFFSET
		);
	}



	//authorization segment configuration
	// formatted A_length + P_length
	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_SIZE_REG, auth_seg_size);


	//encryption segment configuration
	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_SIZE_REG, encr_seg_size);

	//total segment size
	// formatted A_length + P_length
	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SEG_SIZE_REG, seg_size);

	//total segment size
	// formatted A_length + P_length
	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_START_REG, encr_seg_start);


	//write initial IV values
	//for (i=0; i<8; i++) {
	//   HWIO_OUTI(QCC_CRYPTO_AUTH_IVn, (i), initial_auth_iv[i]);
	//}

	//key table
	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_KEY_TABLE_CFG_REG, 0x5);

	//write counter IV
	//This register does not have an index defined in the address file.
	//for (i=0; i<4; i++) HWIO_OUT(QCC_CRYPTO_ENCR_CNTR0_IV0 + (i*4), cntr_iv[i])
	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_CNTR0_IV0_REG,  iv[0]);
	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_CNTR1_IV1_REG,  iv[1]);
	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_CNTR2_IV2_REG,  iv[2]);
	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_CNTR3_IV3_REG,  iv[3]);

	//write AUTH INFO NONCE
	//This register does not have an index defined in the address file.
	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_INFO_NONCE0_REG, nonce[0]);
	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_INFO_NONCE1_REG, nonce[1]);
	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_INFO_NONCE2_REG, nonce[2]);
	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_INFO_NONCE3_REG, nonce[3]);

#endif

	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_GOPROC_REG, 1 << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_GOPROC_GO_OFFSET);


	//vv_msg(SEV_INFO, ST_FUNCTION, "*** Starting crypto_arm ***");
	crypto_result = crypto_arm2(din, dout, input_size, output_size, 500, ceId);
	// vv_msg(SEV_INFO, ST_FUNCTION, "*** crypto data dma mode finished ***");


	//vv_msg(SEV_INFO, ST_FUNCTION, "*** Starting crypto_arm1 ***");
	//crypto_result = crypto_arm(din, dout, input_size, output_size, 500, ceId);
	//vv_msg(SEV_INFO, ST_FUNCTION, "*** crypto data dma mode finished ***");


	return 0;

}

#endif

#endif
