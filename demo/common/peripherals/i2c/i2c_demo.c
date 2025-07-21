/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "qapi_types.h"
#include "qapi_status.h"
#include "qapi_i2c.h"
#include "qcli_api.h"
#include "qapi_console.h"
#include "timer.h"
#include "i2c_demo.h"
#include "uart.h"
#include "nt_osal.h"
#include "safeAPI.h"

#ifdef I2C_DEMO
/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 *-----------------------------------------------------------------------*/
#ifdef I2C_DEMO_DBG
static char I2COutputBuffer[120];
#define I2CM_PRINTF(...)                                             \
    snprintf(I2COutputBuffer, sizeof(I2COutputBuffer), __VA_ARGS__); \
    nt_dbg_print(I2COutputBuffer);
#else
#define I2CM_PRINTF(x, ...)
#endif

/** Bus speeds supported by the master implementation. */
#define I2C_STANDARD_MODE_FREQ_KHZ  100  /**< I2C stadard speed 100 KHz. */
#define I2C_FAST_MODE_FREQ_KHZ      400  /**< I2C fast mode speed 400 KHz. */
#define I2C_FAST_MODE_PLUS_FREQ_KHZ 1000 /**< I2C fast mode plus speed 1 MHz */
#define I2C_HIGH_MODE_KHZ           2000 /**< I2C high speed 2 MHz */

#define I2C_FLAGS_START_WIRTE      QAPI_I2C_FLAG_START | QAPI_I2C_FLAG_WRITE;
#define I2C_FLAGS_START_READ       QAPI_I2C_FLAG_START | QAPI_I2C_FLAG_READ;
#define I2C_FLAGS_START_WIRTE_STOP QAPI_I2C_FLAG_START | QAPI_I2C_FLAG_WRITE | QAPI_I2C_FLAG_STOP;
#define I2C_FLAGS_START_READ_STOP  QAPI_I2C_FLAG_START | QAPI_I2C_FLAG_READ | QAPI_I2C_FLAG_STOP;

#define I2C_MAX_TRANSFER_NUMBER 4

#define I2C_MAX_TRANSFER_DATA_LENGTH       128 /*Max Data Length since memory limitation */
#define I2C_SLAVE_MAX_TRANSFER_DATA_LENGTH 256 /*Max Data Length since memory limitation for i2c slave demo */

#ifdef CONFIG_BOARD_EVB_I2C_OPTION
/** EVB EEPROM memory size */
#define I2C_SLAVE_EEPROM_MEMORY_SIZE 4096
#define I2C_SLAVE_EEPROM_PAGE_SIZE   32

#else
/** MQM EEPROM memory size */
#define I2C_SLAVE_EEPROM_MEMORY_SIZE 256
#define I2C_SLAVE_EEPROM_PAGE_SIZE   8
#endif

/** NFC Tag NT3H2111 specific definitions */
#define NT3H2111_I2C_ADDRESS (0xAA >> 1)

/** NFC PIO */
#define NFC_VDD_PIO QAPI_GPIO_ID7_E

/** NT3H2111 I2C registers */
#define REG_CAPABILITY_CONTAINER (0x00)
#define REG_START_BLOCK          (0x01)
#define REG_CONFIG               (0x7A)
#define REG_SESSION              (0xFE)

#define BYTES_PER_I2C_BLOCK (0x10)
#define WORDS_PER_I2C_BLOCK (BYTES_PER_I2C_BLOCK >> 1)

/** NDEF definition */
#define NDEF_MAX_SIZE         (0x20)
#define NFC_TAG_PHONE_NUM_LEN (12)

#define NDEF_MSG_TLV        (0x3)
#define NDEF_TERMINATOR_TLV (0xFE)

/** NDEF MSG URI ID Code */
#define URI_HTTP_WWW_ABBR_ID  (0x01)  // http://www.
#define URI_HTTPS_WWW_ABBR_ID (0x02)  // https://www.

/*-------------------------------------------------------------------------
 * Type Declarations
 *-----------------------------------------------------------------------*/
typedef enum {
    I2C_MASTER_INSTANCE_CLOSED,
    I2C_MASTER_INSTANCE_READY,
    I2C_MASTER_INSTANCE_TRANSFER_QUEUE_FULL,
} I2CM_Ctxt_State_t;

typedef enum {
    I2C_MASTER_TRANSFER_WRITE,
    I2C_MASTER_TRANSFER_READ,
    I2C_MASTER_TRANSFER_WRITE_READ,
} I2CM_OpCode_t;

typedef struct I2CM_Transfer_s {
    qapi_I2CM_Transfer_Config_t Config;
    I2CM_OpCode_t OpCode;
    uint32_t NumDesc;
    qapi_I2CM_Descriptor_t *Desc;
    qapi_I2CM_Transfer_CB_t CBFunction;
    void *CBParameter;
    I2CM_Ctxt_State_t State;
    struct I2CM_Transfer_s *Next;
} I2CM_Transfer_t;

typedef enum {
    I2C_SlAVE_TYPE_EEPROM,
    I2C_SlAVE_TYPE_NFC,
    I2C_SlAVE_TYPE_RAWSLAVE,
} I2C_Slave_Type_t;

typedef struct I2CM_Ctxt_s {
    I2C_Slave_Type_t SlaveType;
    qapi_I2CM_Config_t Config;
    I2CM_Ctxt_State_t State;
    I2CM_Transfer_t *Transfer;
    uint32_t TransferNum;
} I2C_Master_Context_t;

/*-------------------------------------------------------------------------
 * Static & global Variable Declarations
 *-----------------------------------------------------------------------*/

/**
   I2CM Client.
*/
I2C_Master_Context_t I2CM_Ctxt[QAPI_I2C_INSTANCE_NUM];

