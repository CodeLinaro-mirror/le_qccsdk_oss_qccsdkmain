/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#if 0

/*****************************************************************************

------------------------------------------------------------------------------

------------------------------------------------------------------------------

--                                                                          --

-- QUALCOMM Proprietary                                                     --

-- Copyright (c) 1999-2008  QUALCOMM Incorporated.  All rights reserved.    --

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



/* include files **/
#include "nt_flags.h"
#include "nt_hw.h"
#include "nt_crypto.h"
#include "hal_int_sys.h"

#define QCC 1


int32_t nt_aes128_ctr(uint8_t mode, unsigned char nonce_counter[16], const unsigned char din,
        unsigned char *dout)

{

	//uint32_t actual_auth_iv[8], encr_key[16], xts_key[4];
	uint32_t cntr_iv[4], mask[4],nonce[4];
	uint32_t encr_seg_size, auth_seg_size, encr_seg_start, seg_size, mac_size, input_size, output_size, num_nonce_words;
	//uint32_t din[16], dout[16]={0};
	uint32_t ceId = 0;
	uint32_t crypto_result;


	//declarations
	seg_size = 32;
	encr_seg_size = 32;
	auth_seg_size = 0;
	encr_seg_start = 0;
	mac_size = 0;
	num_nonce_words = 0;
	//input_size = seg_size / 4;
	//output_size = (seg_size + mac_size) / 4;
	input_size = seg_size;
	output_size = (seg_size + mac_size);

	//  encr_key[0]  = 0xAD575D4B;
	//  encr_key[1]  = 0x73FD2878;
	//  encr_key[2]  = 0x1685E4F8;
	//  encr_key[3]  = 0x2BDBA242;
	//  encr_key[4]  = 0x00000000;
	//  encr_key[5]  = 0x00000000;
	//  encr_key[6]  = 0x00000000;
	//  encr_key[7]  = 0x00000000;
	//  encr_key[8]  = 0x00000000;
	//  encr_key[9]  = 0x00000000;
	//  encr_key[10] = 0x00000000;
	//  encr_key[11] = 0x00000000;
	//  encr_key[12] = 0x00000000;
	//  encr_key[13] = 0x00000000;
	//  encr_key[14] = 0x00000000;
	//  encr_key[15] = 0x00000000;


	cntr_iv[0] = 0x8ED01645;
	cntr_iv[1] = 0x80A1C637;
	cntr_iv[2] = 0xD4363813;
	cntr_iv[3] = 0x13DB520A;


	nonce[0] = 0x00000000;
	nonce[1] = 0x00000000;
	nonce[2] = 0x00000000;
	nonce[3] = 0x00000000;

	mask[0] = 0xFFFFFFFF;
	mask[1] = 0xFFFFFFFF;
	mask[2] = 0xFFFFFFFF;
	mask[3] = 0xFFFFFFFF;
#ifdef QCC
	ceId = 0;

	//software reset-- all modules will be reset
	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_CORE_CFG_REG,0x2);//enable the DMA mode of cryto

	read_data = HAL_REG_RD(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_CORE_CFG_REG);

	//auth  configuration

	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_CFG_REG,

			QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_CFG_AUTH_POS_EBEFORE_ENCRYPTION	<< QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_CFG_AUTH_POS_OFFSET |

			//CRYPTO_AUTH_SEG_CFG__AUTH_SIZE__SHA1                     << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_CFG_AUTH_SIZE_OFFSET|

			QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_CFG_AUTH_ALG_ENONE                      	<< QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_CFG_AUTH_ALG_OFFSET
	);


	//encr configuration

	if (mode == 1) {
		HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_REG,
				1                                                        			<< QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCODE_OFFSET |
				QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_MODE_ECTR		<< QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_MODE_OFFSET|
				QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_ALG_EAES                       			<< QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_ALG_OFFSET
		);

	}

	//authorization segment configuration
	// formatted A_length + P_length
	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_SIZE_REG, auth_seg_size);

	//encryption segment configuration
	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_SIZE_REG, encr_seg_size);

	//total segment size
	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SEG_SIZE_REG, seg_size);

	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_START_REG, encr_seg_start);

	//write initial IV values
	//for (i=0; i<8; i++) {
	//   HAL_REG_WRI(QCC_CRYPTO_AUTH_IVn, (i), initial_auth_iv[i]);
	//}

	//write encryption keys
//	HAL_REG_WR(QCC_CRYPTO_SW_KEY0_0, encr_key[0]);
//	HAL_REG_WR(QCC_CRYPTO_SW_KEY0_1, encr_key[1]);
//	HAL_REG_WR(QCC_CRYPTO_SW_KEY0_2, encr_key[2]);
//	HAL_REG_WR(QCC_CRYPTO_SW_KEY0_3, encr_key[3]);

	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_KEY_TABLE_CFG_REG, 0x5);

	//write counter IV
	//This register does not have an index defined in the address file.
	//for (i=0; i<4; i++) HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR0_IV0 + (i*4), cntr_iv[i])
	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_CNTR0_IV0_REG,  cntr_iv[0]);
	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_CNTR1_IV1_REG,  cntr_iv[1]);
	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_CNTR2_IV2_REG,  cntr_iv[2]);
	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_CNTR3_IV3_REG,  cntr_iv[3]);


	/*  HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR0_IV0,  cntr_iv[0]);
   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR1_IV1,  cntr_iv[1]);
   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR2_IV2,  cntr_iv[2]);
   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR3_IV3,  cntr_iv[3]); */

	//write AUTH INFO NONCE
	//This register does not have an index defined in the address file.
	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_CNTR_MASK_REG, mask[0]);
	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_CNTR_MASK0_REG, mask[1]);
	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_CNTR_MASK1_REG, mask[2]);
	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_CNTR_MASK2_REG, mask[3]);

