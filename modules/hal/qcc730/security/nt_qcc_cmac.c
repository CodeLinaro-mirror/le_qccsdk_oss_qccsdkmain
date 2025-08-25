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

------------------------------------------------------------------------------

-- Author      : ccuston                                                   --

-- Description : template                                                   --

-- Notes       :                                                            --

------------------------------------------------------------------------------

*****************************************************************************/



/** include files **/
#include "nt_hw.h"
#include "nt_osal.h"
#include "hal_int_sys.h"

#define QCC 1


int32_t aes128_cmac_known_answer()

{
  int32_t cnt=0;
  uint32_t read_data;
  uint32_t i;
  uint32_t initial_auth_iv[8], expected_auth_iv[8], actual_auth_iv[8], auth_key[16], encr_key[16], xts_key[4], cntr_iv[4], nonce[4], expected_mac[4];
  uint32_t encr_seg_size, auth_seg_size, encr_seg_start, seg_size, mac_size, input_size, output_size, num_nonce_words, encr_xts_du_size;
  uint32_t auth_done = 0;
  uint32_t pass_fail_reg = 0;
  uint32_t din[16], dout[16]={0}, expected_dout[16]={0};
  uint32_t ceId = 0;
  uint32_t crypto_result;


  //declarations
  seg_size = 16;
  encr_seg_size = 0;
  auth_seg_size = 16;
  encr_seg_start = 0;
  mac_size = 0;
  num_nonce_words = 0;
  input_size = seg_size;
  output_size = 0;

  din[0]  = 0x6bc1bee2;
  din[1]  = 0x2e409f96;
  din[2]  = 0xe93d7e11;
  din[3]  = 0x7393172a;
  din[4]  = 0x00000000;
  din[5]  = 0x00000000;
  din[6]  = 0x00000000;
  din[7]  = 0x00000000;
  din[8]  = 0x00000000;
  din[9]  = 0x00000000;
  din[10] = 0x00000000;
  din[11] = 0x00000000;
  din[12] = 0x00000000;
  din[13] = 0x00000000;
  din[14] = 0x00000000;
  din[15] = 0x00000018;


  expected_dout[0]  = 0x00000000;
  expected_dout[1]  = 0x00000000;
  expected_dout[2]  = 0x00000000;
  expected_dout[3]  = 0x00000000;
  expected_dout[4]  = 0x00000000;
  expected_dout[5]  = 0x00000000;
  expected_dout[6]  = 0x00000000;
  expected_dout[7]  = 0x00000000;
  expected_dout[8]  = 0x00000000;
  expected_dout[9]  = 0x00000000;
  expected_dout[10] = 0x00000000;
  expected_dout[11] = 0x00000000;
  expected_dout[12] = 0x00000000;
  expected_dout[13] = 0x00000000;
  expected_dout[14] = 0x00000000;
  expected_dout[15] = 0x00000000;


  expected_auth_iv[0] = 0x5FD596EE;
  expected_auth_iv[1] = 0x78D5553C;
  expected_auth_iv[2] = 0x8FF4E72D;
  expected_auth_iv[3] = 0x266DFD19;
  expected_auth_iv[4] = 0x2366DA29;
  expected_auth_iv[5] = 0x00000000;
  expected_auth_iv[6] = 0x00000000;
  expected_auth_iv[7] = 0x00000000;

  initial_auth_iv[0] = 0X67452301;
  initial_auth_iv[1] = 0Xefcdab89;
  initial_auth_iv[2] = 0X98badcfe;
  initial_auth_iv[3] = 0X10325476;
  initial_auth_iv[4] = 0Xc3d2e1f0;
  initial_auth_iv[5] = 0x00000000;
  initial_auth_iv[6] = 0x00000000;
  initial_auth_iv[7] = 0x00000000;


  cntr_iv[0] = 0x00000000;
  cntr_iv[1] = 0x00000000;
  cntr_iv[2] = 0x00000000;
  cntr_iv[3] = 0x00000000;

  nonce[0] = 0x00000000;
  nonce[1] = 0x00000000;
  nonce[2] = 0x00000000;
  nonce[3] = 0x00000000;

  expected_mac[0] = 0x070a16b4;
  expected_mac[1] = 0x6b4d4144;
  expected_mac[2] = 0xf79bdd9d;
  expected_mac[3] = 0xd04a287c;

#ifdef QCC

  ceId=0;

  //software reset-- all modules will be reset

  HWIO_OUT(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_CORE_CFG_REG,0x2);

  //pulling out of reset with default configuration

  HWIO_OUT(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_CORE_CFG_REG,0xE014);

  read_data = HWIO_IN(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_CORE_CFG_REG);


  //segment configuration

  HWIO_OUT(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_CFG_REG,

  1 << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_CFG_COMP_EXP_MAC_OFFSET |

//   0 << HWIO_QCC_CRYPTO_AUTH_SEG_CFG_AUTH_NONCE_NUM_WORDS_SHFT |

//   0 << HWIO_QCC_CRYPTO_AUTH_SEG_CFG_USE_HW_KEY_AUTH_SHFT |

  1 << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_CFG_FIRST_OFFSET |

  0 << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_CFG_LAST_OFFSET |

  0 << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_CFG_AUTH_POS_OFFSET |

  0 << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_CFG_AUTH_SIZE_OFFSET|

  1 << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_CFG_AUTH_MODE_OFFSET |  //CMAC

  0 << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_CFG_AUTH_KEY_SZ_OFFSET |

  2 << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_CFG_AUTH_ALG_OFFSET); //SHA1



  //authorization segment configuration

  // formatted A_length + P_length

  HWIO_OUTF(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_SIZE_REG, auth_seg_size);



  //encryption segment configuration
  //HWIO_OUTF(QCC_CRYPTO_ENCR_SEG_SIZE, ENCR_SIZE, encr_seg_size);



  //total segment size
  // formatted A_length + P_length
  HWIO_OUTF(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SEG_SIZE_REG, seg_size);



  //total segment size
  // formatted A_length + P_length
  HWIO_OUTF(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_START_REG, encr_seg_start);



  //write hmac key
//  for (i=0; i<4; i++) {
//    HWIO_OUTI(QCC_CRYPTO_AUTH_KEYn, (i), auth_key[i]);
//  }



  //write the expected MAC register
//  for (i=0; i<4; i++) {
//    HWIO_OUTI(QCC_CRYPTO_AUTH_EXP_MACn, (i), expected_mac[i]);
//  }




  //check for MAC

	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_GOPROC_REG, 1 << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_GOPROC_GO_OFFSET);

