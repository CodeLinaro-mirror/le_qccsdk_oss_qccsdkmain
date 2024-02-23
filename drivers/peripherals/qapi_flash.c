
#include "qapi_flash.h"
#include "ferm_flash.h"

#define Flash_ErrorMap(Status)   (Status == QAPI_OK) ? QAPI_OK : __QAPI_ERROR(QAPI_MOD_FLASH, Status)

/**
   @brief Initialize the flash module.

   This function must be called before any other flash functions.

   @return
   QAPI_OK -- On success. \n
   Error code -- On failure.
*/
qapi_Status_t qapi_Flash_Init()
{
    FLASH_STATUS status;

    status = drv_flash_init();

    return Flash_ErrorMap(status);
}

/**
   @brief Read data from the flash.

   @param[in]  Address    The flash address to start to read from.
   @param[in]  ByteCnt    Number of bytes to read.
   @param[out] Buffer     Data buffer for a flash read operation.

   @return
   QAPI_OK -- If a read completed successfully. \n
   Error code -- If there is an error.
*/
qapi_Status_t qapi_Flash_Read(uint32_t Address, uint32_t ByteCnt, uint8_t *Buffer)
{
    FLASH_STATUS status;

    status = drv_flash_read(Address, ByteCnt, Buffer, NULL, NULL);

    return Flash_ErrorMap(status);
}

/**
   @brief Write data to the flash.

   @param[in] Address    The flash address to start to write to.
   @param[in] ByteCnt    Number of bytes to write.
   @param[in] Buffer     Data buffer containing data to be written.

   @return
   QAPI_OK -- If blocking write completed successfully. \n
   Error code -- If there is an error.
*/
qapi_Status_t qapi_Flash_Write(uint32_t Address, uint32_t ByteCnt, uint8_t *Buffer)
{
    FLASH_STATUS status;

    status = drv_flash_write(Address, ByteCnt, Buffer, NULL, NULL);

    return Flash_ErrorMap(status);
}

/**
   @brief Erase the given flash blocks or bulks, or the whole chip.

   @param[in] EraseType  Specify the erase type.
   @param[in] Start      For block erase - the starting block of a
                         number of blocks to erase.
                         For bulk erase - the starting bulk of a number
                         of bulks to erase.
                         For chip erase, it should be 0.
   @param[in] Cnt        For block erase - the number of blocks to erase.
                         For bulk erase - the number of bulks to erase.
                         For chip erase, it should be 1.
   @return
   QAPI_OK -- If blocking erase completed successfully. \n
   Error code -- If there was an error.
*/
qapi_Status_t qapi_Flash_Erase(qapi_FLASH_Erase_Type_t EraseType, uint32_t Start, uint32_t Cnt)
{
    FLASH_STATUS status;

    status = drv_flash_erase(EraseType, Start, Cnt, NULL, NULL);

    return Flash_ErrorMap(status);
}

/**
   @brief Read flash registers.

   @param[in]  RegOpcode  Operation code.
   @param[in]  Len        The length of register value to be read.
   @param[out] RegValue   The read out value.

   @return
   QAPI_OK -- On success. \n
   Error code -- On failure.
*/
qapi_Status_t qapi_Flash_Read_Reg(uint8_t RegOpcode, uint8_t Len, uint8_t *RegValue)
{
    FLASH_STATUS status;

    status = drv_flash_read_reg(RegOpcode, Len, RegValue);

    return Flash_ErrorMap(status);
}

/**
   @brief Write flash registers.

   @param[in] RegOpcode   Operation code.
   @param[in] Len         The length of register value to be written.
   @param[in] RegValue    The written value.

   @return
   QAPI_OK -- If blocking writereg completed successfully. \n
   Error code -- If there was an error.
*/
qapi_Status_t qapi_Flash_Write_Reg(uint8_t RegOpcode, uint8_t Len, uint8_t *RegValue)
{
    FLASH_STATUS status;

    status = drv_flash_write_reg(RegOpcode, Len, RegValue, NULL, NULL);

    return Flash_ErrorMap(status);
}