#endif
	//vv_msg(SEV_INFO, ST_FUNCTION, "*** Starting crypto_arm2 ***");

	// crypto_result = crypto_arm2(din, dout, input_size, output_size, 500, ceId);

	// vv_msg(SEV_INFO, ST_FUNCTION, "*** crypto data pio mode finished ***");

	//printf("*** Starting crypto_dma mode  *** \n" );
#ifdef NT_FN_REG_MODE
	crypto_result = crypto_arm_1(din, dout, input_size, output_size, 500, ceId);
	if(crypto_result == 0) {

	}
#endif

#ifdef NT_FN_DXE_MODE
	crypto_result =  asicDXECryptoDMA(0, 1, din, dout, input_size/4, output_size/4);
	if(crypto_result == 0) {

	}
#endif

	//printf("*** crypto data dma mode finished *** \n");

	//print output dout

	return 0;

}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if 0
int32_t aes128_ctr_known_answer()
{
  int32_t cnt=0;
  uint32_t read_data;
  uint32_t i;
  uint32_t initial_auth_iv[8], expected_auth_iv[8], actual_auth_iv[8], auth_key[16], encr_key[16], xts_key[4], cntr_iv[4], mask[4],nonce[4];
  uint32_t encr_seg_size, auth_seg_size, encr_seg_start, seg_size, mac_size, input_size, output_size, num_nonce_words, encr_xts_du_size;
  uint32_t auth_done = 0;
  uint32_t pass_fail_reg = 0;
  uint32_t din[16], dout[16]={0};
  uint32_t ceId = 0;
  uint32_t crypto_result;

  //declarations
  seg_size = 32;
  encr_seg_size = 32;
  auth_seg_size = 0;
  encr_seg_start = 0;
  mac_size = 0;
  num_nonce_words = 0;
  input_size = seg_size;
  output_size = seg_size + mac_size;
  din[0]  = 0x309F2A97;
  din[1]  = 0x8FF02C2E;
  din[2]  = 0x4A55A09C;
  din[3]  = 0x51406F83;
  din[4]  = 0x69A013A2;
  din[5]  = 0x6572B2DA;
  din[6]  = 0x4C07C67C;
  din[7]  = 0x6B361DA2;
  din[8]  = 0x00000000;
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

  expected_auth_iv[0] = 0x00000000;
  expected_auth_iv[1] = 0x00000000;
  expected_auth_iv[2] = 0x00000000;
  expected_auth_iv[3] = 0x00000000;
  expected_auth_iv[4] = 0x00000000;
  expected_auth_iv[5] = 0x00000000;
  expected_auth_iv[6] = 0x00000000;
  expected_auth_iv[7] = 0x00000000;

  initial_auth_iv[0] = 0x00000000;
  initial_auth_iv[1] = 0x00000000;
  initial_auth_iv[2] = 0x00000000;
  initial_auth_iv[3] = 0x00000000;
  initial_auth_iv[4] = 0x00000000;
  initial_auth_iv[5] = 0x00000000;
  initial_auth_iv[6] = 0x00000000;
  initial_auth_iv[7] = 0x00000000;

  auth_key[0]  = 0x00000000;
  auth_key[1]  = 0x00000000;
  auth_key[2]  = 0x00000000;
  auth_key[3]  = 0x00000000;
  auth_key[4]  = 0x00000000;
  auth_key[5]  = 0x00000000;
  auth_key[6]  = 0x00000000;
  auth_key[7]  = 0x00000000;
  auth_key[8]  = 0x00000000;
  auth_key[9]  = 0x00000000;
  auth_key[10] = 0x00000000;
  auth_key[11] = 0x00000000;
  auth_key[12] = 0x00000000;
  auth_key[13] = 0x00000000;
  auth_key[14] = 0x00000000;
  auth_key[15] = 0x00000000;

  encr_key[0]  = 0xAD575D4B;
  encr_key[1]  = 0x73FD2878;
  encr_key[2]  = 0x1685E4F8;
  encr_key[3]  = 0x2BDBA242;
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

  cntr_iv[0] = 0x8ED01645;
  cntr_iv[1] = 0x80A1C637;
  cntr_iv[2] = 0xD4363813;
  cntr_iv[3] = 0x13DB520A;

  nonce[0] = 0x00000000;
  nonce[1] = 0x00000000;
  nonce[2] = 0x00000000;
  nonce[3] = 0x00000000;

  mask[0] = 0xFFFFFFFF;
  mask[1] = 0xFFFFFFFF;
  mask[2] = 0xFFFFFFFF;
  mask[3] = 0xFFFFFFFF;
#ifdef QCC
  ceId = 0;
  //software reset-- all modules will be reset
  HAL_REG_WR(QCC_CRYPTO_CORE_CFG,0x2);//disable the DMA mode of cryto, use cpu sw mode

  read_data = HAL_REG_RD(QCC_CRYPTO_CORE_CFG);
 // vv_msg(SEV_INFO, ST_FUNCTION, "%s *** QCC_CRYPTO_CORE_CFG %08x ***",__FILE__, read_data);


  //segment configuration
  HAL_REG_WR(QCC_CRYPTO_AUTH_SEG_CFG,
  CRYPTO_AUTH_SEG_CFG__AUTH_POS__BEFORE_ENCRYPTION         << HWIO_QCC_CRYPTO_AUTH_SEG_CFG_AUTH_POS_SHFT |
  CRYPTO_AUTH_SEG_CFG__AUTH_SIZE__SHA1                     << HWIO_QCC_CRYPTO_AUTH_SEG_CFG_AUTH_SIZE_SHFT|
  CRYPTO_AUTH_SEG_CFG__AUTH_ALG__NONE                      << HWIO_QCC_CRYPTO_AUTH_SEG_CFG_AUTH_ALG_SHFT
       );
  //segment configuration
  HAL_REG_WR(QCC_CRYPTO_ENCR_SEG_CFG,
  1                                                        << HWIO_QCC_CRYPTO_ENCR_SEG_CFG_ENCODE_SHFT |
  CRYPTO_ENCR_SEG_CFG__ENCR_MODE__CTR                      << HWIO_QCC_CRYPTO_ENCR_SEG_CFG_ENCR_MODE_SHFT|
  CRYPTO_ENCR_SEG_CFG__ENCR_ALG__AES                       << HWIO_QCC_CRYPTO_ENCR_SEG_CFG_ENCR_ALG_SHFT
  );

  //authorization segment configuration
  // formatted A_length + P_length
  HAL_REG_WRF(QCC_CRYPTO_AUTH_SEG_SIZE, AUTH_SIZE, auth_seg_size);

  //encryption segment configuration
  HAL_REG_WRF(QCC_CRYPTO_ENCR_SEG_SIZE, ENCR_SIZE, encr_seg_size);

  //total segment size
  // formatted A_length + P_length
  HAL_REG_WRF(QCC_CRYPTO_SEG_SIZE, SEG_SIZE, seg_size);

  //total segment size
  // formatted A_length + P_length
  HAL_REG_WRF(QCC_CRYPTO_ENCR_SEG_START, ENCR_START, encr_seg_start);

  //write initial IV values
  //for (i=0; i<8; i++) {
 //   HAL_REG_WRI(QCC_CRYPTO_AUTH_IVn, (i), initial_auth_iv[i]);
  //}
  //write encryption keys

   HAL_REG_WR(QCC_CRYPTO_SW_KEY0_0, encr_key[0]);
   HAL_REG_WR(QCC_CRYPTO_SW_KEY0_1, encr_key[1]);
   HAL_REG_WR(QCC_CRYPTO_SW_KEY0_2, encr_key[2]);
   HAL_REG_WR(QCC_CRYPTO_SW_KEY0_3, encr_key[3]);
  //write counter IV
  //This register does not have an index defined in the address file.
  //for (i=0; i<4; i++) HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR0_IV0 + (i*4), cntr_iv[i])
   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR0_IV0,  cntr_iv[0]);
   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR1_IV1,  cntr_iv[1]);
   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR2_IV2,  cntr_iv[2]);
   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR3_IV3,  cntr_iv[3]);

   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR0_IV0,  cntr_iv[0]);
   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR1_IV1,  cntr_iv[1]);
   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR2_IV2,  cntr_iv[2]);
   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR3_IV3,  cntr_iv[3]);

  //write AUTH INFO NONCE
  //This register does not have an index defined in the address file.
   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR_MASK, mask[0]);
   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR_MASK0, mask[1]);
   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR_MASK1, mask[2]);
   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR_MASK2, mask[3]);


   HAL_REG_WR(QCC_CRYPTO_GOPROC, 1 << HWIO_QCC_CRYPTO_GOPROC_GO_SHFT);
