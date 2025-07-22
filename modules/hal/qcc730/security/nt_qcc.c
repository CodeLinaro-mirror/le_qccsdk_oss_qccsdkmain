/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "nt_flags.h"
#include "nt_hw.h"
#include "hal_int_sys.h"
#include "nt_osal.h"
#include "dxe_api.h"
#include "nt_crypto.h"

#ifdef NT_FN_HW_CRYPTO

#define ENCRYPT 1
#define DECRYPT 0

// Common driver function for the crypto
typedef enum _OPERATION_ {
    HCAL_DECRYPTION = 0,
    HCAL_ENCRYPTION,
    HCAL_AUTHENTICATION,
    HCAL_ENCRYP_AUTHENTICATION,
    HCAL_DECRYP_AUTHENTICATION,

    HCAL_MAX_OPERATION,
} NT_HCAL_OPERATION;

// typedef enum _HASH_OPERATION_ {
// HCAL_SECURE_HASHING = 0,
// HCAL_HASH_AUTHENTICATION,
//
// HCAL_MAX_HASH_OPERATION,
//}NT_HCAL_HASH_OPERATION;

typedef enum _CIPHER_ALG_ {
    HCAL_CIPHER_NONE = 0,
    HCAL_CIPHER_RESERVED,
    HCAL_CIPHER_AES,

    HCAL_MAX_CIPHER_ALG,
} NT_HCAL_CIPHER_ALG;

typedef enum _CIPHER_MODE_ {
    HCAL_AES_ECB = 0,
    HCAL_AES_CBC,
    HCAL_AES_CTR,
    HCAL_AES_CCM,
    HCAL_AES_CCM_N,
    // HCAL_AES_CMAC,
    // HCAL_SHA_1,
    // HCAL_SHA_256,
    // HCAL_HMAC,
    HCAL_MAX_CIPHER_MODE,
} NT_HCAL_CIPHER_MODE;

typedef enum _AUTH_ALG_ {
    HCAL_NONE = 0,
    HCAL_SHA,
    HCAL_AES,

    HCAL_MAX_AUTH_ALG,
} NT_HCAL_AUTH_ALG;

typedef enum _AUTH_MODE_ {
    HCAL_SHA_1 = 0,
    HCAL_SHA_256,
    HCAL_SHA_HMAC,
    HCAL_AES_CMAC,
} NT_HCAL_AUTH_MODE;

typedef enum _CIPHER_HASH_TYPE_ {
    HCAL_CIPHER_AES_128_ECB = 0,
    HCAL_CIPHER_AES_256_ECB,
    HCAL_CIPHER_AES_128_CCM,
    HCAL_CIPHER_AES_256_CCM,
    HCAL_CIPHER_AES_128_CCM_N,
    HCAL_CIPHER_AES_256_CCM_N,
    HCAL_CIPHER_AES_128_CBC,
    HCAL_CIPHER_AES_256_CBC,
    HCAL_CIPHER_AES_128_CTR,
    HCAL_CIPHER_AES_256_CTR,
    HCAL_CIPHER_AES_128_CMAC,
    HCAL_CIPHER_AES_256_CMAC,
    HCAL_HASH_SHA1,
    HCAL_HASH_SHA256,
    HCAL_HASH_HMAC256,

    HCAL_MAX_TYPE,
} NT_HCAL_CIPHER_HASH_TYPE;

// typedef enum _KEY_LENGTH_ {
// HCAL_KEY_LENGTH_128 = 128,
// HCAL_KEY_LENGTH_256 = 256,
//}NT_HCAL_KEY_LENGTH;

typedef struct _HCAL_CONFIG_ {
    NT_HCAL_OPERATION operation;
    // NT_HCAL_CIPHER_HASH_TYPE type;

    // CIPHER
    NT_HCAL_CIPHER_ALG cipher_alg;
    NT_HCAL_CIPHER_MODE cipher_mode;

    // AUTHENTICATION
    NT_HCAL_AUTH_ALG auth_alg;
    NT_HCAL_AUTH_MODE auth_mode;

    uint16_t keysize;
    const void *crypto_input;
    uint32_t crypto_input_size;
    void *crypto_output;
    uint32_t crypto_output_size;
    const uint32_t *swkey;
    uint32_t swkey_size;
    uint32_t *encr_cntr_iv;
    uint32_t encr_cntr_iv_size;
    uint32_t *auth_iv;
    uint32_t authiv_size;

} NT_HCAL_QCC_CONFIG;

extern uint8_t data;

uint8_t nt_qcc_init(void)
{
    /* Enable QCC clock source || By default peripheral clock is enabled in ccu register */
    /* QCC Peripheral enable | Enable QCC only if it is not already enabled */

    // enable QCC

    // SW moves data to/from crypto lib via DATA_IN,OUT registers
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_CORE_CFG_REG, 0x2);

    return 0;
}