static void cmd_I2CM_OpenHelp(void)
{
    I2CM_PRINTF("Usage:\tI2CM Open <Instance> <Blocking> <Dma>\r\n");
    I2CM_PRINTF("Options:\r\n");
    I2CM_PRINTF("\tInstance      : 0 I2C Master instancei, we only support one instance on Fermion\r\n");
    I2CM_PRINTF("\tBlocking      : 0: Nonblocking mode, 1: Blocking mode\r\n");
    I2CM_PRINTF("\tDma           : 0: FIFO mode, 1: DMA mode\r\n");
    I2CM_PRINTF("\tExample:\r\n");
    I2CM_PRINTF("\tI2CM Open 0 0 0\r\n");
}

static void cmd_I2CM_TransferHelp(void)
{
    I2CM_PRINTF(
        "Usage:\tI2CM Transfer <SlaveType> <Operation> <Instance> <SlaveAddress> <Frequency> <Address> <DataLen> "
        "<Data>\r\n");
    I2CM_PRINTF("Options:\r\n");
    I2CM_PRINTF("\tSlaveType     : support 'eeprom' and 'rawslave'\r\n");
    I2CM_PRINTF("\tOperation     : 'w','r' and 'wr'. w: write, r: read, wr: write and read in one transfer\r\n");
    I2CM_PRINTF("\tInstance      : 0 I2C Master instance, we only support one instance on Fermion\r\n");
    I2CM_PRINTF("\tSlaveAddress  : 7 bit I2C Slave address\r\n");
    I2CM_PRINTF("\tFrequency     : 100/400/1000 kHz\r\n");
    I2CM_PRINTF(
        "\tAddress       : The address which the data to be written to or read from, for rawslave this will be "
        "ignore\r\n");
    I2CM_PRINTF("\tDataLen       : The Length of data to be written or read\r\n");
    I2CM_PRINTF("\tData          : Data to be written, only used when write operation\r\n");
    I2CM_PRINTF("\tExample:\r\n");
    I2CM_PRINTF("\tI2CM Transfer eeprom w 0 0x50 400 0 8 aabbccdd\r\n");
}

static void cmd_I2CM_CancelHelp(void)
{
    I2CM_PRINTF("Usage:\tI2CM Cancel <Instance>\r\n");
    I2CM_PRINTF("Options:\r\n");
    I2CM_PRINTF("\tInstance      : 0 I2C Master instancei, we only support one instance on Fermion\r\n");
    I2CM_PRINTF("\tExample:\r\n");
    I2CM_PRINTF("\tI2CM Cancel 0\r\n");
}

static void cmd_I2CM_CloseHelp(void)
{
    I2CM_PRINTF("Usage:\tI2CM Close <Instance>\r\n");
    I2CM_PRINTF("Options:\r\n");
    I2CM_PRINTF("\tInstance      : 0 I2C Master instancei, we only support one instance on Fermion\r\n");
    I2CM_PRINTF("\tExample:\r\n");
    I2CM_PRINTF("\tI2CM Close 0\r\n");
}

static uint32_t I2CM_GetDescNum(I2C_Slave_Type_t SlaveType, I2CM_OpCode_t Op, uint32_t DataAddr, uint32_t DataLen)
{
    uint8_t NumDesc = 0;
    uint8_t Offset;

    if (SlaveType == I2C_SlAVE_TYPE_RAWSLAVE) {
        /* the raw i2c slave demo only need one Descriptor per time */
        NumDesc = 1;
        return NumDesc;
    }

    if (Op == I2C_MASTER_TRANSFER_READ) {
        /** one data address descriptor + one read descriptor*/
        NumDesc = 2;
        return NumDesc;
    }

    if (SlaveType == I2C_SlAVE_TYPE_EEPROM) {
        Offset = I2C_SLAVE_EEPROM_PAGE_SIZE - DataAddr % I2C_SLAVE_EEPROM_PAGE_SIZE;
        if ((Offset > 0) && (DataLen >= Offset)) {
            NumDesc++;
            DataLen -= Offset;
        }

        NumDesc += DataLen / I2C_SLAVE_EEPROM_PAGE_SIZE;

        if (DataLen % I2C_SLAVE_EEPROM_PAGE_SIZE > 0) {
            NumDesc++;
        }

        if (Op == I2C_MASTER_TRANSFER_WRITE_READ) {
            /** write Numdesc +  read Numdesc*/
            NumDesc = NumDesc + 2;
        }
    } else if (SlaveType == I2C_SlAVE_TYPE_NFC) {
        NumDesc = 1;
    }
    // TODO for i2cslave type NumDesc = 1

    return NumDesc;
}

