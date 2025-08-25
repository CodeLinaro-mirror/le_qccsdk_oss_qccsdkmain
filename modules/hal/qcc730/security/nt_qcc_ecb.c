/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*****************************************************************************
------------------------------------------------------------------------------
------------------------------------------------------------------------------
--                                                                          --
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
--                                                                          --
------------------------------------------------------------------------------

*****************************************************************************/

/** include files **/

#include "nt_flags.h"
#include "nt_hw.h"
#include "nt_crypto.h"
#include "hal_int_sys.h"
#include "hal_int_modules.h"
#include "nt_logger_api.h"
/*------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/
#ifdef WIFI_HW_AES
#define AES128_KEY_SIZE_IN_BYTES 16
#define AES256_KEY_SIZE_IN_BYTES 32
#define AES_SW_IO                0x2
#define AES128_ECB_ENCRYPT       0x402
#define AES256_ECB_ENCRYPT       0x412
#define AES128_ECB_DECRYPT       0x02
#define AES256_ECB_DECRYPT       0x12
/*------------------------------------------------------------------------
 * Function Definitions
 * ----------------------------------------------------------------------*/
/**
 * @brief  Driver for configuring HW crypt to CBC
 * @param  key_size : Size of key /8 i.e, 16 for AES 128 and 32 for AES 256
 * @param  mode 	: Encryption or Decryption
 * @param  p_key 	: Pointer to key
 * @param  p_in 	: Pointer to in data
 * @param  size 	: Size of Data in
 * @param  p_out 	: Pointer to in data
 * @return Error codes
 */
int32_t hcal_aes_ecb(uint8_t key_size, uint8_t mode, const uint8_t *p_key, const uint8_t *p_in, uint32_t size,
                     uint8_t *p_out)
{
    uint32_t *key = (uint32_t *)p_key;
    uint32_t dout[32] = {0};
    uint8_t secip_power_domain = 0;
    if (!(HAL_REG_RD(QWLAN_PMU_SECIP_GDSCR_REG) & QWLAN_PMU_SECIP_GDSCR_GDS_CTL_PWR_STATUS_MASK)) {
        // NT_LOG_PRINT(SECURITY,ERR, "SecIP is power domain is turned off, turning SecIP power domain on locally");
        hal_secip_sw_power_req(1);
        secip_power_domain = 1;
    }
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_CORE_CFG_REG, AES_SW_IO);  // use the SW mode of cryto, disable HW mode
    // NT_LOG_PRINT(MLM, ERR, "QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_VERSION_REG: %d",
    // HAL_REG_RD(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_VERSION_REG)); NT_LOG_PRINT(MLM, ERR,
    // "QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_CORE_CFG_REG: %d", HAL_REG_RD(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_CORE_CFG_REG));
    if (AES_ENCRYPT == mode) {
        if (AES128_KEY_SIZE_IN_BYTES == key_size)
            HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_REG, AES128_ECB_ENCRYPT);
        else
            HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_REG, AES256_ECB_ENCRYPT);
    } else {
        if (AES128_KEY_SIZE_IN_BYTES == key_size)
            HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_REG, AES128_ECB_DECRYPT);
        else
            HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_REG, AES256_ECB_DECRYPT);
    }

    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_SIZE_REG, size);
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_START_REG, 0);
    // total segment size
    // formatted A_length + P_length
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
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_GOPROC_REG, 0x1);  // TODO::RESET

    if (size & 0x3) {
        size += 4;
    }
    crypto_arm3((const uint8_t *)p_in, dout, (uint32_t)(size >> 2), (uint32_t)(size >> 2));
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_REG, 0);
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_SIZE_REG, 0);
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_START_REG, 0);
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SEG_SIZE_REG, 0);
    if (1 == secip_power_domain) {
        hal_secip_sw_power_req(0);
    }
    memcpy(p_out, &dout, size * sizeof(uint8_t));
    return 0;
}
#endif
#ifdef NT_FN_HW_CRYPTO

#define QCC 1

extern uint8_t data;
//#define IRQ_ENABLE_1 1

#if 0


