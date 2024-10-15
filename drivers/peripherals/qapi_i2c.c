/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/

/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/

#include <string.h>
#include <stdlib.h>
#include "qapi_i2c.h"
#include "ferm_i2c.h"
#include "nt_osal.h"

#ifdef I2C_QAPI
/** Bus speeds supported by the master implementation. */
#define I2C_STANDARD_MODE_FREQ_KHZ          100     /**< I2C stadard speed 100 KHz. */
#define I2C_FAST_MODE_FREQ_KHZ              400     /**< I2C fast mode speed 400 KHz. */
#define I2C_FAST_MODE_PLUS_FREQ_KHZ         1000    /**< I2C fast mode plus speed 1 MHz */
#define I2C_HIGH_MODE_KHZ					2000    /**< I2C high speed 2 MHz */

/**I2C Error code Mapping*/
#define I2C_SUCCESS(x)  (x == QAPI_OK)
#define I2C_ERROR(x)    (x != QAPI_OK)

#define I2CM_ErrorMap(Status)           (Status == QAPI_OK) ? QAPI_OK : __QAPI_ERROR(QAPI_MOD_I2C, Status)

/**
   @brief Initialize the respective I2C instance.

   The API allocates resources for use by the client handle and the I2C
   instance, and enables power to the I2C HW instance.

   @param[in] Instance  I2C instance that the client intends to initialize.

   @param[in] Config    I2C config.

   @return
   - QAPI_OK                             --  Module was initialized successfully.
   - QAPI_I2CM_ERROR_INVALID_PARAM       --  Invalid instance or handle parameter.
   - QAPI_I2CM_ERROR_BOOTSTRAP_CFG_FAIL  --  Configure bootstrap failure.
   - QAPI_I2CM_ERROR_BUS_CLK_CFG_FAIL	 --  Configure bus clock failure.
   - QAPI_I2CM_ERROR_DEVICE_STATE		 --	 I2C instance is opened.
   - QAPI_I2CM_ERROR_IC_COMP			 --  I2C component type is incorrect.
*/
qapi_Status_t qapi_I2CM_Open(qapi_I2CM_Instance_t Instance, qapi_I2CM_Config_t *Config)
{
	i2c_status i_status;

	//To compatible with previous QAPI, variable Config need to be kept, but we don't need in fermion
	if (Config != NULL)
		Config = NULL;

	i_status = i2c_open((i2c_instance) Instance);

    return I2CM_ErrorMap(i_status);
}

/**
   @brief De-initialize the I2C Master instance.

   The API releases any resources allocated by the qapi_I2CM_Open() API, and
   disable the power to the instance. In this API the transfers which have
   not been completed will be cancelled and also the transfer call back function
   will be called.

   @param[in] Instance  The I2C Master instance to be closed.

   @return
   - QAPI_OK								--  I2C Master driver was closed successfully.
   - QAPI_I2CM_ERROR_INVALID_PARAM			-- Invalid instance parameter.
   - QAPI_I2CM_ERROR_DEVICE_STATE			-- I2C instance is not opened.
   - QAPI_I2CM_ERROR_BOOTSTRAP_CFG_FAIL		-- Config I2C bootstrap failed.
   - QAPI_I2CM_ERROR_BUS_CLK_ENABLE_FAIL	-- Config I2C clock failed.
*/
qapi_Status_t qapi_I2CM_Close(qapi_I2CM_Instance_t Instance)
{
	i2c_status i_status;

	i_status = i2c_close((i2c_instance) Instance);

    return I2CM_ErrorMap(i_status);
}