#endif

//note remove crypto engine 1

//  HAL_REG_WR(QCC_CRYPTO_GOPROC, 0x1); // Uncommented for asicDXECryptoDMA
  crypto_result =  asicDXECryptoDMA(0, 1, din, dout, input_size/4, output_size/4);


//note 30 remove
#if 0
  vv_msg(SEV_INFO, ST_FUNCTION, "*** Starting crypto_arm2 ***");
  crypto_result = crypto_arm2(din, dout, input_size, output_size, 500, ceId);
  vv_msg(SEV_INFO, ST_FUNCTION, "*** crypto data pio mode finished ***");
#endif

  //vv_msg(SEV_INFO, ST_FUNCTION, "*** Starting crypto_arm1 ***");
  //crypto_result = crypto_arm(din, dout, input_size, output_size, 500, ceId);
  //vv_msg(SEV_INFO, ST_FUNCTION, "*** crypto data dma mode finished ***");

  //check for proper cipher output
//  if (memcmp(dout, expected_dout, output_size)) {
    for (i = 0; i < (output_size >> 2); i++) {
      //vv_msg(SEV_INFO, ST_FUNCTION, "*** %1d dout %08x exp %08x ***", i, dout[i], expected_dout[i]);
      printf( "*** %1d dout %08x exp %08x *** \n", i, dout[i], expected_dout[i]);
    }
    //vv_test_fail();
//  } else
if (crypto_result >> 31) {
    //vv_msg(SEV_INFO, ST_FUNCTION, "*** crypto_arm2 timed out ***");
    printf("** crypto_arm2 timed out ***");
    //vv_test_fail();
  } else {
   // vv_test_pass();
  }

  return 0;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