static void I2CM_InitEepromDescriptors(I2CM_OpCode_t OpCode, uint32_t Address, uint8_t *Data, uint32_t DataLen,
                                       qapi_I2CM_Descriptor_t *Desc, uint32_t NumDesc)
{
    uint32_t i;
    uint8_t *Buffer;
    uint32_t BufferOffset = 0;
    uint32_t DescIdx = 0;
    uint32_t DataOffset = 0;
    uint32_t WriteLen = 0;
    uint32_t StartPageSize = 0;
    uint32_t AddressOffset = 0;
    static uint32_t Idx = 0;

    Buffer = (uint8_t *)Desc + sizeof(qapi_I2CM_Descriptor_t) * NumDesc;
    /*If Datatype is string, Print it directly at the customer call back function*/
    if (OpCode == I2C_MASTER_TRANSFER_WRITE_READ) {
        Data = Buffer;
        Idx = Idx % 10;
        Idx++;
        memset(Data, Idx + 0x30, NumDesc * 2 + DataLen);
    }

    if (OpCode == I2C_MASTER_TRANSFER_READ) {
    I2C_EEPROM_READ:
        /*Init Read Descriptor*/
        Desc[DescIdx].Flags = I2C_FLAGS_START_WIRTE;
        Desc[DescIdx].Buffer = Buffer + BufferOffset;
#ifdef CONFIG_BOARD_EVB_I2C_OPTION
        Desc[DescIdx].Length = 2;
        Desc[DescIdx].Buffer[0] = (Address & 0xFF00) >> 8;
        Desc[DescIdx].Buffer[1] = Address & 0xFF;
        BufferOffset += 2;

#else
        Desc[DescIdx].Length = 1;
        Desc[DescIdx].Buffer[0] = Address & 0xFF;
        BufferOffset += 1;
#endif

        Desc[DescIdx + 1].Flags = I2C_FLAGS_START_READ_STOP;
        Desc[DescIdx + 1].Length = DataLen;
        Desc[DescIdx + 1].Buffer = Buffer + BufferOffset;
        return;
    }

    if (OpCode == I2C_MASTER_TRANSFER_WRITE_READ) {
        /** The Write Descriptor Number = Total Descriptor Number - Read Descriptor Number*/
        NumDesc -= 2;
    }

    /*Init Write Descriptor*/
    for (i = 0; i < NumDesc; i++) {
        Desc[i].Flags = I2C_FLAGS_START_WIRTE_STOP;
        if (i == 0) {
            /*EEPROM only write one page at one time*/
            StartPageSize = I2C_SLAVE_EEPROM_PAGE_SIZE - Address % I2C_SLAVE_EEPROM_PAGE_SIZE;
            WriteLen = (DataLen > StartPageSize ? StartPageSize : DataLen);
        } else if (i == NumDesc - 1) {
            WriteLen = DataLen - DataOffset;
        } else {
            WriteLen = I2C_SLAVE_EEPROM_PAGE_SIZE;
        }

        /*Data address descriptor*/
        Desc[i].Buffer = Buffer + BufferOffset;
#ifdef CONFIG_BOARD_EVB_I2C_OPTION
        Desc[i].Buffer[0] = ((Address + AddressOffset) & 0xFF00) >> 8;
        Desc[i].Buffer[1] = (Address + AddressOffset) & 0xFF;
        Desc[i].Length = WriteLen + 2;

#else
        Desc[i].Buffer[0] = (Address + AddressOffset) & 0xFF;
        Desc[i].Length = WriteLen + 1;
#endif

        if (OpCode == I2C_MASTER_TRANSFER_WRITE) {
#ifdef CONFIG_BOARD_EVB_I2C_OPTION
            memscpy(&(Desc[i].Buffer[2]), WriteLen, Data + DataOffset, WriteLen);
#else
            memscpy(&(Desc[i].Buffer[1]), WriteLen, Data + DataOffset, WriteLen);
#endif
        }
        DataOffset += WriteLen;
        BufferOffset += Desc[i].Length;
        AddressOffset += WriteLen;
    }

    if (OpCode == I2C_MASTER_TRANSFER_WRITE_READ) {
        /** The last two descriptor is for read operation*/
        DescIdx = i;
        goto I2C_EEPROM_READ;
    }
}

static void I2CM_InitNfcDescriptors(I2CM_OpCode_t OpCode, uint32_t Address, uint8_t *Data, uint32_t DataLen,
                                    qapi_I2CM_Descriptor_t *Desc, uint32_t NumDesc)
{
    uint8_t *Buffer;
    uint32_t BufferOffset = 0;

    Buffer = (uint8_t *)Desc + sizeof(qapi_I2CM_Descriptor_t) * NumDesc;

    if (OpCode == I2C_MASTER_TRANSFER_READ) {
        /*Init Read Descriptor*/
        Desc[0].Flags = I2C_FLAGS_START_WIRTE_STOP;
        Desc[0].Length = 1;
        Desc[0].Buffer = Buffer;
        Desc[0].Buffer[0] = Address & 0xFF;

        BufferOffset += 1;
        Desc[1].Flags = I2C_FLAGS_START_READ_STOP;
        Desc[1].Length = DataLen;
        Desc[1].Buffer = Buffer + BufferOffset;
    } else if (OpCode == I2C_MASTER_TRANSFER_WRITE) {
        /*Init Write Descriptor*/
        Desc[0].Flags = I2C_FLAGS_START_WIRTE_STOP;
        Desc[0].Length = 1 + DataLen;
        Desc[0].Buffer = Buffer;
        Desc[0].Buffer[0] = Address & 0xFF;
        memscpy(&(Desc[0].Buffer[1]), DataLen, Data, DataLen);
    }
}

static void I2CM_InitRawSlaveDescriptors(I2CM_OpCode_t OpCode, uint8_t *Data, uint32_t DataLen,
                                         qapi_I2CM_Descriptor_t *Desc)
{
    uint8_t *Buffer;

    Buffer = (uint8_t *)Desc + sizeof(qapi_I2CM_Descriptor_t);

    Desc->Length = DataLen;
    Desc->Buffer = Buffer;

    if (OpCode == I2C_MASTER_TRANSFER_READ) {
        /*Init Read Descriptor*/
        Desc->Flags = I2C_FLAGS_START_READ_STOP;
    } else if (OpCode == I2C_MASTER_TRANSFER_WRITE) {
        /*Init Write Descriptor*/
        Desc->Flags = I2C_FLAGS_START_WIRTE_STOP;
        memscpy(Buffer, DataLen, Data, DataLen);
    }
}

static void I2CM_InitDescriptors(I2C_Slave_Type_t SlaveType, I2CM_OpCode_t OpCode, uint32_t Address, uint8_t *Data,
                                 uint32_t DataLen, qapi_I2CM_Descriptor_t *Desc, uint32_t NumDesc)
{
    if (SlaveType == I2C_SlAVE_TYPE_EEPROM) {
        I2CM_InitEepromDescriptors(OpCode, Address, Data, DataLen, Desc, NumDesc);
    } else if (SlaveType == I2C_SlAVE_TYPE_NFC) {
        I2CM_InitNfcDescriptors(OpCode, Address, Data, DataLen, Desc, NumDesc);
    } else if (SlaveType == I2C_SlAVE_TYPE_RAWSLAVE) {
        /* The raw i2c slave will ignore the Address parameter and only has one descriptor */
        I2CM_InitRawSlaveDescriptors(OpCode, Data, DataLen, Desc);
    }
}

