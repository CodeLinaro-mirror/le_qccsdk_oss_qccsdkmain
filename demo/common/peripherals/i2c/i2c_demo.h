/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __I2C_DEMO_H__
#define __I2C_DEMO_H__

/**
   @brief Open I2C Master Instance.

   @param[1] Instance          I2C Master instance.
   @param[2] Blocking          Blocking mode or not.
   @param[3] Dma               DMA or FIFO mode.

   @return
   QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE  - Invalid input paramters.
   QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR  - Open error.
   QAPI_OK - Open successfully.
*/
qapi_Status_t cmd_I2CM_Open(uint32_t __attribute__((__unused__)) Parameter_Count,
                            QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List);

/**
   @brief Perform an I2C Master transfer.

   @param[1]  SlaveType         I2C Slave type.
   @param[2]  Operation         Transfer operattion, "w": write, "r": read, "wr": write and then read in one transfer.
   @param[3]  Instance          I2C Master instance.
   @param[4]  SlaveAddress      7-bit I2C Slave address.
   @param[5]  Frequency         I2C Master bus speed in kHz, only support 100/400/1000.
   @param[6]  Address           The address in slave device to be written to or read from .
   @param[7]  DataLen           The Length of data in bytes.
   @param[8]  Data              Data to be written, only used when write operation.

   @return
   QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE  - Invalid input paramters.
   QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR  - Transfer error.
   QAPI_OK - Transfer successfully.
*/
qapi_Status_t cmd_I2CM_Transfer(uint32_t __attribute__((__unused__)) Parameter_Count,
                                QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List);

/**
   @brief Cancel I2C Master Transfer.

   @param[1] Instance      I2C Master instance.

   @return
   QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE  - Invalid input paramters.
   QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR  - Cancel error.
   QAPI_OK - Cancel successfully.
*/
qapi_Status_t cmd_I2CM_Cancel(uint32_t __attribute__((__unused__)) Parameter_Count,
                              QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List);

/**
   @brief Close I2C Master Instance.

   @param[1] Instance      I2C Master instance.

   @return
   QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE  - Invalid input paramters.
   QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR  - Close error.
   QAPI_OK - Close successfully.
*/
qapi_Status_t cmd_I2CM_Close(uint32_t __attribute__((__unused__)) Parameter_Count,
                             QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List);
#endif