int32_t aes256_ctr_known_answer()
{
  int32_t cnt=0;
  uint32_t read_data;
  uint32_t i;
  uint32_t initial_auth_iv[8], expected_auth_iv[8], actual_auth_iv[8], auth_key[16], encr_key[16], xts_key[4], cntr_iv[4], mask[4],nonce[4];
  uint32_t encr_seg_size, auth_seg_size, encr_seg_start, seg_size, mac_size, input_size, output_size, num_nonce_words, encr_xts_du_size;
  uint32_t auth_done = 0;
  uint32_t pass_fail_reg = 0;
  uint32_t din[16], dout[16]={0};
  uint32_t ceId = 0;
  uint32_t crypto_result;

  //declarations
  seg_size = 32;
  encr_seg_size = 32;
  auth_seg_size = 0;
  encr_seg_start = 0;
  mac_size = 0;
  num_nonce_words = 0;
  input_size = seg_size;
  output_size = seg_size + mac_size;

  cntr_iv[0] = 0x8ED01645;
  cntr_iv[1] = 0x80A1C637;
  cntr_iv[2] = 0xD4363813;
  cntr_iv[3] = 0x13DB520A;

  nonce[0] = 0x00000000;
  nonce[1] = 0x00000000;
  nonce[2] = 0x00000000;
  nonce[3] = 0x00000000;

  mask[0] = 0xFFFFFFFF;
  mask[1] = 0xFFFFFFFF;
  mask[2] = 0xFFFFFFFF;
  mask[3] = 0xFFFFFFFF;
#ifdef QCC
  ceId = 0;

  //software reset-- all modules will be reset
  HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_CORE_CFG_REG,0x2);//disable the DMA mode of cryto, use cpu sw mode

  read_data = HAL_REG_RD(QCC_CRYPTO_CORE_CFG);

  //segment configuration
  HAL_REG_WR(QCC_CRYPTO_AUTH_SEG_CFG,
  CRYPTO_AUTH_SEG_CFG__AUTH_POS__BEFORE_ENCRYPTION         << HWIO_QCC_CRYPTO_AUTH_SEG_CFG_AUTH_POS_SHFT |
  CRYPTO_AUTH_SEG_CFG__AUTH_SIZE__SHA1                     << HWIO_QCC_CRYPTO_AUTH_SEG_CFG_AUTH_SIZE_SHFT|
  CRYPTO_AUTH_SEG_CFG__AUTH_ALG__NONE                      << HWIO_QCC_CRYPTO_AUTH_SEG_CFG_AUTH_ALG_SHFT
       );
  //segment configuration
#if 0
  HAL_REG_WR(QCC_CRYPTO_ENCR_SEG_CFG,
  1                                                        << HWIO_QCC_CRYPTO_ENCR_SEG_CFG_ENCODE_SHFT |
  CRYPTO_ENCR_SEG_CFG__ENCR_MODE__CTR                      << HWIO_QCC_CRYPTO_ENCR_SEG_CFG_ENCR_MODE_SHFT|
  CRYPTO_ENCR_SEG_CFG__ENCR_ALG__AES                       << HWIO_QCC_CRYPTO_ENCR_SEG_CFG_ENCR_ALG_SHFT
  );

#endif
 HAL_REG_WR(QCC_CRYPTO_ENCR_SEG_CFG,
           1                                                        << HWIO_QCC_CRYPTO_ENCR_SEG_CFG_ENCODE_SHFT |
           CRYPTO_ENCR_SEG_CFG__ENCR_MODE__CTR                      << HWIO_QCC_CRYPTO_ENCR_SEG_CFG_ENCR_MODE_SHFT|
           CRYPTO_ENCR_SEG_CFG__ENCR_KEY_SZ__AES256                 << HWIO_QCC_CRYPTO_ENCR_SEG_CFG_ENCR_KEY_SZ_SHFT |
           CRYPTO_ENCR_SEG_CFG__ENCR_ALG__AES                       << HWIO_QCC_CRYPTO_ENCR_SEG_CFG_ENCR_ALG_SHFT
           );

  //authorization segment configuration
  // formatted A_length + P_length
  HAL_REG_WRF(QCC_CRYPTO_AUTH_SEG_SIZE, AUTH_SIZE, auth_seg_size);

  //encryption segment configuration
  HAL_REG_WRF(QCC_CRYPTO_ENCR_SEG_SIZE, ENCR_SIZE, encr_seg_size);

  //total segment size
  // formatted A_length + P_length
  HAL_REG_WRF(QCC_CRYPTO_SEG_SIZE, SEG_SIZE, seg_size);

  //total segment size
  // formatted A_length + P_length
  HAL_REG_WRF(QCC_CRYPTO_ENCR_SEG_START, ENCR_START, encr_seg_start);

  //write initial IV values
  //for (i=0; i<8; i++) {
 //   HAL_REG_WRI(QCC_CRYPTO_AUTH_IVn, (i), initial_auth_iv[i]);
  //}
  //write encryption keys

   HAL_REG_WR(QCC_CRYPTO_SW_KEY0_0, encr_key[0]);
   HAL_REG_WR(QCC_CRYPTO_SW_KEY0_1, encr_key[1]);
   HAL_REG_WR(QCC_CRYPTO_SW_KEY0_2, encr_key[2]);
   HAL_REG_WR(QCC_CRYPTO_SW_KEY0_3, encr_key[3]);
  //write counter IV
  //This register does not have an index defined in the address file.
  //for (i=0; i<4; i++) HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR0_IV0 + (i*4), cntr_iv[i])
   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR0_IV0,  cntr_iv[0]);
   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR1_IV1,  cntr_iv[1]);
   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR2_IV2,  cntr_iv[2]);
   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR3_IV3,  cntr_iv[3]);

   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR0_IV0,  cntr_iv[0]);
   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR1_IV1,  cntr_iv[1]);
   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR2_IV2,  cntr_iv[2]);
   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR3_IV3,  cntr_iv[3]);

  //write AUTH INFO NONCE
  //This register does not have an index defined in the address file.
   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR_MASK, mask[0]);
   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR_MASK0, mask[1]);
   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR_MASK1, mask[2]);
   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR_MASK2, mask[3]);


   HAL_REG_WR(QCC_CRYPTO_GOPROC, 1 << HWIO_QCC_CRYPTO_GOPROC_GO_SHFT);
#endif

//note remove crypto engine 1

//  HAL_REG_WR(QCC_CRYPTO_GOPROC, 0x1); // Uncommented for asicDXECryptoDMA
  crypto_result =  asicDXECryptoDMA(0, 1, din, dout, input_size/4, output_size/4);


//note 30 remove
#if 0
  vv_msg(SEV_INFO, ST_FUNCTION, "*** Starting crypto_arm2 ***");
  crypto_result = crypto_arm2(din, dout, input_size, output_size, 500, ceId);
  vv_msg(SEV_INFO, ST_FUNCTION, "*** crypto data pio mode finished ***");