#ifdef I2C_DEMO_DBG
static void I2CM_PrintTransferResult(uint32_t Status, I2CM_Transfer_t *Transfer)
{
    uint32_t i;
    I2CM_OpCode_t OpCode;
    qapi_I2CM_Descriptor_t *Desc;
    uint32_t NumDesc;
    uint32_t SucNumDesc = 0;
    uint8_t *Data = NULL;
    uint32_t DataLen = 0;
    char *OpStr = "Read";
    char *StaStr;
    uint32_t SlaveAddress;
    uint32_t BusFreqKHz;
    int DataOffSet = 0;

    OpCode = Transfer->OpCode;
    Desc = Transfer->Desc;
    NumDesc = Transfer->NumDesc;
    SlaveAddress = Transfer->Config.SlaveAddress;
    BusFreqKHz = Transfer->Config.BusFreqKHz;
    if ((Status == QAPI_OK)) {
        if (OpCode == I2C_MASTER_TRANSFER_WRITE) {
            Data = Desc[0].Buffer;
            DataLen = 0;
            for (i = 0; i < NumDesc; i++) {
                if (Desc[i].Length > 0) {
                    DataLen += Desc[i].Length;
                    SucNumDesc++;
                }
            }
            /** Minus the length of EEPROM Address*/
#ifdef CONFIG_BOARD_EVB_I2C_OPTION
            DataLen -= SucNumDesc * 2;
#else
            DataLen -= SucNumDesc;
#endif
            OpStr = "Write";
        } else if (OpCode == I2C_MASTER_TRANSFER_READ) {
            Data = Desc[1].Buffer;
            DataLen = Desc[1].Length;
            OpStr = "Read";
        } else if (OpCode == I2C_MASTER_TRANSFER_WRITE_READ) {
            Data = Desc[NumDesc - 1].Buffer;
            DataLen = Desc[NumDesc - 1].Length;
            OpStr = "WriteRead";
            for (i = 0; i < NumDesc - 2; i++) {
                if (memcmp(Desc[i].Buffer + 2, Data + DataOffSet, Desc[i].Length - 2)) {
                    Status = QAPI_ERROR;
                    break;
                }
                DataOffSet += Desc[i].Length - 2;
            }
        }
    }
    StaStr = (Status == QAPI_OK) ? "Suc" : "Fail";

    I2CM_PRINTF("I2C SLV 0x%x Freq %d Khz %s %s Err %d.\r\n ", (unsigned int)SlaveAddress, (int)BusFreqKHz, OpStr,
                StaStr, (int)Status);

    if (DataLen > 0) {
        I2CM_PRINTF("%s Data Length %d.\r\n", OpStr, (int)DataLen);
        if (OpCode == I2C_MASTER_TRANSFER_READ) {
            I2CM_PRINTF("Data: ");
            for (i = 0; i < DataLen; i++) {
                I2CM_PRINTF("%c", Data[i]);
            }
            I2CM_PRINTF("\r\n");
        }
    }

    return;
}

static void I2CM_PrintRawSlaveTransferResult(uint32_t Status, I2CM_Transfer_t *Transfer)
{
    uint32_t i;
    char *OpStr = "Read";
    char *StaStr;
    uint32_t SlaveAddress;
    uint32_t BusFreqKHz;
    uint32_t DataLen = 0;
    uint8_t *Data = NULL;
    qapi_I2CM_Descriptor_t *Desc;
    I2CM_OpCode_t OpCode;

    OpCode = Transfer->OpCode;
    SlaveAddress = Transfer->Config.SlaveAddress;
    BusFreqKHz = Transfer->Config.BusFreqKHz;
    Desc = Transfer->Desc;

    StaStr = (Status == QAPI_OK) ? "Suc" : "Fail";
    OpStr = (OpCode == I2C_MASTER_TRANSFER_WRITE) ? "Write" : "Read";

    Data = Desc->Buffer;
    DataLen = 0;

    if (Status == QAPI_OK)
        DataLen = Desc->Length;

    I2CM_PRINTF("I2C SLV 0x%02X Freq %d Khz %s byte %s Err %d.\r\n ", (unsigned int)SlaveAddress, (int)BusFreqKHz,
                OpStr, StaStr, (int)Status);

    if (DataLen > 0) {
        I2CM_PRINTF("%s Data Length %d.\r\n", OpStr, (int)DataLen);
        if (OpCode == I2C_MASTER_TRANSFER_READ) {
            I2CM_PRINTF("Data: ");
            for (i = 0; i < DataLen; i++) {
                I2CM_PRINTF("%x ", Data[i]);
            }
            I2CM_PRINTF("\r\n");
        }
    }

    return;
}
#endif

