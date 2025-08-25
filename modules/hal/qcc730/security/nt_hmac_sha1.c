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

#define QCC 1

#if 0
int32_t hmac_sha1_known_answer_macfailed_piomode()

{

  int32_t cnt=0;

  uint32_t read_data;

  uint32_t i;

  uint32_t initial_auth_iv[8], expected_auth_iv[8], actual_auth_iv[8], auth_key[16], encr_key[16], xts_key[4], cntr_iv[4], nonce[4];

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

  input_size = seg_size/4;

  output_size = 0;



  din[0]  = 0x53616D70;

  //din[0]  = 0x706d6153;

  din[1]  = 0x6C65206D;

  din[2]  = 0x65737361;

  din[3]  = 0x67652066;

  din[4]  = 0x6F72206B;

  din[5]  = 0x65796C65;

  din[6]  = 0x6E3D626C;

  din[7]  = 0x6F636B6C;

  din[8]  = 0x656E0000;

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

  expected_dout[15] = 0x00000018;



/*  expected_auth_iv[0] = 0xc37224a0;

  expected_auth_iv[1] = 0x0e8f0a9a;

  expected_auth_iv[2] = 0x2472d3c1;

  expected_auth_iv[3] = 0xb4d2a962;

  expected_auth_iv[4] = 0x7b51fa11;

  expected_auth_iv[5] = 0x00000000;

  expected_auth_iv[6] = 0x00000000;

  expected_auth_iv[7] = 0x00000000;*/



  expected_auth_iv[0] = 0x5301e125;

  expected_auth_iv[1] = 0x5b4290a3;

  expected_auth_iv[2] = 0x1b1d2b32;

  expected_auth_iv[3] = 0x31c06237;

  expected_auth_iv[4] = 0xabc88dd1;

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



  auth_key[0]  = 0x00010203;

  auth_key[1]  = 0x04050607;

  auth_key[2]  = 0x08090A0B;

  auth_key[3]  = 0x0C0D0E0F;

  auth_key[4]  = 0x10111213;

  auth_key[5]  = 0x14151617;

  auth_key[6]  = 0x18191A1B;

  auth_key[7]  = 0x1C1D1E1F;

  auth_key[8]  = 0x20212223;

  auth_key[9]  = 0x24252627;

  auth_key[10] = 0x28292A2B;

  auth_key[11] = 0x2C2D2E2F;

  auth_key[12] = 0x30313233;

  auth_key[13] = 0x34353637;

  auth_key[14] = 0x38393A3B;

  auth_key[15] = 0x3C3D3E3F;



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

#ifdef QCC

ceId=0;

  //software reset-- all modules will be reset

  HWIO_OUT(QCC_CRYPTO_CORE_CFG,0x2); //PIO mode



  /* //pulling out of reset with default configuration

  HWIO_OUT(QCC_CRYPTO_CONFIG,0XE014);

  read_data = HWIO_IN(QCC_CRYPTO_CONFIG);

vv_msg(SEV_INFO, ST_FUNCTION, "%s *** CRYPTO_CONFIG %08x ***",__FILE__, read_data);

  if (read_data != 0xE014) {

    vv_test_fail();

  } */



  // New thing for crypto5.

  // Added above--bit 4.

  // HWIO_OUTF(QCC_CRYPTO_CONFIG, HIGH_SPD_DATA_EN_N, 1);

read_data = HWIO_IN(QCC_CRYPTO_CORE_CFG);

  //vv_msg(SEV_INFO, ST_FUNCTION, "%s *** QCC_CRYPTO_CORE_CFG %08x ***",__FILE__, read_data);

  printf("%s *** QCC_CRYPTO_CORE_CFG %08x *** \n",__FILE__, read_data);



  //segment configuration - HMAC SHA1

  HWIO_OUT(QCC_CRYPTO_AUTH_SEG_CFG,

  CRYPTO_AUTH_SEG_CFG__AUTH_POS__BEFORE_ENCRYPTION         << HWIO_QCC_CRYPTO_AUTH_SEG_CFG_AUTH_POS_SHFT |

  CRYPTO_AUTH_SEG_CFG__AUTH_SIZE__SHA1                     << HWIO_QCC_CRYPTO_AUTH_SEG_CFG_AUTH_SIZE_SHFT|

  CRYPTO_AUTH_SEG_CFG__AUTH_ALG__SHA                      << HWIO_QCC_CRYPTO_AUTH_SEG_CFG_AUTH_ALG_SHFT |

  CRYPTO_AUTH_SEG_CFG__AUTH_MODE__HMAC << HWIO_QCC_CRYPTO_AUTH_SEG_CFG_AUTH_MODE_SHFT |

  1 << HWIO_QCC_CRYPTO_AUTH_SEG_CFG_FIRST_SHFT |

  1 << HWIO_QCC_CRYPTO_AUTH_SEG_CFG_LAST_SHFT |

  1 << HWIO_QCC_CRYPTO_AUTH_SEG_CFG_COMP_EXP_MAC_SHFT //CRYPTO_AUTH_SEG_CFG__COMP_EXP_MAC__HASH

       );




 /*  // auth segment configuration

  HWIO_OUT(QCC_CRYPTO_AUTH_SEG_CFG,

  0                                                       << HWIO_QCC_CRYPTO_AUTH_SEG_CFG_AUTH_NONCE_NUM_WORDS_SHFT |

  CRYPTO_AUTH_SEG_CFG__USE_HW_KEY_AUTH__USE_KEY_REGISTERS << HWIO_QCC_CRYPTO_AUTH_SEG_CFG_USE_HW_KEY_AUTH_SHFT |

  1 << HWIO_QCC_CRYPTO_AUTH_SEG_CFG_FIRST_SHFT |

  1 << HWIO_QCC_CRYPTO_AUTH_SEG_CFG_LAST_SHFT |

  CRYPTO_AUTH_SEG_CFG__AUTH_POS__BEFORE_ENCRYPTION        << HWIO_QCC_CRYPTO_AUTH_SEG_CFG_AUTH_POS_SHFT |

  CRYPTO_AUTH_SEG_CFG__AUTH_SIZE__SHA1                    << HWIO_QCC_CRYPTO_AUTH_SEG_CFG_AUTH_SIZE_SHFT |

  CRYPTO_AUTH_SEG_CFG__AUTH_MODE__HMAC                    << HWIO_QCC_CRYPTO_AUTH_SEG_CFG_AUTH_MODE_SHFT |

  CRYPTO_AUTH_SEG_CFG__AUTH_KEY_SZ__AES256                << HWIO_QCC_CRYPTO_AUTH_SEG_CFG_AUTH_KEY_SZ_SHFT |

  CRYPTO_AUTH_SEG_CFG__AUTH_ALG__SHA << HWIO_QCC_CRYPTO_AUTH_SEG_CFG_AUTH_ALG_SHFT

  ); //SHA1



  // encr segment configuration

  HWIO_OUT(QCC_CRYPTO_ENCR_SEG_CFG,

  CRYPTO_ENCR_SEG_CFG__USE_HW_KEY_ENCR__USE_KEY_REGISTERS  << HWIO_QCC_CRYPTO_ENCR_SEG_CFG_USE_HW_KEY_ENCR_SHFT |

  CRYPTO_ENCR_SEG_CFG__CNTR_ALG__NIST_800_32A              << HWIO_QCC_CRYPTO_ENCR_SEG_CFG_CNTR_ALG_SHFT |

  0                                                        << HWIO_QCC_CRYPTO_ENCR_SEG_CFG_ENCODE_SHFT |

  CRYPTO_ENCR_SEG_CFG__ENCR_MODE__ECB                      << HWIO_QCC_CRYPTO_ENCR_SEG_CFG_ENCR_MODE_SHFT |

  CRYPTO_ENCR_SEG_CFG__ENCR_KEY_SZ__AES128                 << HWIO_QCC_CRYPTO_ENCR_SEG_CFG_ENCR_KEY_SZ_SHFT |

  CRYPTO_ENCR_SEG_CFG__ENCR_ALG__NONE                      << HWIO_QCC_CRYPTO_ENCR_SEG_CFG_ENCR_ALG_SHFT

  );

  */

  //authorization segment configuration

  // formatted A_length + P_length

  HWIO_OUTF(QCC_CRYPTO_AUTH_SEG_SIZE, AUTH_SIZE, auth_seg_size);



  //encryption segment configuration

 // HWIO_OUTF(QCC_CRYPTO_ENCR_SEG_SIZE, ENCR_SIZE, encr_seg_size);



  //total segment size

  // formatted A_length + P_length

  HWIO_OUTF(QCC_CRYPTO_SEG_SIZE, SEG_SIZE, seg_size);



  //total segment size

  // formatted A_length + P_length

  //HWIO_OUTF(QCC_CRYPTO_ENCR_SEG_START, ENCR_START, encr_seg_start);


 /*   //write hmac key

  for (i=0; i<16; i++) {

    HWIO_OUTI(QCC_CRYPTO_AUTH_KEYn, (i), auth_key[i]);

  }

 */

  //write initial IV values

 /*  for (i=0; i<8; i++) {

    HWIO_OUTI(QCC_CRYPTO_AUTH_IVn, (i), initial_auth_iv[i]);

  } */



   HWIO_OUT(QCC_CRYPTO_AUTH_IV0,  initial_auth_iv[0]);

   HWIO_OUT(QCC_CRYPTO_AUTH_IV1,  initial_auth_iv[1]);

   HWIO_OUT(QCC_CRYPTO_AUTH_IV2,  initial_auth_iv[2]);

   HWIO_OUT(QCC_CRYPTO_AUTH_IV3,  initial_auth_iv[3]);

   HWIO_OUT(QCC_CRYPTO_AUTH_IV4,  initial_auth_iv[0]);

   HWIO_OUT(QCC_CRYPTO_AUTH_IV5,  initial_auth_iv[1]);

   HWIO_OUT(QCC_CRYPTO_AUTH_IV6,  initial_auth_iv[2]);

   HWIO_OUT(QCC_CRYPTO_AUTH_IV7,  initial_auth_iv[3]);



    //Expected MAC values

   HWIO_OUT(QCC_CRYPTO_AUTH_EXP_MAC0, expected_auth_iv[0]);

   HWIO_OUT(QCC_CRYPTO_AUTH_EXP_MAC1, expected_auth_iv[1]);

   HWIO_OUT(QCC_CRYPTO_AUTH_EXP_MAC2, expected_auth_iv[2]);

   HWIO_OUT(QCC_CRYPTO_AUTH_EXP_MAC3, expected_auth_iv[3]);

   HWIO_OUT(QCC_CRYPTO_AUTH_EXP_MAC4, expected_auth_iv[4]);

   HWIO_OUT(QCC_CRYPTO_AUTH_EXP_MAC5, expected_auth_iv[5]);

   HWIO_OUT(QCC_CRYPTO_AUTH_EXP_MAC6, expected_auth_iv[6]);

   HWIO_OUT(QCC_CRYPTO_AUTH_EXP_MAC7, expected_auth_iv[7]);



  //write encryption keys

 /*  for (i=0; i<4; i++) {

    HWIO_OUTI(QCC_CRYPTO_ENCR_KEYn, (i), encr_key[i]);

  } */

#if 0

  //write counter IV

  //This register does not have an index defined in the address file.

  //for (i=0; i<4; i++) HWIO_OUT(QCC_CRYPTO_ENCR_CNTR0_IV0 + (i*4), cntr_iv[i])

  HWIO_OUT(QCC_CRYPTO_ENCR_CNTR0_IV0,  cntr_iv[0]);

  HWIO_OUT(QCC_CRYPTO_ENCR_CNTR1_IV1,  cntr_iv[1]);

  HWIO_OUT(QCC_CRYPTO_ENCR_CNTR2_IV2,  cntr_iv[2]);

  HWIO_OUT(QCC_CRYPTO_ENCR_CNTR3_IV3,  cntr_iv[3]);



  //write counter IV

  //This register does not have an index defined in the address file.

  for (i=0; i<4; i++) {

    HWIO_OUTI(QCC_CRYPTO_AUTH_INFO_NONCEn, (i), nonce[i]);

  }



  HWIO_OUT(QCC_CRYPTO_GOPROC, 1 << HWIO_QCC_CRYPTO_GOPROC_GO_SHFT);

#endif



  HWIO_OUT(QCC_CRYPTO_GOPROC, 1 << HWIO_QCC_CRYPTO_GOPROC_GO_SHFT);



  //vv_msg(SEV_INFO, ST_FUNCTION, "*** Starting crypto_arm ***");

  printf("*** Starting crypto_arm *** \n");

  //crypto_result = crypto_arm2(din, dout, input_size, output_size, 500, ceId);

  crypto_result = crypto_arm2_sha(din, dout, input_size, ceId);

  //vv_msg(SEV_INFO, ST_FUNCTION, "*** crypto_arm finished ***");

  printf("*** crypto_arm finished *** \n");





  /* //hash values

  for(i=0; i<8; i++) {

    actual_auth_iv[i] = HWIO_INI(QCC_CRYPTO_AUTH_IVn, (i));

    vv_msg(SEV_INFO, ST_FUNCTION, "*** crypto_auth_iv%1d %08x exp %08x ***", i, actual_auth_iv[i], expected_auth_iv[i]);



    // Check for proper Hash output

    if(expected_auth_iv[i] != actual_auth_iv[i]) {

      pass_fail_reg  = 1;

      //HWIO_OUT( NATIVE_CTL0_ADR, COMP_FAIL_FLAG );

    }

  }     */

#endif





//note 90 remove crypto engine 1



 /*  if (pass_fail_reg) {

    vv_test_fail();

  } */

 // vv_msg(SEV_INFO, ST_FUNCTION, "\n MAC_FAILED =%d \n", HWIO_INF(QCC_CRYPTO_STATUS, MAC_FAILED));

  printf("\n MAC_FAILED =%d \n", HWIO_INF(QCC_CRYPTO_STATUS, MAC_FAILED));

if(HWIO_INF(QCC_CRYPTO_STATUS, MAC_FAILED)==0)

{

// vv_test_pass();

        printf("test Pass \n");

return 0;

}

else

{


actual_auth_iv[0] = HWIO_IN(QCC_CRYPTO_AUTH_IV0);

actual_auth_iv[1] = HWIO_IN(QCC_CRYPTO_AUTH_IV1);

actual_auth_iv[2] = HWIO_IN(QCC_CRYPTO_AUTH_IV2);

actual_auth_iv[3] = HWIO_IN(QCC_CRYPTO_AUTH_IV3);

actual_auth_iv[4] = HWIO_IN(QCC_CRYPTO_AUTH_IV4);

actual_auth_iv[5] = HWIO_IN(QCC_CRYPTO_AUTH_IV5);

actual_auth_iv[6] = HWIO_IN(QCC_CRYPTO_AUTH_IV6);

actual_auth_iv[7] = HWIO_IN(QCC_CRYPTO_AUTH_IV7);



for(i=0; i<8; i++) {

//vv_msg(SEV_INFO, ST_FUNCTION, "*** %1d iv %08x  ***", i, actual_auth_iv[i]);

printf("*** %1d iv %08x  *** \n", i, actual_auth_iv[i]);

}

//vv_test_fail();

printf("Test Fail\n");

return 1;

}

#if 0

  //check for proper cipher output



  if (memcmp(dout, expected_dout, output_size)) {

    for (i = 0; i < 16; i++) {

      if (dout[i] == expected_dout[i]) {

vv_msg(SEV_INFO, ST_FUNCTION, "*** %1d out %08x == %08x exp ***", i, dout[i], expected_dout[i]);

      } else {

vv_msg(SEV_INFO, ST_FUNCTION, "*** %1d out %08x != %08x exp ***", i, dout[i], expected_dout[i]);

      }

    }

    vv_test_fail();

  } else if (crypto_result >> 31) {

    vv_msg(SEV_INFO, ST_FUNCTION, "*** crypto_arm2 timed out ***");

    vv_test_fail();

  } else {

    vv_test_pass();

  }

  return 0;

#endif

}

#endif
#endif