#endif

  //vv_msg(SEV_INFO, ST_FUNCTION, "*** Starting crypto_arm1 ***");
  //crypto_result = crypto_arm(din, dout, input_size, output_size, 500, ceId);
  //vv_msg(SEV_INFO, ST_FUNCTION, "*** crypto data dma mode finished ***");

  //check for proper cipher output
//  if (memcmp(dout, expected_dout, output_size)) {
    for (i = 0; i < (output_size >> 2); i++) {
      //vv_msg(SEV_INFO, ST_FUNCTION, "*** %1d dout %08x exp %08x ***", i, dout[i], expected_dout[i]);
      printf( "*** %1d dout %08x exp %08x *** \n", i, dout[i], expected_dout[i]);
    }
    //vv_test_fail();
//  } else
if (crypto_result >> 31) {
    //vv_msg(SEV_INFO, ST_FUNCTION, "*** crypto_arm2 timed out ***");
    printf("** crypto_arm2 timed out ***");
    //vv_test_fail();
  } else {
   // vv_test_pass();
  }

  return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int32_t aes256_ctr_known_answer_piomode()

{

  int32_t cnt=0;

  uint32_t read_data;

  uint32_t i;

  uint32_t initial_auth_iv[8], expected_auth_iv[8], actual_auth_iv[8], auth_key[16], encr_key[16], xts_key[4], cntr_iv[4], mask[4],nonce[4];

  uint32_t encr_seg_size, auth_seg_size, encr_seg_start, seg_size, mac_size, input_size, output_size, num_nonce_words, encr_xts_du_size;

  uint32_t auth_done = 0;

  uint32_t pass_fail_reg = 0;

  uint32_t din[16], dout[16]={0};

  uint32_t ceId = 0;

  uint32_t crypto_result;



  uint32_t expected_dout[16]={0};



  //declarations

  seg_size = 32;

  encr_seg_size = 32;

  auth_seg_size = 0;

  encr_seg_start = 0;

  mac_size = 0;

  num_nonce_words = 0;

  input_size = seg_size / 4;

  output_size = (seg_size + mac_size) / 4;

  din[0]  = 0x309F2A97;
  din[1]  = 0x8FF02C2E;

  din[2]  = 0x4A55A09C;

  din[3]  = 0x51406F83;
  din[4]  = 0x69A013A2;
  din[5]  = 0x6572B2DA;
  din[6]  = 0x4C07C67C;
  din[7]  = 0x6B361DA2;
  din[8]  = 0x00000000;
  din[9]  = 0x00000000;
  din[10] = 0x00000000;
  din[11] = 0x00000000;
  din[12] = 0x00000000;
  din[13] = 0x00000000;
  din[14] = 0x00000000;
  din[15] = 0x00000000;


  expected_dout[0]  = 0x468aefec;
  expected_dout[1]  = 0x8c4401fb;
  expected_dout[2]  = 0x3c402bce;
  expected_dout[3]  = 0x24714750;
  expected_dout[4]  = 0x1751ec59;
  expected_dout[5]  = 0x257e1ea2;
  expected_dout[6]  = 0xd4c95df4;
  expected_dout[7]  = 0x1915fca8;
  expected_dout[8]  = 0x00000000;
  expected_dout[9]  = 0x00000000;
  expected_dout[10] = 0x00000000;
  expected_dout[11] = 0x00000000;
  expected_dout[12] = 0x00000000;
  expected_dout[13] = 0x00000000;
  expected_dout[14] = 0x00000000;
  expected_dout[15] = 0x00000000;


  expected_auth_iv[0] = 0x00000000;
  expected_auth_iv[1] = 0x00000000;
  expected_auth_iv[2] = 0x00000000;
  expected_auth_iv[3] = 0x00000000;
  expected_auth_iv[4] = 0x00000000;
  expected_auth_iv[5] = 0x00000000;
  expected_auth_iv[6] = 0x00000000;
  expected_auth_iv[7] = 0x00000000;


  initial_auth_iv[0] = 0x00000000;
  initial_auth_iv[1] = 0x00000000;
  initial_auth_iv[2] = 0x00000000;
  initial_auth_iv[3] = 0x00000000;
  initial_auth_iv[4] = 0x00000000;
  initial_auth_iv[5] = 0x00000000;
  initial_auth_iv[6] = 0x00000000;
  initial_auth_iv[7] = 0x00000000;


  auth_key[0]  = 0x00000000;
  auth_key[1]  = 0x00000000;
  auth_key[2]  = 0x00000000;
  auth_key[3]  = 0x00000000;
  auth_key[4]  = 0x00000000;
  auth_key[5]  = 0x00000000;
  auth_key[6]  = 0x00000000;
  auth_key[7]  = 0x00000000;
  auth_key[8]  = 0x00000000;
  auth_key[9]  = 0x00000000;
  auth_key[10] = 0x00000000;
  auth_key[11] = 0x00000000;
  auth_key[12] = 0x00000000;
  auth_key[13] = 0x00000000;
  auth_key[14] = 0x00000000;
  auth_key[15] = 0x00000000;


  encr_key[0]  = 0xAD575D4B;
  encr_key[1]  = 0x73FD2878;
  encr_key[2]  = 0x1685E4F8;
  encr_key[3]  = 0x2BDBA242;
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


  cntr_iv[0] = 0x8ED01645;
  cntr_iv[1] = 0x80A1C637;
  cntr_iv[2] = 0xD4363813;
  cntr_iv[3] = 0x13DB520A;


  nonce[0] = 0x00000000;
  nonce[1] = 0x00000000;
  nonce[2] = 0x00000000;
  nonce[3] = 0x00000000;

  mask[0] = 0xFFFFFFFF;
  mask[1] = 0xFFFFFFFF;
  mask[2] = 0xFFFFFFFF;
  mask[3] = 0xFFFFFFFF;
#ifdef QCC

  ceId = 0;

  //software reset-- all modules will be reset

  HAL_REG_WR(QCC_CRYPTO_CORE_CFG,0x2);//enable the PIO mode of cryto



  read_data = HAL_REG_RD(QCC_CRYPTO_CORE_CFG);

  //vv_msg(SEV_INFO, ST_FUNCTION, "%s *** QCC_CRYPTO_CORE_CFG %08x ***",__FILE__, read_data);

  printf("%s *** QCC_CRYPTO_CORE_CFG %08x ***",__FILE__, read_data);





  //auth  configuration

  HAL_REG_WR(QCC_CRYPTO_AUTH_SEG_CFG,

  CRYPTO_AUTH_SEG_CFG__AUTH_POS__BEFORE_ENCRYPTION         << HWIO_QCC_CRYPTO_AUTH_SEG_CFG_AUTH_POS_SHFT |

  CRYPTO_AUTH_SEG_CFG__AUTH_SIZE__SHA1                     << HWIO_QCC_CRYPTO_AUTH_SEG_CFG_AUTH_SIZE_SHFT|

  CRYPTO_AUTH_SEG_CFG__AUTH_ALG__NONE                      << HWIO_QCC_CRYPTO_AUTH_SEG_CFG_AUTH_ALG_SHFT

       );


  //encr configuration

  HAL_REG_WR(QCC_CRYPTO_ENCR_SEG_CFG,

  1                                                        << HWIO_QCC_CRYPTO_ENCR_SEG_CFG_ENCODE_SHFT |

  CRYPTO_ENCR_SEG_CFG__ENCR_MODE__CTR                      << HWIO_QCC_CRYPTO_ENCR_SEG_CFG_ENCR_MODE_SHFT|

  CRYPTO_ENCR_SEG_CFG__ENCR_KEY_SZ__AES256                 << HWIO_QCC_CRYPTO_ENCR_SEG_CFG_ENCR_KEY_SZ_SHFT |

  CRYPTO_ENCR_SEG_CFG__ENCR_ALG__AES                       << HWIO_QCC_CRYPTO_ENCR_SEG_CFG_ENCR_ALG_SHFT

  );



  //authorization segment configuration

  // formatted A_length + P_length

  HAL_REG_WRF(QCC_CRYPTO_AUTH_SEG_SIZE, AUTH_SIZE, auth_seg_size);



  //encryption segment configuration

  HAL_REG_WRF(QCC_CRYPTO_ENCR_SEG_SIZE, ENCR_SIZE, encr_seg_size);



  //total segment size

  HAL_REG_WRF(QCC_CRYPTO_SEG_SIZE, SEG_SIZE, seg_size);



  HAL_REG_WRF(QCC_CRYPTO_ENCR_SEG_START, ENCR_START, encr_seg_start);



  //write initial IV values

  //for (i=0; i<8; i++) {

 //   HAL_REG_WRI(QCC_CRYPTO_AUTH_IVn, (i), initial_auth_iv[i]);

  //}

  //write encryption keys



   HAL_REG_WR(QCC_CRYPTO_SW_KEY0_0, encr_key[0]);

   HAL_REG_WR(QCC_CRYPTO_SW_KEY0_1, encr_key[1]);

   HAL_REG_WR(QCC_CRYPTO_SW_KEY0_2, encr_key[2]);
   HAL_REG_WR(QCC_CRYPTO_SW_KEY0_3, encr_key[3]);

   HAL_REG_WR(QCC_CRYPTO_SW_KEY0_4, encr_key[4]);

   HAL_REG_WR(QCC_CRYPTO_SW_KEY0_5, encr_key[5]);

   HAL_REG_WR(QCC_CRYPTO_SW_KEY0_6, encr_key[6]);

   HAL_REG_WR(QCC_CRYPTO_SW_KEY0_7, encr_key[7]);

  //write counter IV

  //This register does not have an index defined in the address file.

  //for (i=0; i<4; i++) HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR0_IV0 + (i*4), cntr_iv[i])

   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR0_IV0,  cntr_iv[0]);

   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR1_IV1,  cntr_iv[1]);

   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR2_IV2,  cntr_iv[2]);

   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR3_IV3,  cntr_iv[3]);


  /*  HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR0_IV0,  cntr_iv[0]);

   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR1_IV1,  cntr_iv[1]);

   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR2_IV2,  cntr_iv[2]);

   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR3_IV3,  cntr_iv[3]); */


  //write AUTH INFO NONCE

  //This register does not have an index defined in the address file.

   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR_MASK, mask[0]);
   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR_MASK0, mask[1]);
   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR_MASK1, mask[2]);
   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR_MASK2, mask[3]);



   HAL_REG_WR(QCC_CRYPTO_GOPROC, 1 << HWIO_QCC_CRYPTO_GOPROC_GO_SHFT);