static qapi_Status_t I2CM_CheckTransferParameters(uint32_t Instance, uint32_t Frequency, I2C_Slave_Type_t SlaveType,
                                                  uint16_t Address, uint32_t DataLen)
{
    if (Instance > QAPI_I2C_INSTANCE_SE0_E) {
        I2CM_PRINTF("Instance should be 0.\r\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
    }

    if (I2CM_Ctxt[Instance].State == I2C_MASTER_INSTANCE_CLOSED) {
        I2CM_PRINTF("Please open I2C Master Instance %d at first.\r\n", (int)Instance);
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
    }

    if (I2CM_Ctxt[Instance].State == I2C_MASTER_INSTANCE_TRANSFER_QUEUE_FULL) {
        I2CM_PRINTF("Transfer queue is full, please try it later.\r\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
    }

    if ((Frequency != I2C_STANDARD_MODE_FREQ_KHZ) && (Frequency != I2C_FAST_MODE_FREQ_KHZ) &&
        (Frequency != I2C_FAST_MODE_PLUS_FREQ_KHZ) && (Frequency != I2C_HIGH_MODE_KHZ)) {
        I2CM_PRINTF("The Frequency only support 100/400/1000/2000 kHz.\r\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
    }

    if ((SlaveType == I2C_SlAVE_TYPE_EEPROM) && (Address + DataLen > I2C_SLAVE_EEPROM_MEMORY_SIZE)) {
        I2CM_PRINTF("The size of EEPROM is %d, please reduce the Address or DataLen.\r\n",
                    I2C_SLAVE_EEPROM_MEMORY_SIZE);
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
    }

    return QAPI_OK;
}

static I2CM_Transfer_t *I2CM_PrepareTransferCtxt(qapi_I2CM_Instance_t Instance, I2CM_OpCode_t OpCode, uint32_t Address,
                                                 uint32_t DataLen, uint8_t *Data)
{
    qapi_I2CM_Descriptor_t *Desc = NULL;
    I2CM_Transfer_t *Transfer = NULL;
    I2C_Slave_Type_t SlaveType;
    uint8_t *Buffer = NULL;
    uint32_t BufferLen;
    uint32_t NumDesc;

    SlaveType = I2CM_Ctxt[Instance].SlaveType;
    NumDesc = I2CM_GetDescNum(SlaveType, OpCode, Address, DataLen);
    BufferLen = sizeof(I2CM_Transfer_t) + sizeof(qapi_I2CM_Descriptor_t) * NumDesc +
                DataLen; /*transfer struct: descriptor*NumDesc: DataLen*/
    if (SlaveType == I2C_SlAVE_TYPE_EEPROM) {
        if (OpCode == I2C_MASTER_TRANSFER_WRITE) {
#ifdef CONFIG_BOARD_EVB_I2C_OPTION
            BufferLen += NumDesc * 2; /*Each EEPROM Page write need 2 bytes address space*/
#else
            BufferLen += NumDesc; /*Each EEPROM Page write need 1 bytes address space*/
#endif
        } else if (OpCode == I2C_MASTER_TRANSFER_READ) {
#ifdef CONFIG_BOARD_EVB_I2C_OPTION
            BufferLen += 2; /* eeprom: 2 bytes address,  nfc: 1 byte address */

#else
            BufferLen += 1;       /* eeprom: 1 bytes address,  nfc: 1 byte address */
#endif
        } else if (OpCode == I2C_MASTER_TRANSFER_WRITE_READ) {
            BufferLen += NumDesc * 2 + 2 + DataLen; /* write page address + read address + read buffer lenght*/
        }
    }

    // Buffer = (uint8_t *)malloc(BufferLen);
    Buffer = (uint8_t *)nt_osal_calloc(BufferLen, sizeof(char));
    if (Buffer == NULL) {
        I2CM_PRINTF("I2C Master transfer failed, No enough memory.\r\n");
        return NULL;
    }
    memset(Buffer, 0, BufferLen);
    Transfer = (I2CM_Transfer_t *)Buffer;
    Desc = (qapi_I2CM_Descriptor_t *)(Buffer + sizeof(I2CM_Transfer_t));
    I2CM_InitDescriptors(SlaveType, OpCode, Address, Data, DataLen, Desc, NumDesc);
    Transfer->OpCode = OpCode;
    Transfer->NumDesc = NumDesc;
    Transfer->Desc = Desc;

    return Transfer;
}

#ifdef I2C_DEMO_DBG
static void I2CM_DumpTransferDesc(I2CM_Transfer_t *Transfer)
{
    qapi_I2CM_Descriptor_t *Desc;
    uint32_t i = 0, j = 0;

    if (Transfer == NULL)
        return;

    I2CM_PRINTF("I2CM Dump Transfer:\r\n");
    I2CM_PRINTF("	Slave address: 0x%x\r\n", (uint16_t)(Transfer->Config.SlaveAddress));
    I2CM_PRINTF("	Freq KHZ: %d\r\n", (int)Transfer->Config.BusFreqKHz);
    I2CM_PRINTF("	Op Code: %d\r\n", (int)Transfer->OpCode);

    for (i = 0; i < Transfer->NumDesc; i++) {
        Desc = &(Transfer->Desc[i]);
        I2CM_PRINTF("	Desc[%d]:\r\n", (int)i);
        I2CM_PRINTF("		lenth: %d\r\n", (int)Desc->Length);
        I2CM_PRINTF("		flag: %d\r\n", (int)Desc->Flags);
        I2CM_PRINTF("		buffer:");
        for (j = 0; j < Desc->Length; j++) {
            I2CM_PRINTF("0x%x", Desc->Buffer[j]);
        }
        I2CM_PRINTF("\r\n");
    }
    return;
}
#endif

static qapi_Status_t I2CM_TransferHandler(qapi_I2CM_Instance_t Instance, uint32_t SlaveAddress, uint32_t Frequency,
                                          I2CM_OpCode_t OpCode, uint32_t Address, uint32_t DataLen, uint8_t *Data)
{
    qapi_I2CM_Transfer_Config_t *Config;
    I2CM_Transfer_t *Transfer = NULL;
    qapi_Status_t Status;

    Transfer = I2CM_PrepareTransferCtxt(Instance, OpCode, Address, DataLen, Data);
    if (Transfer == NULL) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
    }

    Config = &Transfer->Config;
    Config->SlaveAddress = SlaveAddress;
    Config->BusFreqKHz = Frequency;
    if ((I2CM_Ctxt[Instance].SlaveType == I2C_SlAVE_TYPE_EEPROM) ||
        (I2CM_Ctxt[Instance].SlaveType == I2C_SlAVE_TYPE_NFC) ||
        (I2CM_Ctxt[Instance].SlaveType == I2C_SlAVE_TYPE_RAWSLAVE)) {
        Config->Delay = 5;
    }
#ifdef I2C_DEMO_DBG
    I2CM_DumpTransferDesc(Transfer);
#endif

    Status = qapi_I2CM_Transfer(Instance, Config, Transfer->Desc, Transfer->NumDesc, NULL, NULL);

#ifdef I2C_DEMO_DBG
    if ((I2CM_Ctxt[Instance].SlaveType == I2C_SlAVE_TYPE_NFC) && (Status == QAPI_OK) &&
        (OpCode == I2C_MASTER_TRANSFER_READ)) {
        memscpy(Data, Transfer->Desc[1].Length, Transfer->Desc[1].Buffer, Transfer->Desc[1].Length);
    }
    if (I2CM_Ctxt[Instance].SlaveType == I2C_SlAVE_TYPE_EEPROM) {
        I2CM_PrintTransferResult(Status, Transfer);
    } else if (I2CM_Ctxt[Instance].SlaveType == I2C_SlAVE_TYPE_RAWSLAVE) {
        I2CM_PrintRawSlaveTransferResult(Status, Transfer);
    }
#endif  // I2C_DEMO_DBG

    nt_osal_free_memory(Transfer);

    I2CM_Ctxt[Instance].Transfer = NULL;
    I2CM_Ctxt[Instance].State = I2C_MASTER_INSTANCE_READY;

    return Status;
}

static qapi_Status_t NFC_ReadCapabilityContainer(qapi_I2CM_Instance_t Instance)
{
    int i;
    uint16_t Data[WORDS_PER_I2C_BLOCK] = {0};
    qapi_Status_t Status;

    Status = I2CM_TransferHandler(Instance, NT3H2111_I2C_ADDRESS, I2C_FAST_MODE_FREQ_KHZ, I2C_MASTER_TRANSFER_READ,
                                  REG_CAPABILITY_CONTAINER, BYTES_PER_I2C_BLOCK, (uint8_t *)Data);

    I2CM_PRINTF("Read CC: %d\r\n", (int)Status);
    if (QAPI_OK == Status) {
        for (i = 0; i < WORDS_PER_I2C_BLOCK; i++) {
            I2CM_PRINTF("0x%04x, ", Data[i]);
        }
        I2CM_PRINTF("\r\n");

        /* initialize CC if needed */
        if ((Data[WORDS_PER_I2C_BLOCK - 1] == 0) && (Data[WORDS_PER_I2C_BLOCK - 2] == 0)) {
            I2CM_PRINTF("Initialize CC ");

            /* MUST write back I2C slave address */
            Data[0] = 0x00aa;
            /* content of CC */
            Data[WORDS_PER_I2C_BLOCK - 2] = 0x10e1;
            Data[WORDS_PER_I2C_BLOCK - 1] = 0x006f;
            Status =
                I2CM_TransferHandler(Instance, NT3H2111_I2C_ADDRESS, I2C_FAST_MODE_FREQ_KHZ, I2C_MASTER_TRANSFER_WRITE,
                                     REG_CAPABILITY_CONTAINER, BYTES_PER_I2C_BLOCK, (uint8_t *)Data);
            if (QAPI_OK == Status) {
                I2CM_PRINTF("successfully\r\n");
            } else {
                I2CM_PRINTF("failed: %d\r\n", (int)Status);
            }
            // NT3H2111_DelayMs(5);
        }
    } else {
        I2CM_PRINTF("Failed to read CC: %d!!\r\n", (int)Status);
    }

    return Status;
}

qapi_Status_t NFCTag_Init(qapi_I2CM_Instance_t Instance)
{
    qapi_Status_t Ret = QAPI_OK;

    Ret = NFC_ReadCapabilityContainer(Instance);

    return Ret;
}

qapi_Status_t NfcTag_WritePhoneNumber(qapi_I2CM_Instance_t Instance, uint8_t *Number, uint8_t NumberLen)
{
    qapi_Status_t Status = QAPI_ERROR;
    uint8_t MsgLenCnt = 0;
    uint8_t NDEF_Buffer[NDEF_MAX_SIZE] = {0};
    int Count = 0;
    int i;

    if (NumberLen != NFC_TAG_PHONE_NUM_LEN) {
        return Status;
    }

    /* NDEF message Tag */
    NDEF_Buffer[MsgLenCnt++] = NDEF_MSG_TLV;
    /* Length field, fill out later */
    NDEF_Buffer[MsgLenCnt++] = 0;
    /* SR = 1; Short Record
     * TNF = 1; NFC well-known type
     * ME = 1; message end
     * MB = 1; message begin
     */
    NDEF_Buffer[MsgLenCnt++] = 0xD1;
    /* length of the record type */
    NDEF_Buffer[MsgLenCnt++] = 0x01;
    /* length of the payload */
    NDEF_Buffer[MsgLenCnt++] = 1 + NumberLen;
    /* URI record type, ("U") */
    NDEF_Buffer[MsgLenCnt++] = 0x55;
    /* URI identifier, ("https://www.") */
    NDEF_Buffer[MsgLenCnt++] = 0x05;
    /* URL content */
    memscpy(NDEF_Buffer + MsgLenCnt, NumberLen, Number, NumberLen);
    MsgLenCnt += NumberLen;
    /* terminator */
    NDEF_Buffer[MsgLenCnt++] = NDEF_TERMINATOR_TLV;
    /* update the NDEF length field, deduct Tag, Length, and Terminator fields */
    NDEF_Buffer[1] = MsgLenCnt - 3;

    /* need to write a block (16 bytes) at once */
    Count = (MsgLenCnt >> 4) + 1;

    for (i = 0; i < Count; i++) {
        Status = I2CM_TransferHandler(Instance, NT3H2111_I2C_ADDRESS, I2C_FAST_MODE_FREQ_KHZ, I2C_MASTER_TRANSFER_WRITE,
                                      REG_START_BLOCK + i, BYTES_PER_I2C_BLOCK, &NDEF_Buffer[BYTES_PER_I2C_BLOCK * i]);
        if (QAPI_OK == Status) {
            // wait for a while, can poll session register later
            // qapi_TMR_Delay_Us(5);
            hres_timer_us_delay(5);
        } else {
            I2CM_PRINTF("Failed to write phone number to (%d): %d!!\r\n", REG_START_BLOCK + i, (int)Status);
            break;
        }
    }

    if (QAPI_OK == Status) {
        I2CM_PRINTF("Successful to write phone number: ");
        for (i = 0; i < NumberLen; i++) {
            I2CM_PRINTF("%c", *(Number + i));
        }
        I2CM_PRINTF("\r\n");
    }
    return Status;
}

static qapi_Status_t nfc_Demo(qapi_I2CM_Instance_t Instance)
{
    uint8_t default_num[] = {0x2b, 0x31, 0x33, 0x36, 0x30, 0x33, 0x38, 0x38, 0x36, 0x37, 0x32, 0x38};

    I2CM_Ctxt[Instance].SlaveType = I2C_SlAVE_TYPE_NFC;
    NFCTag_Init(Instance);
    NfcTag_WritePhoneNumber(Instance, default_num, sizeof(default_num));

    return QAPI_OK;
}

qapi_Status_t cmd_I2CM_Open(uint32_t __attribute__((__unused__)) Parameter_Count,
                            QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List)
{
    qapi_I2CM_Config_t *Config;
    qapi_I2CM_Instance_t Instance;
    qapi_Status_t Status;

    if ((Parameter_Count != 3) || (!Parameter_List) || (Parameter_List[0].Integer_Value > QAPI_I2C_INSTANCE_SE0_E) ||
        (Parameter_List[1].Integer_Value > 1) || (Parameter_List[2].Integer_Value > 1)) {
        I2CM_PRINTF("Invalid Parameter.\r\n");
        cmd_I2CM_OpenHelp();
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    Instance = (qapi_I2CM_Instance_t)Parameter_List[0].Integer_Value;
    if (I2CM_Ctxt[Instance].State != I2C_MASTER_INSTANCE_CLOSED) {
        I2CM_PRINTF("I2C Master Instance %d has been opened.\r\n", (int)Instance);
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    Config = &I2CM_Ctxt[Instance].Config;
    // Config->Blocking = Parameter_List[1].Integer_Value;
    // Config->Dma = Parameter_List[2].Integer_Value;
    Config->Blocking = 1;
    Config->Dma = 0;

    Status = qapi_I2CM_Open(Instance, Config);
    if (Status != QAPI_OK) {
        I2CM_PRINTF("I2C Master instance %d open failed with status %d.\r\n", (int)Instance, (int)Status);
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
    }

    I2CM_Ctxt[Instance].State = I2C_MASTER_INSTANCE_READY;

    I2CM_PRINTF("I2C Master instance %d open success.\r\n", (int)Instance);

    return QAPI_OK;
}

qapi_Status_t cmd_I2CM_Transfer(uint32_t __attribute__((__unused__)) Parameter_Count,
                                QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List)
{
    qapi_I2CM_Instance_t Instance;
    I2C_Slave_Type_t SlaveType;
    qapi_Status_t Status;
    uint32_t SlaveAddress;
    uint32_t Frequency;
    uint32_t Address;
    uint32_t DataLen;
    I2CM_OpCode_t OpCode;
    char *Data;

    if ((Parameter_List) && (!strncmp(Parameter_List[0].String_Value, "nfc", 3))) {
        if (Parameter_Count < 2) {
            I2CM_PRINTF("Usage:\ttransfer nfc <Instance>\r\n");
            I2CM_PRINTF("Options:\r\n");
            I2CM_PRINTF("\tInstance      : 0 I2C Master instance\r\n");
            I2CM_PRINTF("Example: transfer nfc 0\r\n");
            goto err2;
        }

        Instance = (qapi_I2CM_Instance_t)Parameter_List[1].Integer_Value;
        nfc_Demo(Instance);
        return QAPI_OK;
    }

    if ((Parameter_Count < 7) || (!Parameter_List)) {
        goto err1;
    }

    if (!strncmp(Parameter_List[0].String_Value, "eeprom", 6)) {
        SlaveType = I2C_SlAVE_TYPE_EEPROM;
    } else if (!strncmp(Parameter_List[0].String_Value, "rawslave", 8)) {
        SlaveType = I2C_SlAVE_TYPE_RAWSLAVE;
    } else {
        I2CM_PRINTF("Invalid slave type.\r\n");
        goto err2;
    }

    if (!strncmp(Parameter_List[1].String_Value, "wr", 2)) {
        if (SlaveType != I2C_SlAVE_TYPE_EEPROM) {
            I2CM_PRINTF("Invalid slave operation type, 'wr' only support eeprom\r\n");
            goto err2;
        }
        if (Parameter_Count != 7) {
            I2CM_PRINTF("Invalid parameter count %d for eeprom wr\r\n", (int)Parameter_Count);
            goto err1;
        }
        OpCode = I2C_MASTER_TRANSFER_WRITE_READ;
    } else if (!strncmp(Parameter_List[1].String_Value, "w", 1)) {
        if (Parameter_Count != 8) {
            I2CM_PRINTF("Invalid parameter count %d for eeprom w\r\n", (int)Parameter_Count);
            goto err1;
        }
        OpCode = I2C_MASTER_TRANSFER_WRITE;
    } else if (!strncmp(Parameter_List[1].String_Value, "r", 1)) {
        if (Parameter_Count != 7) {
            I2CM_PRINTF("Invalid parameter count %d for eeprom r\r\n", (int)Parameter_Count);
            goto err1;
        }
        OpCode = I2C_MASTER_TRANSFER_READ;
    } else if (!strncmp(Parameter_List[1].String_Value, "wr", 2)) {
        if (SlaveType != I2C_SlAVE_TYPE_EEPROM) {
            I2CM_PRINTF("Invalid slave operation type, 'wr' only support eeprom\r\n");
            goto err2;
        }
        if (Parameter_Count != 7) {
            I2CM_PRINTF("Invalid parameter count %d for eeprom wr\r\n", (int)Parameter_Count);
            goto err1;
        }
        OpCode = I2C_MASTER_TRANSFER_WRITE_READ;
    } else {
        I2CM_PRINTF("Invalid operation, only support 'w' ,'r' and 'wr'.\r\n");
        goto err2;
    }

    Instance = (qapi_I2CM_Instance_t)Parameter_List[2].Integer_Value;
    if (I2CM_Ctxt[Instance].TransferNum >= I2C_MAX_TRANSFER_NUMBER) {
        I2CM_Ctxt[Instance].State = I2C_MASTER_INSTANCE_TRANSFER_QUEUE_FULL;
        I2CM_PRINTF("Transfer queue is full, please try it later.\r\n");
        goto err2;
    }

    DataLen = Parameter_List[6].Integer_Value;
    if (((DataLen > I2C_MAX_TRANSFER_DATA_LENGTH) && (I2C_SlAVE_TYPE_EEPROM == SlaveType)) ||
        ((DataLen > I2C_SLAVE_MAX_TRANSFER_DATA_LENGTH) && (I2C_SlAVE_TYPE_RAWSLAVE == SlaveType))) {
        I2CM_PRINTF("The Max Transfer Data Lenght is %d.\r\n", SlaveType == I2C_SlAVE_TYPE_EEPROM
                                                                   ? I2C_MAX_TRANSFER_DATA_LENGTH
                                                                   : I2C_SLAVE_MAX_TRANSFER_DATA_LENGTH);
        goto err2;
    }

    if (DataLen == 0) {
        I2CM_PRINTF("The Transfer Data Lenght %d is invalid.\r\n", (int)DataLen);
        goto err2;
    }

    SlaveAddress = Parameter_List[3].Integer_Value;
    Frequency = Parameter_List[4].Integer_Value;
    Address = Parameter_List[5].Integer_Value;
    if (OpCode != I2C_MASTER_TRANSFER_READ) {
        Data = Parameter_List[7].String_Value;
    } else {
        Data = NULL;
    }

    if ((OpCode == I2C_MASTER_TRANSFER_WRITE) && (strlen(Data) != DataLen)) {
        I2CM_PRINTF("The lenght of Data is not same as DataLen.\r\n");
        goto err2;
    }

    if (I2CM_CheckTransferParameters(Instance, Frequency, SlaveType, Address, DataLen) != QAPI_OK) {
        goto err2;
    }
    I2CM_Ctxt[Instance].SlaveType = SlaveType;
    Status = I2CM_TransferHandler(Instance, SlaveAddress, Frequency, OpCode, Address, DataLen, (uint8_t *)Data);

    if (Status != QAPI_OK) {
        I2CM_PRINTF("Transfer I2C Master Instance %d failed with Status %d.\r\n", (int)Instance, (int)Status);
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
    } else {
        return QAPI_OK;
    }
err1:
    I2CM_PRINTF("Invalid parameter.\r\n");
    cmd_I2CM_TransferHelp();
err2:
    return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
}

qapi_Status_t cmd_I2CM_Cancel(uint32_t __attribute__((__unused__)) Parameter_Count,
                              QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List)
{
    qapi_I2CM_Instance_t Instance;
    qapi_Status_t Status;

    if ((Parameter_Count != 1) || (!Parameter_List) || (Parameter_List[0].Integer_Value > QAPI_I2C_INSTANCE_SE0_E)) {
        I2CM_PRINTF("Invalid Parameter.\r\n");
        cmd_I2CM_CancelHelp();
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    Instance = (qapi_I2CM_Instance_t)Parameter_List[0].Integer_Value;
    if (I2CM_Ctxt[Instance].State == I2C_MASTER_INSTANCE_CLOSED) {
        I2CM_PRINTF("Cannot cancel I2C Master Instance %d since it has not been opened.\r\n", (int)Instance);
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    Status = qapi_I2CM_Cancel_Transfer(Instance);

    if (Status != QAPI_OK) {
        I2CM_PRINTF("Cancel I2C Master Instance %d failed with Status %d.\r\n", (int)Instance, (int)Status);
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
    }
    I2CM_Ctxt[Instance].State = I2C_MASTER_INSTANCE_READY;

    I2CM_PRINTF("I2C Master Instance %d Cancel successfully.\r\n", (int)Instance);

    return QAPI_OK;
}

qapi_Status_t cmd_I2CM_Close(uint32_t __attribute__((__unused__)) Parameter_Count,
                             QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List)
{
    qapi_I2CM_Instance_t Instance;
    qapi_Status_t Status;

    if ((Parameter_Count != 1) || (!Parameter_List) || (Parameter_List[0].Integer_Value > QAPI_I2C_INSTANCE_SE0_E)) {
        I2CM_PRINTF("Invalid Parameter.\r\n");
        cmd_I2CM_CloseHelp();
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    Instance = (qapi_I2CM_Instance_t)Parameter_List[0].Integer_Value;
    if (I2CM_Ctxt[Instance].State == I2C_MASTER_INSTANCE_CLOSED) {
        I2CM_PRINTF("I2C Master Instance %d has been Closed.\r\n", (int)Instance);
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    Status = qapi_I2CM_Close(Instance);

    if (Status != QAPI_OK) {
        I2CM_PRINTF("Close I2C Master Instance %d failed with Status %d.\r\n", (int)Instance, (int)Status);
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
    }

    memset(&I2CM_Ctxt[Instance], 0, sizeof(I2C_Master_Context_t));

    I2CM_PRINTF("I2C Master Instance %d Close successfully.\r\n", (int)Instance);

    return QAPI_OK;
}

#if (CONFIG_I2CM_SHELL)
const QAPI_Console_Command_t i2cm_shell_cmds[] = {
    // cmd_function			cmd_string       usage_string									description
    {cmd_I2CM_Open, "Open", "<Instance> <Blocking> <Dma>", "Open I2CM instance"},
    {cmd_I2CM_Transfer, "Transfer",
     "<SlaveType> <Operation> <Instance> <SlaveAddress> <Frequency> <Address> <DataLen> <Data>", "Transfer I2CM data"},
    {cmd_I2CM_Cancel, "Cancel", "<Instance>", "Cancel I2CM tranfer"},
    {cmd_I2CM_Close, "Close", "<Instance>", "Close I2CM instance"},
};

const QAPI_Console_Command_Group_t i2cm_shell_cmd_group = {
    "I2CM", sizeof(i2cm_shell_cmds) / sizeof(QAPI_Console_Command_t), i2cm_shell_cmds};

QAPI_Console_Group_Handle_t i2cm_shell_cmd_group_handle;

void i2cm_shell_init(void)
{
    i2cm_shell_cmd_group_handle = QAPI_Console_Register_Command_Group(NULL, &i2cm_shell_cmd_group);
}
#endif
#endif  // I2C_DEMO