/**
   @brief Perform an I2C transfer.

   In case a transfer is already in progress by another client, this call
   return a busy error. If the transfer returns a failure, the transfer
   is failure. If the transfer returns QAPI_OK, the transfer has been
   done successfully.

   @note
   In general, if the client wishes to queue mutliple transfers, it could
   use an array of descriptors of type qapi_I2CM_Descriptor_t instead of
   calling the API multiple times. I2C Master driver doesn't support
   queueing mutiple transfers in non-blocking mode, it only support blocking mode

   @param[in] Instance     The I2CM instance.
   @param[in] Config       Transfer configuration.
   @param[in] Desc         I2C transfer descriptor. This can be an array
                           of descriptors.
   @param[in] NumDesc      Number of descriptors in the descriptor array.
   @param[in]  CBFunction   The callback function that is called at the
                            completion of the transfer occurs in non-blocking
                            mode.The call must do minimal processing and must
                            not call any API defined in this file. It should
                            be NULL in blocking mode.
   @param[in]  CBParameter  The context that the client passes here is
                            returned as is in the callback function. It should
                            be NULL in blocking mode.

   @return
   - QAPI_OK									-- I²C Master transfer successful.
   - QAPI_I2CM_ERROR_INVALID_PARAM				-- One or more parameters are invalid.
   - QAPI_I2CM_ERROR_TRANSFER_BUSY				-- I²C Master instance is busy.
   - QAPI_I2CM_ERROR_DEVICE_STATE				-- I2C instance is not opened.
   - QAPI_I2CM_ERROR_TRANSFER_TIMEOUT			-- Transfer timed out.
   - QAPI_I2CM_ERROR_INPUT_FIFO_UNDER_RUN		-- Software reads from an empty Rx FIFO.
   - QAPI_I2CM_ERROR_INPUT_FIFO_OVER_RUN		-- Hardware writes to a full Rx FIFO.
   - QAPI_I2CM_ERROR_OUTPUT_FIFO_OVER_RUN		-- Software writes a new word to a full Tx FIFO.
   - QAPI_I2CM_ERROR_TRANSFER_FORCE_TERMINATED	-- Transfer abort or cancel request by software.
*/
qapi_Status_t qapi_I2CM_Transfer(qapi_I2CM_Instance_t Instance, qapi_I2CM_Transfer_Config_t *Config, qapi_I2CM_Descriptor_t *Desc, uint32_t NumDesc, qapi_I2CM_Transfer_CB_t CBFunction, void *CBParameter)
{
	i2c_status i_status;
	i2c_msg		*msgs, *cur_msg;
	uint8_t	i = 0, num_msgs = NumDesc;
	uint16_t tar_addr;
	uint32_t freq;

	if (Desc == NULL || NumDesc == 0)
		return QAPI_I2CM_ERROR_INVALID_PARAM;

	msgs = (i2c_msg *)nt_osal_allocate_memory(sizeof(i2c_msg) * NumDesc);

	if (msgs == NULL)
		return	QAPI_I2CM_ERROR_MEM_ALLOC;

	cur_msg = msgs;

	for (i = 0; i < NumDesc; i++, cur_msg++, Desc ++) {
		cur_msg->buf = Desc->Buffer;
		cur_msg->len = Desc->Length;
		cur_msg->flag = Desc->Flags;
	}

	if (CBFunction != NULL)
		CBFunction = NULL;

	if (CBParameter != NULL)
		CBParameter = NULL;

	freq = Config->BusFreqKHz;
	tar_addr = (uint16_t) Config->SlaveAddress;

	i_status = i2c_transfer(Instance, msgs, num_msgs,
					tar_addr, freq);

	nt_osal_free_memory(msgs);

    return I2CM_ErrorMap(i_status);
}

/**
   @brief Cancels a transfer.

   A transfer that has been initiated successfully by calling qapi_I2CM_Transfer()
   may be canceled. Based on the internal state of the transfer, this function
   will either immediately cancel the transfer or end the transfer at a later time.

   @param[in] Instance  The I2C instance.

   @return
   - QAPI_OK								-- Transfer is cancelled successfully.
   - QAPI_I2CM_ERROR_INVALID_PARAM			-- One or more parameters are invalid.
   - QAPI_I2CM_CANCEL_TRANSFER_INVALID		-- No transfer to cancel.
   - QAPI_I2CM_ERROR_CANCEL_TRANSFER_FAIL	-- Transfer cancel failed on the bus.
*/
qapi_Status_t qapi_I2CM_Cancel_Transfer(qapi_I2CM_Instance_t Instance)
{

	i2c_status i_status;

	i_status = i2c_cancel_transfer((i2c_instance) Instance);

	return I2CM_ErrorMap(i_status);
}

#endif //I2C_QAPI