#endif



//note remove crypto engine 1





//note 30 remove



  //vv_msg(SEV_INFO, ST_FUNCTION, "*** Starting crypto_arm2 ***");

 // crypto_result = crypto_arm2(din, dout, input_size, output_size, 500, ceId);

 // vv_msg(SEV_INFO, ST_FUNCTION, "*** crypto data pio mode finished ***");



  //vv_msg(SEV_INFO, ST_FUNCTION, "*** Starting crypto_dma mode  ***");

  printf("*** Starting crypto_dma mode  ***");

  crypto_result = crypto_arm2(din, dout, input_size, output_size, 500, ceId);

  //vv_msg(SEV_INFO, ST_FUNCTION, "*** crypto data dma mode finished ***");
  printf("*** crypto data dma mode finished ***");


  //check for proper cipher output

//  if (memcmp(dout, expected_dout, output_size)) {

    for (i = 0; i < output_size; i++) {

      //vv_msg(SEV_INFO, ST_FUNCTION, "*** %1d dout %08x exp %08x ***", i, dout[i], expected_dout[i]);

      printf("*** %1d dout %08x exp %08x ***", i, dout[i], expected_dout[i]);

    }

    //vv_test_fail();

    return 1;

//  } else
if (crypto_result >> 31) {

   // vv_msg(SEV_INFO, ST_FUNCTION, "*** crypto_arm2 timed out ***");

  //  vv_test_fail();

    return 1;

  } else {

    //vv_test_pass();

    return 0;

  }



  return 0;

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int32_t aes128_ctr_dma_known_answer()

