/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "nt_hw.h"
#include "nt_crypto.h"
#include "hal_int_sys.h"
#include "string.h"
#include "nt_osal.h"

#ifdef NT_FN_HW_CRYPTO
uint32_t seg_size = 32;

uint8_t data;

uint32_t expected_dout[16];

#if 0
//dma mode for data out
uint32_t crypto_arm(uint32_t ce_data_in[], uint32_t ce_data_out[], uint32_t din_limit, uint32_t dout_limit, uint32_t __attribute__((__unused__)) timeout, uint32_t ceId)
{
	//Crypto FIFO is only 32 bytes in size
	uint32_t i, j, val;

	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_DATA_IN_BUF_BASE_REG, DATA_IN_BASE_ADDRESS);

	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_DATA_IN_BUF_LEN_REG, din_limit);

	//HWIO_OUT(QCC_CRYPTO_DATA_IN_BUF_LEN, seg_size  );

	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_DATA_OUT_BUF_BASE_REG, DATA_OUT_BASE_ADDRESS);


	//din_limit = 1;
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
		}

		//shoor HWIO_OUT(QCC_CRYPTO_GOPROC, 1 << HWIO_QCC_CRYPTO_GOPROC_GO_SHFT);
		//HWIO_OUT(QCC_CRYPTO_GOPROC, 0x1);

		HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_GOPROC_REG, 0x1);

		HAL_DBG_PRINT("wait for crypto to finish processing ",data,0,0);
		//Wait for Crypto to finish prossessing
		//while( !(HWIO_INF(QCC_CRYPTO_STATUS, OPERATION_DONE)));
		do {
			val = HAL_REG_RD(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_REG);
		}while(!(val & QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_OPERATION_DONE_MASK));

		HAL_DBG_PRINT("crypto process completed ",data,0,0);

		dout_limit = HAL_REG_RD(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_DATA_OUT_BUF_LEN_REG);

		for(j = 0; j < dout_limit; j++) {
			ce_data_out[j] = DATA_OUT(j);
			HAL_DBG_PRINT("ce_data_out",ce_data_out[i],0,0);
		}
	}
	return 0;
}


uint32_t crypto_arm_1(uint32_t ce_data_in[], uint32_t ce_data_out[], uint32_t din_limit, uint32_t dout_limit, uint32_t __attribute__((__unused__)) timeout, uint32_t ceId)
{
	//Crypto FIFO is only 32 bytes in size
	uint32_t i, j, val;
//	uint32_t crypto_status=0, din_size_avail=0, dout_size_avail=0, din_rdy=0, dout_rdy=0, operation_done=0, din_cnt=0, dout_cnt=0;
//	uint32_t return_status = 0;

	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_DATA_IN_BUF_BASE_REG, DATA_IN_BASE_ADDRESS);
	//HWIO_OUT(QCC_CRYPTO_DATA_IN_BUF_LEN, seg_size );
	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_DATA_IN_BUF_LEN_REG, din_limit);


	HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_DATA_OUT_BUF_BASE_REG, DATA_OUT_BASE_ADDRESS);
	//din_limit = 1;
	for(i = 0; i < din_limit/4; i++) {
		//init the data in buffer
		DATA_IN(i) = 0x00000000;
	}

	for(i = 0; i < din_limit/4; i++) {
		//init the data out buffer
		DATA_OUT(i) = 0x00000000;
	}

	if (ceId == 0) {
		for(i = 0; i < din_limit/4; i++) {
			//write  data in buffer in
			DATA_IN(i) = ce_data_in[i];
		}

		//shoor HWIO_OUT(QCC_CRYPTO_GOPROC, 1 << HWIO_QCC_CRYPTO_GOPROC_GO_SHFT);
		HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_GOPROC_REG, 0x1);
		//HWIO_OUT(QCC_CRYPTO_GOPROC, 0x1);

		//Wait for Crypto to finish prossessing
		//while( !(HWIO_INF(QCC_CRYPTO_STATUS, OPERATION_DONE)));
		do {
			val = HAL_REG_RD(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_REG);
		}while((val & QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_OPERATION_DONE_MASK));

		dout_limit = HAL_REG_RD(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_DATA_OUT_BUF_LEN_REG);

		for(j = 0; j < dout_limit/4; j++) {
			ce_data_out[j] = DATA_OUT(j);
			//print dout
		}
	}
	return 0;
}
#endif

// cpu pio mode read data out
uint32_t nt_qcc_register_transfer(const uint32_t ce_data_in[], uint32_t ce_data_out[], uint32_t din_limit,
                                  uint32_t __attribute__((__unused__)) dout_limit,
                                  uint32_t __attribute__((__unused__)) timeout, uint32_t ceId)
{
    // Crypto FIFO is only 32 bytes in size
    uint32_t i, j;

    if (ceId == 0) {  // CRYPTO
        // execute this loop as many times as necessary. Setting the "timeout" should prevent the test from exceeding
        // 5hour limit of simulation if crypto hangs.
        for (i = 0; i < din_limit; i += 4) {
            // write 4 data in buffer in
            for (j = 0; j < 4; j++) {
                HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_DATA_IN_REG + (j * 4), ce_data_in[i + j]);
            }
            // read 4 data to out buffer
            for (j = 0; j < 4; j++) {
                ce_data_out[j + i] = HAL_REG_RD(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_DATA_OUT_REG + (j * 4));
            }
        }
    }

    return 1;  // crypto done!
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// cpu pio mode read data out
uint32_t crypto_arm2_sha(uint32_t ce_data_in[], uint32_t __attribute__((__unused__)) ce_data_out[], uint32_t din_limit,
                         uint32_t ceId)
{
    // Crypto FIFO is only 32 bytes in size
    uint32_t i = 0, j = 0;
    // uint32_t crypto_status=0, din_size_avail=0, dout_size_avail=0, din_rdy=0, dout_rdy=0, operation_done=0,
    // din_cnt=0, dout_cnt=0; uint32_t return_status = 0;
    if (ceId == 0) {  // CRYPTO
        // execute this loop as many times as necessary. Setting the "timeout" should prevent the test from exceeding
        // 5hour limit of simulation if crypto hangs.
        for (i = 0; i < din_limit; i++) {  // original value is i+=4
            // write 4 data in buffer in

            HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_DATA_IN_REG, ce_data_in[i + j]);
        }
    }
    // Set return_status to the number of times through the loop (i), and set the msb.
    // return_status = i | (1 << 31);
    return 1;  // crypto done!
    // return return_status; //crypto done!
}  // crypto_arm2

