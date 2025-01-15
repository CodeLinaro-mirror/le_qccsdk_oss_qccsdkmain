/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "qapi_rram.h"
#include "ferm_qspi.h"
#include "qurt_mutex.h"

#define RRAM_WRITE_ADR_BYTE_ALIGN 4
#define RRAM_SHELL_INFO 1
#define RRAM_SHELL_GROUP_PRINTF_SUFFIX  "RRAM: "

#if RRAM_SHELL_INFO
#define rram_printf(msg,...)     printf(RRAM_SHELL_GROUP_PRINTF_SUFFIX msg, ##__VA_ARGS__)
#else
#define rram_printf(args...)     do { } while (0)
#endif

#define PART_NUMBER                      10 /**< part map size */

qurt_mutex_t rram_udpart_mutex;     // Mutex to prevent concurrent accesses to the hardware

extern IDAddr fdt_part[PART_NUMBER];
extern uint32_t bdf_addr;

bool rram_udpart_init_done;
uint32_t part_map[PART_NUMBER];

/**
   @brief Verifies if the given RRAM  address range is valid for an RRAM write/read
          operation.

   @param[in] address  Start address for the RRAM write operation.
   @param[in] length   Number of bytes to be written.

   @return true if the address range is valid or false if it is not.
*/
static rram_status_t rram_verify_address(uint32_t address, uint32_t length, uint32_t start_addr, uint32_t end_addr)
{
    rram_status_t ret_val;
    int max_len = bdf_addr - address + 1;

    if ((address >= start_addr) &&
        (address <= end_addr))
    {
        /* Main RRAM region. */
        ret_val = RRAM_OK;
    }
    else
    {
        /* Invalid address*/
        ret_val = RRAM_ADDRESS_ERROR;
        printf("ERROR address: Invalid address %d\n", ret_val);
    }
    if (max_len > length)
    {
        /* Main RRAM region. */
        ret_val = RRAM_OK;
    }
    else
    {
        /* Invalid length. */
        ret_val = RRAM_OFFSET_ERROR;
        printf("ERROR address: Invalid len %d\n", ret_val);
    }

    return (ret_val);
}

/**
   @brief Initialize the rram module.

   This function must be called before any other rram functions.

   @return
   QAPI_OK -- On success. \n
   Error code -- On failure.
*/
qbool_t rram_udpart_init()
{
      
    if (rram_udpart_init_done) {
        return RRAM_DEVICE_DONE;
    }

    rram_udpart_init_done = true;

    qurt_mutex_create(&rram_udpart_mutex);

    return QAPI_OK;
}

/**
   @brief deInitialize the rram module.

   This function must be called before any other rram functions.

   @return
   QAPI_OK -- On success. \n
   Error code -- On failure.
*/
static qbool_t rram_udpart_deinit()
{
    if(!rram_udpart_init_done)
       return RRAM_DEVICE_DONE;

    qurt_mutex_delete(&rram_udpart_mutex);
    rram_udpart_mutex = NULL;
    rram_udpart_init_done = false;

    return QAPI_OK;
}

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
qapi_Status_t qapi_rram_read(uint32_t partid, uint32_t offset, uint8_t *buffer, uint32_t len)
{
    qapi_Status_t status;
    uint32_t address_base;
    uint32_t address_dst;
    int32_t id;

    if (!rram_udpart_init_done) {
        return RRAM_DEVICE_FAIL;
    }

    int array_size = sizeof(fdt_part) / sizeof(IDAddr);
	for (int i = 0; i < array_size; i++) {
        id = fdt_part[i].id;
        part_map[id] = fdt_part[i].addr;
	}

    if (!part_map[partid]) {
        printf("ERROR partiton id: valid range 0-4\n");
        return QAPI_ERR_BOUNDS;
    } else {
        address_base = part_map[partid];
    }

    address_dst = address_base + offset;
    uint32_t end_addr = bdf_addr;

    /* Verify the address and size are valid. */
    if (rram_verify_address(address_dst, len, address_base, end_addr) != RRAM_OK) {
        return QAPI_ERR_BOUNDS;
    }

    qurt_mutex_lock(&rram_udpart_mutex);
    status = nt_rram_read(address_dst, buffer, len);
    if (status != QAPI_OK) {
        printf("dxe rram read dst address error: %08x\n", address_dst);
        return QAPI_ERROR;
    }
    rram_printf("dxe rram read dst address: %08x, buffer: %s \n", address_dst, buffer);
    qurt_mutex_unlock(&rram_udpart_mutex);

    return QAPI_OK;
}

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
qapi_Status_t qapi_rram_write(uint32_t partid, uint32_t offset, uint8_t *buffer, uint32_t len)
{
    qapi_Status_t status;
    uint32_t address_base;
    uint32_t destination;
    uint32_t id;

    if (!rram_udpart_init_done) {
        return RRAM_DEVICE_FAIL;
    }

    if (buffer == NULL) {
        return QAPI_ERROR;
    }

    int array_size = sizeof(fdt_part) / sizeof(IDAddr);
	for (int i = 0; i < array_size; i++) {
        id = fdt_part[i].id;
        part_map[id] = fdt_part[i].addr;
	}

    if (!part_map[partid]) {
        printf("ERROR partiton id: valid range 0-4\n");
        return QAPI_ERR_BOUNDS;
    } else {
        address_base = part_map[partid];
    }

    destination = address_base + offset;
    uint32_t end_addr = bdf_addr;

    /* Verify the address and size are valid. */
    if (rram_verify_address(destination, len, address_base, end_addr) != RRAM_OK) {
        return QAPI_ERR_BOUNDS;
    }

    qurt_mutex_lock(&rram_udpart_mutex);
    status = nt_rram_write(destination, buffer, len);
    if (status != QAPI_OK) {
        printf("dxe rram write dst address error: %08x\n", destination);
        return QAPI_ERROR;
    }
    
    rram_printf("dxe rram write dst address: %08x, data:%s \n", destination, buffer);
    qurt_mutex_unlock(&rram_udpart_mutex);

    return status;
}