{

  int32_t cnt=0;

  uint32_t read_data;

  uint32_t i;

  uint32_t initial_auth_iv[8], expected_auth_iv[8], actual_auth_iv[8], auth_key[16], encr_key[16], xts_key[4], cntr_iv[4], mask[4],nonce[4];

  uint32_t encr_seg_size, auth_seg_size, encr_seg_start, seg_size, mac_size, input_size, output_size, num_nonce_words, encr_xts_du_size;

  uint32_t auth_done = 0;

  uint32_t pass_fail_reg = 0;

  uint32_t din[16], dout[16]={0};

  uint32_t ceId = 0;

  uint32_t crypto_result;



  uint32_t expected_dout[16]={0};



  //declarations

  seg_size = 32;

  encr_seg_size = 32;

  auth_seg_size = 0;

  encr_seg_start = 0;

  mac_size = 0;

  num_nonce_words = 0;

  input_size = seg_size / 4;

  output_size = (seg_size + mac_size) / 4;

  din[0]  = 0x309F2A97;
  din[1]  = 0x8FF02C2E;

  din[2]  = 0x4A55A09C;

  din[3]  = 0x51406F83;
  din[4]  = 0x69A013A2;
  din[5]  = 0x6572B2DA;
  din[6]  = 0x4C07C67C;
  din[7]  = 0x6B361DA2;
  din[8]  = 0x00000000;
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


  expected_auth_iv[0] = 0x00000000;
  expected_auth_iv[1] = 0x00000000;
  expected_auth_iv[2] = 0x00000000;
  expected_auth_iv[3] = 0x00000000;
  expected_auth_iv[4] = 0x00000000;
  expected_auth_iv[5] = 0x00000000;
  expected_auth_iv[6] = 0x00000000;
  expected_auth_iv[7] = 0x00000000;


  initial_auth_iv[0] = 0x00000000;
  initial_auth_iv[1] = 0x00000000;
  initial_auth_iv[2] = 0x00000000;
  initial_auth_iv[3] = 0x00000000;
  initial_auth_iv[4] = 0x00000000;
  initial_auth_iv[5] = 0x00000000;
  initial_auth_iv[6] = 0x00000000;
  initial_auth_iv[7] = 0x00000000;


  auth_key[0]  = 0x00000000;
  auth_key[1]  = 0x00000000;
  auth_key[2]  = 0x00000000;
  auth_key[3]  = 0x00000000;
  auth_key[4]  = 0x00000000;
  auth_key[5]  = 0x00000000;
  auth_key[6]  = 0x00000000;
  auth_key[7]  = 0x00000000;
  auth_key[8]  = 0x00000000;
  auth_key[9]  = 0x00000000;
  auth_key[10] = 0x00000000;
  auth_key[11] = 0x00000000;
  auth_key[12] = 0x00000000;
  auth_key[13] = 0x00000000;
  auth_key[14] = 0x00000000;
  auth_key[15] = 0x00000000;


  encr_key[0]  = 0xAD575D4B;
  encr_key[1]  = 0x73FD2878;
  encr_key[2]  = 0x1685E4F8;
  encr_key[3]  = 0x2BDBA242;
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


  cntr_iv[0] = 0x8ED01645;
  cntr_iv[1] = 0x80A1C637;
  cntr_iv[2] = 0xD4363813;
  cntr_iv[3] = 0x13DB520A;


  nonce[0] = 0x00000000;
  nonce[1] = 0x00000000;
  nonce[2] = 0x00000000;
  nonce[3] = 0x00000000;

  mask[0] = 0xFFFFFFFF;
  mask[1] = 0xFFFFFFFF;
  mask[2] = 0xFFFFFFFF;
  mask[3] = 0xFFFFFFFF;
#ifdef QCC

  ceId = 0;

  //software reset-- all modules will be reset

  HAL_REG_WR(QCC_CRYPTO_CORE_CFG,0x5);//enable the DMA mode of cryto



  read_data = HAL_REG_RD(QCC_CRYPTO_CORE_CFG);

  //vv_msg(SEV_INFO, ST_FUNCTION, "%s *** QCC_CRYPTO_CORE_CFG %08x ***",__FILE__, read_data);





  //auth  configuration

  HAL_REG_WR(QCC_CRYPTO_AUTH_SEG_CFG,

  CRYPTO_AUTH_SEG_CFG__AUTH_POS__BEFORE_ENCRYPTION         << HWIO_QCC_CRYPTO_AUTH_SEG_CFG_AUTH_POS_SHFT |

  CRYPTO_AUTH_SEG_CFG__AUTH_SIZE__SHA1                     << HWIO_QCC_CRYPTO_AUTH_SEG_CFG_AUTH_SIZE_SHFT|

  CRYPTO_AUTH_SEG_CFG__AUTH_ALG__NONE                      << HWIO_QCC_CRYPTO_AUTH_SEG_CFG_AUTH_ALG_SHFT

       );


  //encr configuration

  HAL_REG_WR(QCC_CRYPTO_ENCR_SEG_CFG,

  1                                                        << HWIO_QCC_CRYPTO_ENCR_SEG_CFG_ENCODE_SHFT |

  CRYPTO_ENCR_SEG_CFG__ENCR_MODE__CTR                      << HWIO_QCC_CRYPTO_ENCR_SEG_CFG_ENCR_MODE_SHFT|

  CRYPTO_ENCR_SEG_CFG__ENCR_ALG__AES                       << HWIO_QCC_CRYPTO_ENCR_SEG_CFG_ENCR_ALG_SHFT

  );



  //authorization segment configuration

  // formatted A_length + P_length

  HAL_REG_WRF(QCC_CRYPTO_AUTH_SEG_SIZE, AUTH_SIZE, auth_seg_size);



  //encryption segment configuration

  HAL_REG_WRF(QCC_CRYPTO_ENCR_SEG_SIZE, ENCR_SIZE, encr_seg_size);



  //total segment size

  HAL_REG_WRF(QCC_CRYPTO_SEG_SIZE, SEG_SIZE, seg_size);



  HAL_REG_WRF(QCC_CRYPTO_ENCR_SEG_START, ENCR_START, encr_seg_start);



  //write initial IV values

  //for (i=0; i<8; i++) {

 //   HAL_REG_WRI(QCC_CRYPTO_AUTH_IVn, (i), initial_auth_iv[i]);

  //}

  //write encryption keys



   HAL_REG_WR(QCC_CRYPTO_SW_KEY0_0, encr_key[0]);

   HAL_REG_WR(QCC_CRYPTO_SW_KEY0_1, encr_key[1]);

   HAL_REG_WR(QCC_CRYPTO_SW_KEY0_2, encr_key[2]);
   HAL_REG_WR(QCC_CRYPTO_SW_KEY0_3, encr_key[3]);

  //write counter IV

  //This register does not have an index defined in the address file.

  //for (i=0; i<4; i++) HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR0_IV0 + (i*4), cntr_iv[i])

   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR0_IV0,  cntr_iv[0]);

   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR1_IV1,  cntr_iv[1]);

   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR2_IV2,  cntr_iv[2]);

   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR3_IV3,  cntr_iv[3]);


   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR0_IV0,  cntr_iv[0]);

   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR1_IV1,  cntr_iv[1]);

   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR2_IV2,  cntr_iv[2]);

   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR3_IV3,  cntr_iv[3]);


  //write AUTH INFO NONCE

  //This register does not have an index defined in the address file.

   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR_MASK, mask[0]);
   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR_MASK0, mask[1]);
   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR_MASK1, mask[2]);
   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR_MASK2, mask[3]);