/*===========================================================================*/

uint8_t nt_mode_of_transfer(uint8_t mode)
{
    if (mode == 0) {
        data = HCAL_DXE_MODE;
        HAL_DBG_PRINT("dxe mode", data, 0, 0);
    } else if (mode == 1) {
        data = HCAL_REGISTER_MODE;
        HAL_DBG_PRINT("register mode", data, 0, 0);
    } else {
        HAL_DBG_PRINT("invalid mode", data, 0, 0);
    }
    return data;
}

#endif

#ifdef WIFI_HW_AES
/**
 * @brief  Gets the size of data available to write into Data in registers
 * @return Size
 */
uint32_t crypto_arm_available_in_size(void)
{
    uint32_t ret = 0;
    uint32_t status = HAL_REG_RD(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_REG);
    if (((QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_DIN_RDY_MASK)&status) &&
        !((QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_SW_ERR_MASK |
           QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_DIN_ERR_MASK |
           QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_ERR_INTR_MASK) &
          status)) {
        ret = (uint32_t)((QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_DIN_SIZE_AVAIL_MASK & status) >>
                         QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_DIN_SIZE_AVAIL_OFFSET);
        // NT_LOG_PRINT(MLM, ERR, "crypto_arm_available_in_size:%d\n",ret);
        return ret;
    }
    return ret;
}

/**
 * @brief  Gets the size of data available to read from Data out registers
 * @return Size
 */
uint32_t crypto_arm_available_out_size(void)
{
    uint32_t ret = 0;
    uint32_t status = HAL_REG_RD(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_REG);
    if (((QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_DOUT_RDY_MASK)&status) &&
        !((QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_SW_ERR_MASK |
           QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_DOUT_ERR_MASK |
           QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_ERR_INTR_MASK) &
          status)) {
        ret = (uint32_t)((QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_DOUT_SIZE_AVAIL_MASK & status) >>
                         QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_DOUT_SIZE_AVAIL_OFFSET);
        // NT_LOG_PRINT(MLM, ERR, "crypto_arm_available_out_size:%d\n",ret);
        return ret;
    }
    return ret;
}

/**
 * @brief  Returns 0 or 1 based on errors or operation status
 * @return State
 */
uint32_t crypto_done(void)
{
    uint32_t ret = 0;
    uint32_t status = HAL_REG_RD(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_REG);
    if ((QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_SW_ERR_MASK | QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_DOUT_ERR_MASK |
         QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_ERR_INTR_MASK |
         QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_DIN_ERR_MASK |
         QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_OP_DONE_INTR_MASK) &
        status) {
        ret = 1;
        return ret;
    }
    return ret;
}
/**
 * @brief  Driver for writing data into Data in register and read cipher text from data out register
 * @param  ce_data_in 	: Pointer to in data
 * @param  ce_data_out 	: Pointer to out data
 * @param  din_limit 	: Size of Data in
 * @param  dout_limit 	: Size of Data out
 * @return Error codes
 */
uint32_t crypto_arm3(const uint8_t *ce_data_in, uint32_t ce_data_out[], uint32_t din_limit, uint32_t dout_limit)
{
    uint32_t i = 0, j = 0, written = 0, read = 0;
    uint32_t *p_din = NULL;
    p_din = (uint32_t *)ce_data_in;
    while (i < din_limit) {
        if (crypto_done()) {
            // NT_LOG_PRINT(MLM, ERR, "Crypto done:%d\n",i);
            break;
        }
        for (i = written; i < din_limit; i++) {
            if (crypto_arm_available_in_size() >= (uint32_t)4) {
                HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_DATA_IN_REG, p_din[i]);
                // NT_LOG_PRINT(MLM, ERR, "in:%08x\n",p_din[i]);
            } else {
                written = i;
                break;
            }
        }
        // NT_LOG_PRINT(MLM, ERR, "written:%d\n",written);
        // Wait for Crypto to finish prossessing
        while (!crypto_arm_available_out_size())
            ;

        for (j = read; j < dout_limit; j++) {
            if (crypto_arm_available_out_size() >= (uint32_t)4) {
                ce_data_out[j] = HAL_REG_RD(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_DATA_OUT_REG);
                // NT_LOG_PRINT(MLM, ERR, "out:%08x\n",ce_data_out[j]);
            } else {
                read = j;
                break;
            }
        }
    }
    // NT_LOG_PRINT(MLM, ERR, "status : %08X\n",HAL_REG_RD(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_REG));
    return 1;
}

#endif