int32_t nt_aes256_ecb(uint8_t mode, const unsigned char din[16], unsigned char dout[16])
{
	//uint32_t initial_auth_iv[8], expected_auth_iv[8], actual_auth_iv[8], auth_key[16], encr_key[16], xts_key[8], cntr_iv[4], nonce[4];
	uint32_t encr_seg_size, /*auth_seg_size,*/ encr_seg_start, seg_size, mac_size, input_size, output_size /*, num_nonce_words, encr_xts_du_size*/;
	uint32_t ceId = 0;
	uint32_t crypto_result;
	//uint32_t use_hw_key = 0;

	//declarations
	seg_size = 32;
	encr_seg_size = seg_size;
	//auth_seg_size = 0;
	encr_seg_start = 0;
	mac_size = 0;
	//num_nonce_words = 0;
	//encr_xts_du_size = seg_size;
	input_size = seg_size;
	output_size = (seg_size + mac_size);


	ceId=0;
	//software reset-- all modules will be reset
	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_CORE_CFG_REG,0x2);


	//segment configuration
	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_CFG_REG,0 );

	if(mode == encrypt) {
		HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_REG,
				1                                                        			<< QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCODE_OFFSET |
				QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_MODE_EECB 		<< QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_MODE_OFFSET|
				QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_KEY_SZ_EAES256 	<< QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_KEY_SZ_OFFSET |
				QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_ALG_EAES        << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_ALG_OFFSET
		);
	}
	else if (mode == decrypt) {
		HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_REG,
				0                                                        			<< QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCODE_OFFSET |
				QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_MODE_EECB 		<< QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_MODE_OFFSET|
				QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_KEY_SZ_EAES256 	<< QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_KEY_SZ_OFFSET |
				QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_ALG_EAES        << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_ALG_OFFSET
		);
	}

	//authorization segment configuration
	// formatted A_length + P_length

	//encryption segment configuration
	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_SIZE_REG, encr_seg_size);


	//total segment size
	// formatted A_length + P_length
	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SEG_SIZE_REG, seg_size);

	//total segment size
	// formatted A_length + P_length
	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_START_REG, encr_seg_start);


	//HWIO_OUT(QCC_CRYPTO_GOPROC, 1 << HWIO_QCC_CRYPTO_GOPROC_GO_SHFT);
	//HWIO_OUT(QCC_CRYPTO_GOPROC, 0x1);

	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_KEY_TABLE_CFG_REG, 0x5);

	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_GOPROC_REG, 0x1); // Uncommented for asicDXECryptoDMA

	if(data == HCAL_REGISTER_MODE) {
		crypto_result = crypto_arm((uint32_t *) din, (uint32_t *)dout, input_size, output_size, 500, ceId);
		if (crypto_result != 0) {

		}
	}
	//  crypto_result = crypto_arm(din, dout, input_size, output_size, 1000, ceId);

	else if (data == HCAL_DXE_MODE ) {
		crypto_result =  asicDXECryptoDMA(0, 1, din[0], dout[0], input_size/4, output_size/4);
		if (crypto_result != 0) {

		}

	}

	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_SIZE_REG, 0);

	//encryption segment configuration
	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_SIZE_REG, 0);

	//total segment size
	// formatted A_length + P_length
	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SEG_SIZE_REG, 0);

	//total segment size
	// formatted A_length + P_length
	//kbp HWIO_OUTF(QCC_CRYPTO_ENCR_SEG_START, ENCR_START, encr_seg_start);
	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_START_REG, 0);

	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_CFG_REG,0);

	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_REG,0);

	//print output

	return 0;
}