uint32_t nt_hcal_qcc_operation(NT_HCAL_QCC_CONFIG *config)
{
    uint32_t enc_config_reg = 0;
    uint32_t encr_seg_size = 0;
    uint32_t encr_seg_start = 0;
    uint32_t seg_size = 0;
    uint32_t mac_size = 0;
    uint32_t input_size = 0;
    uint32_t output_size = 0;

    uint32_t encr_key[16] = {0};
    uint32_t cntr_iv[4] = {0};
    uint32_t nonce[4] = {0};
    uint32_t loc_i = 0;
    uint32_t ceId = 0;
    uint32_t crypto_result = 0;
    // uint32_t *lcdin = NULL;

    if (config->operation >= HCAL_MAX_OPERATION) {
        return 1;
    }

    // enc_config_reg = HW_REG_RD( QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_REG );

    switch (config->operation) {
        case HCAL_DECRYPTION:
        case HCAL_ENCRYPTION:
            switch (config->cipher_mode) {
                case HCAL_AES_ECB:
                    seg_size = 16;
                    break;

                case HCAL_AES_CBC:
                    seg_size = 64;
                    memcpy(cntr_iv, config->encr_cntr_iv, config->encr_cntr_iv_size);
                    break;

                case HCAL_AES_CTR:
                    break;

                case HCAL_AES_CCM:
                    break;

                default:
                    break;
            }

            encr_seg_size = seg_size;
            encr_seg_start = 0;
            mac_size = 0;
            input_size = seg_size / 4;
            output_size = (seg_size + mac_size) / 4;

            // Enable the encryption in CRYPTO_ENCR_SEG_CFG_REG bit CRYPTO_ENCR_SEG_CFG_ENCODE to 1
            enc_config_reg |= (config->cipher_alg << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_ALG_OFFSET) |
                              (config->cipher_mode << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_MODE_OFFSET) |
                              (config->operation << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCODE_OFFSET);
            switch (config->keysize) {
                case 128:
                    enc_config_reg |= QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_KEY_SZ_EAES128
                                      << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_KEY_SZ_OFFSET;
                    break;

                case 256:
                    enc_config_reg |= QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_KEY_SZ_EAES256
                                      << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_ENCR_KEY_SZ_OFFSET;
                    break;

                default:
                    break;
            }

            memcpy(encr_key, config->swkey, config->swkey_size);
            // lcdin = (uint32_t *)&config->crypto_input;
            break;

        case HCAL_AUTHENTICATION:
            break;

        case HCAL_ENCRYP_AUTHENTICATION:
            break;

        case HCAL_DECRYP_AUTHENTICATION:
            break;

        default:
            break;
    }

    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_CORE_CFG_REG, 0x2);

    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_REG, enc_config_reg);

    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_SIZE_REG, encr_seg_size);

    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SEG_SIZE_REG, seg_size);

    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_START_REG, encr_seg_start);

    //(void)loc_i;

    for (loc_i = 0; loc_i < config->swkey_size / 2; loc_i++) {
        HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SW_KEY0_0_REG + (loc_i * 4), encr_key[loc_i]);
    }

    for (loc_i = 0; loc_i < 4; loc_i++) {
        HAL_REG_WR((QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_CNTR0_IV0_REG + (loc_i * 4)), cntr_iv[loc_i]);

        HAL_REG_WR((QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_INFO_NONCE0_REG + (loc_i * 4)), nonce[loc_i]);
    }

    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_GOPROC_REG, 0x1);

    //	nt_qccreg_transfer( config->crypto_input, config->crypto_output, input_size, output_size, 500, ceId);

    if (data == HCAL_REGISTER_MODE) {
        crypto_result =
            nt_qcc_register_transfer(config->crypto_input, config->crypto_output, input_size, output_size, 500, ceId);
        if (crypto_result != 0) {
        }
    }

    else if (data == HCAL_DXE_MODE) {
        crypto_result = nt_qcc_dxe_transfer(DXE_CHANNEL_8, DXE_CHANNEL_9, (uint32_t)config->crypto_input,
                                            (uint32_t)config->crypto_output, input_size, output_size);
        if (crypto_result != 0) {
        }
    }

    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_SIZE_REG, 0);

    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SEG_SIZE_REG, 0);

    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_START_REG, 0);

    return 0;
}

int32_t nt_qcc_aes128_ecb(uint8_t mode, const unsigned char din[16], const uint32_t key[8], unsigned char dout[16])
{
    NT_HCAL_QCC_CONFIG aes_ecb128_config;

    // configuration
    aes_ecb128_config.operation = mode;
    aes_ecb128_config.cipher_alg = HCAL_CIPHER_AES;
    aes_ecb128_config.cipher_mode = HCAL_AES_ECB;
    aes_ecb128_config.keysize = 128;

    // Crypto input data
    aes_ecb128_config.crypto_input = din;
    aes_ecb128_config.crypto_input_size = 16;

    aes_ecb128_config.crypto_output = dout;

    aes_ecb128_config.swkey = key;

    aes_ecb128_config.swkey_size = 64 / 4;

    nt_hcal_qcc_operation(&aes_ecb128_config);

    return 0;
}

#if 0
int32_t nt_qcc_aes128_cbc(int mode, const unsigned char iv[16], const unsigned char *input,
							const uint32_t key[8],unsigned char *output)
{
	NT_HCAL_QCC_CONFIG aes_cbc128_config;

	// configuration
	aes_cbc128_config.operation = mode;
	aes_cbc128_config.cipher_alg = HCAL_CIPHER_AES;
	aes_cbc128_config.cipher_mode = HCAL_AES_CBC;
	aes_cbc128_config.keysize = 128;

	// Crypto input data
	aes_cbc128_config.crypto_input = din;
	aes_cbc128_config.crypto_input_size = 16;

	aes_cbc128_config.crypto_output = dout;

	aes_cbc128_config.encr_cntr_iv = iv;
	aes_cbc128_config.encr_cntr_iv_size = ( sizeof(iv) >> 2 );

	aes_cbc128_config.swkey = key;
	aes_cbc128_config.swkey_size = 64/4;

	aes_cbc128_config( &aes_cbc128_config );

}
#endif

/*test code*/

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
