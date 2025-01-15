/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/
/** @file qapi_rram.h
   @brief Rram Services Interface definition.

      This module provides rram operation APIs.

*/

/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/

#include "qapi_types.h"
#include "qapi_status.h"
#include "ferm_qspi.h"

#define RRAM_DEVICE_DONE                 0 /**< Operation passed */
#define RRAM_DEVICE_FAIL                (-1) /**< Operation failed */

// define struct with id and addr
typedef struct {
    uint32_t id;
    uint32_t addr;
} IDAddr;

typedef enum{
   RRAM_OFFSET_ERROR  = -2,  
	RRAM_ADDRESS_ERROR = -3,
   RRAM_OK = 1,
} rram_status_t;

/*-------------------------------------------------------------------------
 * Function Declarations
 *-----------------------------------------------------------------------*/
/**
   @brief Read data from the rram.

   @param[in]  partid    The partid to map the ud part base addrss in sbl.
   @param[in]  offset    The rram address to start to read from.
   @param[in]  len       Number of bytes to read.
   @param[out] buffer    Data buffer for a rram read operation.

   @return
   QAPI_OK -- If a read completed successfully. \n
   Error code -- If there is an error.
*/
qapi_Status_t qapi_rram_read(uint32_t partid, uint32_t offset, uint8_t *buffer, uint32_t len);

/**
   @brief Write data to the rram.

   @param[in] partid    The partid to map the ud part base addrss in sbl.
   @param[in] offset    The rram address to start to write to.
   @param[in] len       Number of bytes to write.
   @param[in] buffer    Data buffer containing data to be written.

   @return
   QAPI_OK -- If blocking write completed successfully. \n
   Error code -- If there is an error.
*/
qapi_Status_t qapi_rram_write(uint32_t partid, uint32_t offset, uint8_t *buffer, uint32_t len);