int32_t nt_qcc_aes128_ecb(uint8_t mode, const unsigned char din[16], const uint32_t key[8], unsigned char dout[16])
{

	uint32_t encr_key[16], cntr_iv[4], nonce[4];
	uint32_t encr_seg_size, encr_seg_start, seg_size, mac_size, input_size, output_size;
	uint32_t lcdin[4], lcdout[4] = {0};
	uint32_t ceId = 0;
	uint32_t crypto_result;


	//declarations
	seg_size = 16;
	encr_seg_size = 16;
	encr_seg_start = 0;
	mac_size = 0;
	input_size = seg_size/4;
	output_size = (seg_size + mac_size)/4;

	// copy input
	memcpy( &lcdin[0], (uint32_t *)(&din[0]), sizeof(lcdin));

	// copy key
	for(uint8_t i = 0 ; i < 8; i++) {
		encr_key[i] = key[i];
	}

	cntr_iv[0] = 0x00000000;
	cntr_iv[1] = 0x00000000;
	cntr_iv[2] = 0x00000000;
	cntr_iv[3] = 0x00000000;

	nonce[0] = 0x00000000;
	nonce[1] = 0x00000000;
	nonce[2] = 0x00000000;
	nonce[3] = 0x00000000;

	ceId = 0;

	HAL_REG_WR( QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_CORE_CFG_REG, 0x2 );

	if(mode == encrypt) {
		//HAL_DBG_PRINT("encrypt",0,0,0);
		HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_REG,
				1                                                        			<< QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCODE_OFFSET |
				QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_MODE_EECB 		<< QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_MODE_OFFSET|
				QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_KEY_SZ_EAES128 	<< QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_KEY_SZ_OFFSET |
				QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_ALG_EAES        << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_ALG_OFFSET
		);
	}
	else if (mode == decrypt) {
		//HAL_DBG_PRINT("decrypt",0,0,0);
		HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_REG,
				0                                                        			<< QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCODE_OFFSET |
				QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_MODE_EECB 		<< QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_MODE_OFFSET|
				QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_KEY_SZ_EAES128 	<< QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_KEY_SZ_OFFSET |
				QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_ALG_EAES        << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_ALG_OFFSET
		);
	}
	else
	{
		// do nothing
		HAL_DBG_PRINT("No mode found",0,0,0);
	}

	HAL_REG_WR( QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_SIZE_REG, encr_seg_size );

	HAL_REG_WR( QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SEG_SIZE_REG, seg_size );

	HAL_REG_WR( QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_START_REG, encr_seg_start );

	HAL_REG_WR( QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SW_KEY0_0_REG, encr_key[0] );
	HAL_REG_WR( QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SW_KEY0_1_REG, encr_key[1] );
	HAL_REG_WR( QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SW_KEY0_2_REG, encr_key[2] );
	HAL_REG_WR( QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SW_KEY0_3_REG, encr_key[3] );
	HAL_REG_WR( QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SW_KEY0_4_REG, encr_key[4] );
	HAL_REG_WR( QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SW_KEY0_5_REG, encr_key[5] );
	HAL_REG_WR( QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SW_KEY0_6_REG, encr_key[6] );
	HAL_REG_WR( QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SW_KEY0_7_REG, encr_key[7] );

	HAL_REG_WR( QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_CNTR0_IV0_REG, cntr_iv[0] );
	HAL_REG_WR( QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_CNTR1_IV1_REG, cntr_iv[1] );
	HAL_REG_WR( QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_CNTR2_IV2_REG, cntr_iv[2] );
	HAL_REG_WR( QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_CNTR3_IV3_REG, cntr_iv[3] );

	HAL_REG_WR( QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_INFO_NONCE0_REG, nonce[0] );
	HAL_REG_WR( QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_INFO_NONCE1_REG, nonce[1] );
	HAL_REG_WR( QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_INFO_NONCE2_REG, nonce[2] );
	HAL_REG_WR( QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_INFO_NONCE3_REG, nonce[3] );

	HAL_REG_WR( QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_GOPROC_REG, 0x1 );


	if(data == HCAL_REGISTER_MODE) {
		crypto_result = nt_qcc_register_transfer( lcdin, lcdout, input_size, output_size, 500, ceId);
		if (crypto_result != 0) {

		}
	}

	else if (data == HCAL_DXE_MODE ) {
		crypto_result = nt_qcc_dxe_transfer(DXE_CHANNEL_8, DXE_CHANNEL_9, (uint32_t)lcdin, (uint32_t)lcdout, input_size, output_size);
		if (crypto_result != 0) {

		}
	}

	memcpy( &dout[0], (uint8_t *)(&lcdout[0]), sizeof(lcdout));

	HAL_REG_WR( QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_SIZE_REG, 0 );

	HAL_REG_WR( QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SEG_SIZE_REG, 0 );

	HAL_REG_WR( QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_START_REG, 0 );


	return 0;
}
#endif
/*
void irq_handler(void)
{
  uint32_t read_data, i;

  interrupt_count++;

  // Read interrupt register.
  read_data = INP32(HWIO_APCS_GICC_IAR_ADDR);
  irq_handler_executed = read_data;
  vv_msg(SEV_INFO, ST_FUNCTION, "*** Interrupt %1d APCS_GICC_IAR %08x ***", interrupt_count, read_data);

  if (irq_handler_executed) {
    // Interrupt hasn't been cleared by the main program.
  }

  HWIO_OUT(APCS_GICC_EOIR, read_data);

}
*/

#endif