#endif

#ifdef NT_FN_REG_MODE
	crypto_result = crypto_arm(din, dout, input_size, output_size, 1000, ceId);
#endif

#ifdef NT_FN_DXE_MODE
	crypto_result =  asicDXECryptoDMA(0, 1, din, dout, input_size/4, output_size/4);
#endif


//
//  if (HWIO_INF(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_REG, MAC_FAILED)) {
//      printf("Test Fail \n");
//  }

  return 0;

}

#if 0

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int32_t aes256_cmac_known_answer_macfailed()

{

  int32_t cnt=0;

  uint32_t read_data;

  uint32_t i;

  uint32_t initial_auth_iv[8], expected_auth_iv[8], actual_auth_iv[8], auth_key[16], encr_key[16], xts_key[4], cntr_iv[4], nonce[4], expected_mac[4];

  uint32_t encr_seg_size, auth_seg_size, encr_seg_start, seg_size, mac_size, input_size, output_size, num_nonce_words, encr_xts_du_size;

  uint32_t auth_done = 0;

  uint32_t pass_fail_reg = 0;

  uint32_t din[16], dout[16]={0}, expected_dout[16]={0};

  uint32_t ceId = 0;

  uint32_t crypto_result;


  //declarations

  seg_size = 32;

  encr_seg_size = 0;

  auth_seg_size = 32;

  encr_seg_start = 0;

  mac_size = 0;

  num_nonce_words = 0;

  input_size = seg_size;

  output_size = seg_size + mac_size;



  din[0]  = 0x6bc1bee2;

  din[1]  = 0x2e409f96;

  din[2]  = 0xe93d7e11;

  din[3]  = 0x7393172a;

  din[4]  = 0x00000000;

  din[5]  = 0x00000000;

  din[6]  = 0x00000000;

  din[7]  = 0x00000000;

  din[8]  = 0x00000000;

  din[9]  = 0x00000000;

  din[10] = 0x00000000;

  din[11] = 0x00000000;

  din[12] = 0x00000000;

  din[13] = 0x00000000;

  din[14] = 0x00000000;

  din[15] = 0x00000018;



  expected_dout[0]  = 0x00000000;

  expected_dout[1]  = 0x00000000;

  expected_dout[2]  = 0x00000000;

  expected_dout[3]  = 0x00000000;

  expected_dout[4]  = 0x00000000;

  expected_dout[5]  = 0x00000000;

  expected_dout[6]  = 0x00000000;

  expected_dout[7]  = 0x00000000;

  expected_dout[8]  = 0x00000000;

  expected_dout[9]  = 0x00000000;

  expected_dout[10] = 0x00000000;

  expected_dout[11] = 0x00000000;

  expected_dout[12] = 0x00000000;

  expected_dout[13] = 0x00000000;

  expected_dout[14] = 0x00000000;

  expected_dout[15] = 0x00000000;



  expected_auth_iv[0] = 0x81ceed26;

  expected_auth_iv[1] = 0xd27b86d0;

  expected_auth_iv[2] = 0xb7342344;

  expected_auth_iv[3] = 0xd7eff938;

  expected_auth_iv[4] = 0x00000000;

  expected_auth_iv[5] = 0x00000000;

  expected_auth_iv[6] = 0x00000000;

  expected_auth_iv[7] = 0x00000000;



  initial_auth_iv[0] = 0X67452301;

  initial_auth_iv[1] = 0Xefcdab89;

  initial_auth_iv[2] = 0X98badcfe;

  initial_auth_iv[3] = 0X10325476;

  initial_auth_iv[4] = 0Xc3d2e1f0;

  initial_auth_iv[5] = 0x00000000;

  initial_auth_iv[6] = 0x00000000;

  initial_auth_iv[7] = 0x00000000;



  auth_key[0]  = 0x2b7e1516;

  auth_key[1]  = 0x28aed2a6;

  auth_key[2]  = 0xabf71588;

  auth_key[3]  = 0x09cf4f3c;

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



  cntr_iv[0] = 0x00000000;

  cntr_iv[1] = 0x00000000;

  cntr_iv[2] = 0x00000000;

  cntr_iv[3] = 0x00000000;



  nonce[0] = 0x00000000;

  nonce[1] = 0x00000000;

  nonce[2] = 0x00000000;

  nonce[3] = 0x00000000;



  expected_mac[0] = 0x070a16b4;

  expected_mac[1] = 0x6b4d4144;

  expected_mac[2] = 0xf79bdd9d;

  expected_mac[3] = 0xd04a287c;

#ifdef QCC

  ceId=0;

 /*  //software reset-- all modules will be reset

  HWIO_OUT(QCC_CRYPTO_CONFIG,0x1);



  //pulling out of reset with default configuration

  HWIO_OUT(QCC_CRYPTO_CONFIG,0xE014);

  read_data = HWIO_IN(QCC_CRYPTO_CONFIG);

vv_msg(SEV_INFO, ST_FUNCTION, "%s *** CRYPTO_CONFIG %08x ***",__FILE__, read_data);

  if (read_data != 0xE014) {

    vv_test_fail();

  } */

//software reset-- all modules will be reset

  HWIO_OUT(QCC_CRYPTO_CORE_CFG,0x2);//use the DMA mode of cryto, disable cpu sw mode



  read_data = HWIO_IN(QCC_CRYPTO_CORE_CFG);

//  vv_msg(SEV_INFO, ST_FUNCTION, "%s *** QCC_CRYPTO_CORE_CFG %08x ***",__FILE__, read_data);



  //segment configuration

  HWIO_OUT(QCC_CRYPTO_AUTH_SEG_CFG,

  1 << HWIO_QCC_CRYPTO_AUTH_SEG_CFG_COMP_EXP_MAC_SHFT |

  1 << HWIO_QCC_CRYPTO_AUTH_SEG_CFG_FIRST_SHFT |

  1 << HWIO_QCC_CRYPTO_AUTH_SEG_CFG_LAST_SHFT |

  0 << HWIO_QCC_CRYPTO_AUTH_SEG_CFG_AUTH_POS_SHFT |

  0 << HWIO_QCC_CRYPTO_AUTH_SEG_CFG_AUTH_SIZE_SHFT|

  1 << HWIO_QCC_CRYPTO_AUTH_SEG_CFG_AUTH_MODE_SHFT |  //CMAC

  2 << HWIO_QCC_CRYPTO_AUTH_SEG_CFG_AUTH_KEY_SZ_SHFT | //AES256

  2 << HWIO_QCC_CRYPTO_AUTH_SEG_CFG_AUTH_ALG_SHFT); //AES







  //authorization segment configuration

  // formatted A_length + P_length

  HWIO_OUTF(QCC_CRYPTO_AUTH_SEG_SIZE, AUTH_SIZE, auth_seg_size);



  //encryption segment configuration

  //HWIO_OUTF(QCC_CRYPTO_ENCR_SEG_SIZE, ENCR_SIZE, encr_seg_size);



  //total segment size

  // formatted A_length + P_length

  HWIO_OUTF(QCC_CRYPTO_SEG_SIZE, SEG_SIZE, seg_size);



  //total segment size

  // formatted A_length + P_length

  //HWIO_OUTF(QCC_CRYPTO_ENCR_SEG_START, ENCR_START, encr_seg_start);



   HWIO_OUT(QCC_CRYPTO_AUTH_IV0,  initial_auth_iv[0]); //for AES256 ...

   HWIO_OUT(QCC_CRYPTO_AUTH_IV1,  initial_auth_iv[1]);

   HWIO_OUT(QCC_CRYPTO_AUTH_IV2,  initial_auth_iv[2]);

   HWIO_OUT(QCC_CRYPTO_AUTH_IV3,  initial_auth_iv[3]);

   HWIO_OUT(QCC_CRYPTO_AUTH_IV4,  initial_auth_iv[4]);

   HWIO_OUT(QCC_CRYPTO_AUTH_IV5,  initial_auth_iv[5]);

   HWIO_OUT(QCC_CRYPTO_AUTH_IV6,  initial_auth_iv[6]);

   HWIO_OUT(QCC_CRYPTO_AUTH_IV7,  initial_auth_iv[7]);



    //Expected MAC values

   HWIO_OUT(QCC_CRYPTO_AUTH_EXP_MAC0, expected_auth_iv[0]); //for AES256

   HWIO_OUT(QCC_CRYPTO_AUTH_EXP_MAC1, expected_auth_iv[1]);

   HWIO_OUT(QCC_CRYPTO_AUTH_EXP_MAC2, expected_auth_iv[2]);

   HWIO_OUT(QCC_CRYPTO_AUTH_EXP_MAC3, expected_auth_iv[3]);

   HWIO_OUT(QCC_CRYPTO_AUTH_EXP_MAC4, expected_auth_iv[4]);

   HWIO_OUT(QCC_CRYPTO_AUTH_EXP_MAC5, expected_auth_iv[5]);

   HWIO_OUT(QCC_CRYPTO_AUTH_EXP_MAC6, expected_auth_iv[6]);

   HWIO_OUT(QCC_CRYPTO_AUTH_EXP_MAC7, expected_auth_iv[7]);



  //write hmac key

  /* for (i=0; i<4; i++) {

    HWIO_OUTI(QCC_CRYPTO_AUTH_KEYn, (i), auth_key[i]);

  } */



  //write the expected MAC register

 /*  for (i=0; i<4; i++) {

    HWIO_OUTI(QCC_CRYPTO_AUTH_EXP_MACn, (i), expected_mac[i]);

  } */



 // HWIO_OUT(QCC_CRYPTO_GOPROC, 1 << HWIO_QCC_CRYPTO_GOPROC_GO_SHFT);

//  vv_msg(SEV_INFO, ST_FUNCTION, "*** Starting crypto_arm ***");

  //crypto_result = crypto_arm(din, NULL,seg_size, ceId);

//  crypto_result = crypto_arm(din, dout, input_size, output_size, 500, ceId);

//  vv_msg(SEV_INFO, ST_FUNCTION, "*** crypto_arm finished ***");



   HWIO_OUT(QCC_CRYPTO_GOPROC, 0x1); // Uncommented for asicDXECryptoDMA

   crypto_result =  asicDXECryptoDMA(0, 1, din, dout, input_size/4, output_size/4);



  HWIO_OUTF(QCC_CRYPTO_AUTH_SEG_SIZE, AUTH_SIZE, 0);



  //encryption segment configuration

  HWIO_OUTF(QCC_CRYPTO_ENCR_SEG_SIZE, ENCR_SIZE, 0);



  //total segment size

  // formatted A_length + P_length

  HWIO_OUTF(QCC_CRYPTO_SEG_SIZE, SEG_SIZE, 0);



  //total segment size

  // formatted A_length + P_length

  //kbp HWIO_OUTF(QCC_CRYPTO_ENCR_SEG_START, ENCR_START, encr_seg_start);

  HWIO_OUTF(QCC_CRYPTO_AUTH_SEG_START, AUTH_START, 0);



  HWIO_OUT(QCC_CRYPTO_AUTH_SEG_CFG,0);



  HWIO_OUT(QCC_CRYPTO_ENCR_SEG_CFG,0);



  //check for MAC



  if (HWIO_INF(QCC_CRYPTO_STATUS, MAC_FAILED)) {



actual_auth_iv[0] = HWIO_IN(QCC_CRYPTO_AUTH_IV0);

actual_auth_iv[1] = HWIO_IN(QCC_CRYPTO_AUTH_IV1);

actual_auth_iv[2] = HWIO_IN(QCC_CRYPTO_AUTH_IV2);

actual_auth_iv[3] = HWIO_IN(QCC_CRYPTO_AUTH_IV3);

actual_auth_iv[4] = HWIO_IN(QCC_CRYPTO_AUTH_IV4);

actual_auth_iv[5] = HWIO_IN(QCC_CRYPTO_AUTH_IV5);

actual_auth_iv[6] = HWIO_IN(QCC_CRYPTO_AUTH_IV6);

actual_auth_iv[7] = HWIO_IN(QCC_CRYPTO_AUTH_IV7);


for(i=0; i<8; i++) {

// vv_msg(SEV_INFO, ST_FUNCTION, "*** %1d iv %08x  ***", i, actual_auth_iv[i]);

printf("*** %1d iv %08x  ***", i, actual_auth_iv[i]);

}

printf("Test Fail \n");

//    vv_test_fail();

return 1;

  /* } else if (crypto_result >> 31) {

    vv_msg(SEV_INFO, ST_FUNCTION, "*** crypto_arm2 timed out ***");

    vv_test_fail(); */

  } else {

    printf("Test Pass \n");

//    vv_test_pass();

return 0;

  }

#endif



//note 90 remove crypto engine 1



  //return 0;

}

#endif

#endif
