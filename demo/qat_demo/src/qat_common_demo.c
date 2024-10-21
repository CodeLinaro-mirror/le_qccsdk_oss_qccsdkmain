/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/
#include <stdio.h>
#include <stdarg.h>
#include "string.h"
#include "qapi_version.h"
#include "qapi_rtc.h"
#include "qapi_heap_status.h"
#include "qat.h"
#include "qat_api.h"
#include "qurt_internal.h"
#include "nt_osal.h"
#include "qurt_mutex.h"
#include "wifi_fw_version.h"
#include "wifi_fw_pmu_ts_cfg.h"

/*-------------------------------------------------------------------------
 * Function Declarations
 *-----------------------------------------------------------------------*/
static QAT_Command_Status_t Extend_Command_Version(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_Info(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_Reset(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_Write_Memory(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_Read_Memory(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_Cmd(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);

static uint32_t mem_test = 123;

/* The following is the complete command list for the QAT common command demo. */
/** List of global commands that are supported when in a group. */
static QAT_Command_t QAT_Common_Command_List[] =
{
	{"+CMD",   Extend_Command_Cmd,      QAT_OP_QUERY},
	{"+GMR",   Extend_Command_Version,   QAT_OP_EXEC},
    {"+INFO",  Extend_Command_Info,      QAT_OP_EXEC},
    {"+RST",   Extend_Command_Reset,      QAT_OP_EXEC},
    {"+WRTMEM",   Extend_Command_Write_Memory,      QAT_OP_EXEC_W_PARAM | QAT_OP_EXEC},
    {"+RDMEM",   Extend_Command_Read_Memory,      QAT_OP_EXEC_W_PARAM | QAT_OP_EXEC},
};
/*-------------------------------------------------------------------------
 * External parameters
 *-----------------------------------------------------------------------*/
 extern HTC_Context_t HTC_Context;

/*-------------------------------------------------------------------------
 * Parameters define
 *-----------------------------------------------------------------------*/

#define COMMON_COMMAND_LIST_SIZE                      (sizeof(QAT_Common_Command_List) / sizeof(QAT_Command_t))

#define VERSION_STR_BUFFER_LENGTH 					  256
#define INFO_STR_BUFFER_LENGTH					      256
#define WRTMEM_STR_BUFFER_LENGTH					  128
#define CMD_STR_BUFFER_LENGTH					      1024

/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/

/**
   @brief Processes the Extend command from the QAT.

   This command will change the current group to its parent. No parameters are
   expected for this command.

   @param[in] Op_Type          The input command type.
   @param[in] Parameter_Count  Number of parameters that were entered into the
                               command line.
   @param[in] Parameter_List   List of parameters entered into the command line.
*/
static QAT_Command_Status_t Extend_Command_Version(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{
   char *buffer;
   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;

   switch (Op_Type)
   {      
      case QAT_OP_EXEC: 	     /* AT+GMR */
      {
		 unsigned int otp_version = *(unsigned int *)0x1a002c;
         unsigned int PBL_version = *(unsigned int *)0x200168;
         unsigned int kdf_lock = *(unsigned int *)0x1a0090;
 		 unsigned int CUID_0 = *(unsigned int *)0x1a0004;
 		 unsigned short CUID_1 = *(unsigned short *)0x1a0008;

		 buffer = malloc(VERSION_STR_BUFFER_LENGTH);
         if(!buffer)
         {
             QAT_Response_Str(QAT_RC_ERROR, NULL);
             return rc;
         }
         memset((void*)buffer, 0, VERSION_STR_BUFFER_LENGTH);
         snprintf(buffer, VERSION_STR_BUFFER_LENGTH, "QAPI Ver: %d.%d.%d\r\n crm num: %d.\r\n WiFi version: %d.%d.%d\t%s\r\n build board name: %s\r\n OTP: OTP-version %d.%d, PBL-version %d.%d.%d, KDF-Lock 0x%x, CUID 0x%x %x\r\n build date and time: %s - %s\r\n", 
                  QAPI_VERSION_MAJOR, QAPI_VERSION_MINOR, QAPI_VERSION_NIT, CRM_BUILD_NUM,
                  WIFI_FW_VER_MAJOR,WIFI_FW_VER_MINOR,WIFI_FW_VER_COUNT,WIFI_FW_VARIANT_NAME,
                  CONFIG_QCCSDK_BOARD_NAME, 
                  ((otp_version>>24)&0xff), (((otp_version>>16)&0xff)),
	   	          ((PBL_version>>24)&0xff), (((PBL_version>>16)&0xff)), (((PBL_version>>0)&0xffff)),
	   	          ((kdf_lock>>8)&0xff), CUID_1, CUID_0,
	   	          __DATE__, __TIME__);
		 //printf("+VER: ROM 0x%04x, Build 0x%04x, %s, %s", QAPI_CHIP_VERSION, QAPI_BUILD_VERSION, Rel_Date, Rel_Time);
         rc = QAT_Response_Str(QAT_RC_OK, buffer);
         memset((void*)buffer, 0, VERSION_STR_BUFFER_LENGTH);
         free(buffer);
         buffer = NULL;
         break;
      }
      
      default:
         ;
   }
   
   return rc;
}

/**
   @brief Processes the Extend command from the QAT.

   This command will change the current group to its parent. No parameters are
   expected for this command.

   @param[in] Op_Type          The input command type.
   @param[in] Parameter_Count  Number of parameters that were entered into the
                               command line.
   @param[in] Parameter_List   List of parameters entered into the command line.
*/
static QAT_Command_Status_t Extend_Command_Info(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{
	heap_status hs;
	qapi_Time_t tm;
	char *buffer;
	QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
	
	switch (Op_Type)
   {      
      case QAT_OP_EXEC: 	     /* AT+INFO */
      {
		buffer = malloc(VERSION_STR_BUFFER_LENGTH);

		if(!buffer)
		{	
		    QAT_Response_Str(QAT_RC_ERROR, NULL);
		    return rc;
		}

		if(qapi_Heap_Status(&hs) != QAPI_OK)
	    {   
	        QAT_Response_Str(QAT_RC_ERROR, NULL);
		    return rc;
	    }
		
		snprintf(buffer, VERSION_STR_BUFFER_LENGTH, "Show system information\r\n\
		Temperature=%dC\r\nVbat=%dmV\r\n\
		get heap status\r\n\
		           total       used       free       min_free\r\n\
		Heap:   %8d   %8d   %8d       %8d\r\n"
		, pmu_ts_get_current_temperature(), tv_monitor_get_vbat_mV()
		, hs.total_Bytes, hs.total_Bytes-hs.free_Bytes, hs.free_Bytes, hs.min_ever_free_bytes); 
		
		rc = QAT_Response_Str(QAT_RC_OK, buffer);
		memset((void*)buffer, 0, VERSION_STR_BUFFER_LENGTH);
        free(buffer);
        buffer = NULL;
		
		break;
	  }
	  
	  default:
         ;
	}

    return rc;
}

/**
   @brief Processes the Extend command from the QAT.

   This command will change the current group to its parent. No parameters are
   expected for this command.

   @param[in] Op_Type          The input command type.
   @param[in] Parameter_Count  Number of parameters that were entered into the
                               command line.
   @param[in] Parameter_List   List of parameters entered into the command line.
*/
static QAT_Command_Status_t Extend_Command_Reset(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{
   char *buffer;
   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;

   switch (Op_Type)
   {
      case QAT_OP_EXEC: 	     /* AT+RST */
      {
		 rc = QAT_Response_Str(QAT_RC_OK, NULL);
		 /*Wait for sending OK*/
		 sleep(1);
		 nt_system_sw_reset();
         break;
      }
      
      default:
         ;
   }
   
   return rc;
}

/**
   @brief Processes the Extend command from the QAT.

   This command will change the current group to its parent. No parameters are
   expected for this command.

   @param[in] Op_Type          The input command type.
   @param[in] Parameter_Count  Number of parameters that were entered into the
                               command line.
   @param[in] Parameter_List   List of parameters entered into the command line.
*/
static QAT_Command_Status_t Extend_Command_Write_Memory(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{
   char *buffer;
   char *buffer_test;
   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
   uint32_t data, size, addr;
   
  
   switch (Op_Type)
   {
      case QAT_OP_EXEC:		     /* AT+WRTMEM */
      {	
	     buffer = malloc(WRTMEM_STR_BUFFER_LENGTH);
         if(!buffer)
         {
             QAT_Response_Str(QAT_RC_ERROR, NULL);
             return rc;
         }
		 snprintf(buffer, WRTMEM_STR_BUFFER_LENGTH, "+WRTMEM: <addr>,<size:1|2|4>,<value>\r\nmem: 0x%08x for test\r\n",&mem_test);
		 rc = QAT_Response_Str(QAT_RC_OK, buffer);
		 memset((void*)buffer, 0, WRTMEM_STR_BUFFER_LENGTH);
         free(buffer);
         buffer = NULL;
         break;
      }
      
      case QAT_OP_EXEC_W_PARAM: 	     /* AT+WRTMEM */
      {
		 if( Parameter_Count != 3 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid || !Parameter_List[1].Integer_Is_Valid|| !Parameter_List[2].Integer_Is_Valid) {
        	rc = QAT_Response_Str(QAT_RC_ERROR, "Wrong Input, AT+WRTMEM? for hint\r\n");
            return rc;
    	 }
		 
		 addr = Parameter_List[0].Integer_Value;
    	 size = Parameter_List[1].Integer_Value;
    	 data = Parameter_List[2].Integer_Value;
		 
		 buffer = malloc(WRTMEM_STR_BUFFER_LENGTH);
         if(!buffer)
         {
             QAT_Response_Str(QAT_RC_ERROR, NULL);
             return rc;
         }
		 
		 if (size == 1)
 	        *(uint8_t *)addr = data;
 	     else if (size == 2)
 	        *(uint16_t *)addr = data;
 	     else if (size == 4)
 	        *(uint32_t *)addr = data;
 	     else{
			rc = QAT_Response_Str(QAT_RC_ERROR, "Wrong Input, AT+WRTMEM? for hint\r\n");
            return rc;
		 }

    	 snprintf(buffer, WRTMEM_STR_BUFFER_LENGTH, "Writting, Address = 0x%08x , Width = %d  Data = 0x%08x(%d)\r\n",addr,size,data,data);
         rc = QAT_Response_Str(QAT_RC_OK, buffer);
         memset((void*)buffer, 0, WRTMEM_STR_BUFFER_LENGTH);
         free(buffer);
         buffer = NULL;
         break;
      }
      
      default:
         ;
   }
   
   return rc;
}

/**
   @brief Processes the Extend command from the QAT.

   This command will change the current group to its parent. No parameters are
   expected for this command.

   @param[in] Op_Type          The input command type.
   @param[in] Parameter_Count  Number of parameters that were entered into the
                               command line.
   @param[in] Parameter_List   List of parameters entered into the command line.
*/
static QAT_Command_Status_t Extend_Command_Read_Memory(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{
   char *buffer;
   char *buffer_test;
   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
   uint32_t data, size, addr;
   
  
   switch (Op_Type)
   {
      case QAT_OP_EXEC:		     /* AT+RDMEM */
      {	
		 buffer = malloc(WRTMEM_STR_BUFFER_LENGTH);
         if(!buffer)
         {
             QAT_Response_Str(QAT_RC_ERROR, NULL);
             return rc;
         }
		 snprintf(buffer, WRTMEM_STR_BUFFER_LENGTH, "+RDMEM: <addr>,<size:1|2|4>\r\nmem: 0x%08x for test\r\n",&mem_test);
		 rc = QAT_Response_Str(QAT_RC_OK, buffer);
		 memset((void*)buffer, 0, WRTMEM_STR_BUFFER_LENGTH);
         free(buffer);
         buffer = NULL;
         break;
      }
      
      case QAT_OP_EXEC_W_PARAM: 	     /* AT+RDMEM */
      {
		 if( Parameter_Count != 2 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid || !Parameter_List[1].Integer_Is_Valid) {
        	rc = QAT_Response_Str(QAT_RC_ERROR, "Wrong Input, AT+RDMEM? for hint\r\n");
            return rc;
    	 }
		 
		 addr = Parameter_List[0].Integer_Value;
 	     size = Parameter_List[1].Integer_Value;
 	     if (size == 1)
 	        data = *(uint8_t *)addr;
 	     else if (size == 2)
 	        data = *(uint16_t *)addr;
 	     else if (size == 4)
 	        data = *(uint32_t *)addr;
 	     else{
			rc = QAT_Response_Str(QAT_RC_ERROR, "Wrong Input, AT+RDMEM? for hint\r\n");
            return rc;
		 }
		 
		 buffer = malloc(WRTMEM_STR_BUFFER_LENGTH);
         if(!buffer)
         {
             QAT_Response_Str(QAT_RC_ERROR, NULL);
             return rc;
         }

    	 snprintf(buffer, WRTMEM_STR_BUFFER_LENGTH, "Read, Address = 0x%08x , Width = %d  Data = 0x%08x(%d)\r\n",addr,size,data,data);
         rc = QAT_Response_Str(QAT_RC_OK, buffer);
         memset((void*)buffer, 0, WRTMEM_STR_BUFFER_LENGTH);
         free(buffer);
         buffer = NULL;
         break;
      }
      
      default:
         ;
   }
   
   return rc;
}

/**
   @brief Processes the Extend command from the QAT.

   This command will change the current group to its parent. No parameters are
   expected for this command.

   @param[in] Op_Type          The input command type.
   @param[in] Parameter_Count  Number of parameters that were entered into the
                               command line.
   @param[in] Parameter_List   List of parameters entered into the command line.
*/
static QAT_Command_Status_t Extend_Command_Cmd(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{
   char *buffer;
   int offset = 0;
   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
   Group_List_Entry_t *Current_Entry = HTC_Context.Next;   
   int Index;
   const QAT_Command_Group_t *Command_Group; /**< Command group information. */
   uint8_t command_len = 0;
   uint8_t command_len_total = 0;

   switch (Op_Type)
   {      
      case QAT_OP_QUERY: 	     /* AT+CMD */
      {
		 buffer = malloc(CMD_STR_BUFFER_LENGTH);
		 
         if(!buffer)
         {
             QAT_Response_Str(QAT_RC_ERROR, NULL);
             return rc;
         }
		 
	 	 while(Current_Entry)
	 	 {
			
		 	/* Add the new entry to its parents subgroup list. */
	 	    Command_Group = &Current_Entry->Command_Group;
	 	    for(Index = 0; Index < Command_Group->Command_Count; Index++)
	 	    {
	 	       
			   command_len = strlen(Command_Group->Command_List[Index].Command_String)+1;
			   
			   offset += snprintf(buffer+offset, CMD_STR_BUFFER_LENGTH-offset, "+CMD:%d,\"%s\",%d,%d,%d\r\n", 
			   	Index, Command_Group->Command_List[Index].Command_String,
			   	(Command_Group->Command_List[Index].Command_Flags & QAT_OP_QUERY),
			   	(Command_Group->Command_List[Index].Command_Flags & QAT_OP_EXEC)>>2,
			   	(Command_Group->Command_List[Index].Command_Flags & QAT_OP_EXEC_W_PARAM)>>3
			   	);
	 	    }        
	 	    Current_Entry = Current_Entry->Next;
	 	 }

		 rc = QAT_Response_Str(QAT_RC_OK, buffer);
 		 memset((void*)buffer, 0, CMD_STR_BUFFER_LENGTH);
 		 free(buffer);
		 buffer = NULL;

		 break;
      }
      
      default:
         ;
   }

   return rc;
}

void Initialize_QAT_Common_Demo (void)
{
	qbool_t RetVal;
	RetVal = QAT_Register_Command_Group(QAT_Common_Command_List, COMMON_COMMAND_LIST_SIZE);
	if(RetVal == false)
   {
      printf("Failed to register common command group.\n");
   }
}