#endif


  //vv_msg(SEV_INFO, ST_FUNCTION, "*** Starting crypto_dma mode  ***");

  crypto_result = crypto_arm(din, dout, input_size, output_size, 500, ceId);

  //vv_msg(SEV_INFO, ST_FUNCTION, "*** crypto data dma mode finished ***");


  if (memcmp(dout, expected_dout, output_size))

  {

    for (i = 0; i < (output_size); i++)

{

            // vv_msg(SEV_INFO, ST_FUNCTION, "*** %1d dout %08x exp %08x ***", i, dout[i], expected_dout[i]);

             printf("*** %1d dout %08x exp %08x ***", i, dout[i], expected_dout[i]);
    }

    //vv_test_fail();

  }

  else if (crypto_result >> 31)

  {

    //vv_msg(SEV_INFO, ST_FUNCTION, "*** crypto_arm2 timed out ***");

    printf("*** crypto_arm2 timed out ***");
    //vv_test_fail();

  }

  else

  {

    //vv_test_pass();

    printf("test pass \n");
  }



  din[0]  = 0x309F2A97;

  din[1]  = 0x8FF02C2E;

  din[2]  = 0x4A55A09C;

  din[3]  = 0x51406F83;

  din[4]  = 0x69A013A2;

  din[5]  = 0x6572B2DA;

  din[6]  = 0x4C07C67C;

  din[7]  = 0x6B361DA2;

  din[8]  = 0x00000000;

  din[9]  = 0x00000000;

  din[10] = 0x00000000;

  din[11] = 0x00000000;

  din[12] = 0x00000000;

  din[13] = 0x00000000;

  din[14] = 0x00000000;

  din[15] = 0x00000000;



   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR0_IV0,  cntr_iv[0]);

   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR1_IV1,  cntr_iv[1]);

   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR2_IV2,  cntr_iv[2]);

   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR3_IV3,  cntr_iv[3]);



   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR0_IV0,  cntr_iv[0]);

   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR1_IV1,  cntr_iv[1]);

   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR2_IV2,  cntr_iv[2]);

   HAL_REG_WR(QCC_CRYPTO_ENCR_CNTR3_IV3,  cntr_iv[3]);



   //vv_msg(SEV_INFO, ST_FUNCTION, "*** Starting crypto_dma mode  ***");

  crypto_result = crypto_arm(din, dout, input_size, output_size, 500, ceId);

  //vv_msg(SEV_INFO, ST_FUNCTION, "*** crypto data dma mode finished ***");



  if (memcmp(dout, expected_dout, output_size))

  {

    for (i = 0; i < (output_size); i++)

{

      //vv_msg(SEV_INFO, ST_FUNCTION, "*** %1d dout %08x exp %08x ***", i, dout[i], expected_dout[i]);

    }

    vv_test_fail();

  }

  else if (crypto_result >> 31)

  {

    //vv_msg(SEV_INFO, ST_FUNCTION, "*** crypto_arm2 timed out ***");

    vv_test_fail();

  }

  else

  {

    vv_test_pass();

  }



}
#endif
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
