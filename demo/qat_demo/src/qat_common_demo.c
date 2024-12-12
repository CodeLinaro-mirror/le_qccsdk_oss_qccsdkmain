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
#include "nt_socpm_sleep.h"
#include "qurt_mutex.h"
#include "wifi_fw_version.h"
#include "wifi_fw_pmu_ts_cfg.h"
#include "dns.h"
#include "ip_addr.h"

#ifdef CONFIG_SNTP_CLIENT_DEMO
#include "lwip/apps/sntp.h"
#endif

/*-------------------------------------------------------------------------
 * Function Declarations
 *-----------------------------------------------------------------------*/
static QAT_Command_Status_t Extend_Command_Version(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_Info(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_Reset(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_Write_Memory(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_Read_Memory(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_TIME(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);

static QAT_Command_Status_t Extend_Command_Deep_Sleep(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_DNSC(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_SNTPC(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);

static QAT_Command_Status_t Extend_Command_Cmd(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
QAT_Command_Status_t qat_power_state_event_handler(uint8_t evt);

static uint32_t mem_test = 123;

/* The following is the complete command list for the QAT common command demo. */
/** List of global commands that are supported when in a group. */
static QAT_Command_t QAT_Common_Command_List[] =
{
	{"+CMD",   Extend_Command_Cmd,      QAT_OP_EXEC | QAT_OP_QUERY},
	{"+GMR",   Extend_Command_Version,   QAT_OP_EXEC | QAT_OP_QUERY},
    {"+INFO",  Extend_Command_Info,      QAT_OP_EXEC | QAT_OP_QUERY},
    {"+RST",   Extend_Command_Reset,      QAT_OP_EXEC},
    {"+WRTMEM",   Extend_Command_Write_Memory,      QAT_OP_EXEC_W_PARAM | QAT_OP_EXEC},
    {"+RDMEM",    Extend_Command_Read_Memory,      QAT_OP_EXEC_W_PARAM | QAT_OP_EXEC},
    {"+TIME",     Extend_Command_TIME,      QAT_OP_EXEC | QAT_OP_EXEC_W_PARAM | QAT_OP_QUERY},
    {"+SLEEP",    Extend_Command_Deep_Sleep,      QAT_OP_EXEC_W_PARAM | QAT_OP_EXEC},
    {"+DNSC",     Extend_Command_DNSC,      QAT_OP_EXEC_W_PARAM | QAT_OP_EXEC},
    {"+SNTPC",    Extend_Command_SNTPC,     QAT_OP_QUERY | QAT_OP_EXEC_W_PARAM | QAT_OP_EXEC},
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
#define NORMAL_RESPONSE_BUFFER_LENGTH 				  1024

#define DEEP_SLP_WKUP_EXT           				  2

 /* @brief Register Power Event Mask as Enum */
typedef enum qat_pwr_evt
{
    PWR_EVT_WMAC_PREPARE_TO_SLEEP  = 1 << 1U,
    PWR_EVT_WMAC_FAIL_TO_SLEEP = 1 << 2U,
} qat_pwr_evt_t;

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
	  case QAT_OP_EXEC:
	  {
		 rc = QAT_Response_Str(QAT_RC_OK, "AT+GMR: get usage of command\r\n"\
										 "AT+GMR?: get SDK version\r\n");
		 break;
	  }
	  case QAT_OP_QUERY: 	     /* AT+GMR */
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
         snprintf(buffer, VERSION_STR_BUFFER_LENGTH, "+GMR:%d.%d.%d,%d,%d.%d.%d,%s,%s,%d.%d,%d.%d.%d,0x%x,0x%x,%x,%s-%s\r\n", 
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
	  case QAT_OP_EXEC:
	  {
		 rc = QAT_Response_Str(QAT_RC_OK, "AT+INFO: get usage of command\r\n"\
										 "AT+INFO?: get board information, including temparature, battery voltage, and memory usage\r\n");
		 break;
	  }
	  case QAT_OP_QUERY: 	     /* AT+INFO */
      {
		buffer = malloc(VERSION_STR_BUFFER_LENGTH);

		if(!buffer)
		{	
		    QAT_Response_Str(QAT_RC_ERROR, NULL);
		    return rc;
		}

		memset(buffer, 0, VERSION_STR_BUFFER_LENGTH);

		if(qapi_Heap_Status(&hs) != QAPI_OK)
	    {   
	        QAT_Response_Str(QAT_RC_ERROR, NULL);
		    return rc;
	    }
		
		snprintf(buffer, VERSION_STR_BUFFER_LENGTH, "+INFO:%d,%d,%d,%d,%d,%d"
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

		 memset(buffer, 0, WRTMEM_STR_BUFFER_LENGTH);
		 
		 snprintf(buffer, WRTMEM_STR_BUFFER_LENGTH, "+WRTMEM: <addr>,<size:1|2|4>,<value>\r\nmem: 0x%08x for test",&mem_test);
		 rc = QAT_Response_Str(QAT_RC_OK, buffer);
		 memset((void*)buffer, 0, WRTMEM_STR_BUFFER_LENGTH);
         free(buffer);
         buffer = NULL;
         break;
      }
      
      case QAT_OP_EXEC_W_PARAM: 	     /* AT+WRTMEM */
      {
		 if( Parameter_Count != 3 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid || !Parameter_List[1].Integer_Is_Valid|| !Parameter_List[2].Integer_Is_Valid) {
        	rc = QAT_Response_Str(QAT_RC_ERROR, "+WRTMEM:Wrong Input, AT+WRTMEM? for hint");
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
		 {
 	        *(uint8_t *)addr = data;
		 }
 	     else if (size == 2)
 	     {
 	        *(uint16_t *)addr = data;
 	     }
 	     else if (size == 4)
 	     {
 	        *(uint32_t *)addr = data;
 	     }
 	     else{
			rc = QAT_Response_Str(QAT_RC_ERROR, "+WRTMEM:Wrong Input, AT+WRTMEM? for hint");
            return rc;
		 }

		 snprintf(buffer, WRTMEM_STR_BUFFER_LENGTH, "+WRTMEM:0x%08x,%d,0x%0*x,%d",addr,size,size*2,data,data);
		 
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

		 memset(buffer, 0, WRTMEM_STR_BUFFER_LENGTH);
		 
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
        	rc = QAT_Response_Str(QAT_RC_ERROR, "+RDMEM:Wrong Input, AT+RDMEM? for hint\r\n");
            return rc;
    	 }
		 
		 addr = Parameter_List[0].Integer_Value;
 	     size = Parameter_List[1].Integer_Value;

		 buffer = malloc(WRTMEM_STR_BUFFER_LENGTH);
         if(!buffer)
         {
             QAT_Response_Str(QAT_RC_ERROR, NULL);
             return rc;
         }
		 
		 memset(buffer, 0, WRTMEM_STR_BUFFER_LENGTH);
		 
 	     if (size == 1)
		 {
 	        data = *(uint8_t *)addr;
 	     }
 	     else if (size == 2)
 	     {
 	        data = *(uint16_t *)addr;
 	     }
 	     else if (size == 4)
 	     {
 	        data = *(uint32_t *)addr;
 	     }
 	     else{
			rc = QAT_Response_Str(QAT_RC_ERROR, "+RDMEM:Wrong Input, AT+RDMEM? for hint\r\n");
            return rc;
		 }
		 
		 snprintf(buffer, WRTMEM_STR_BUFFER_LENGTH, "+RDMEM:0x%08x,%d,0x%0*x,%d",addr,size,size*2,data,data);
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
static QAT_Command_Status_t Extend_Command_TIME(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{
   char buffer[NORMAL_RESPONSE_BUFFER_LENGTH]={0};
   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
   int offset = 0;

   memset(buffer, 0, NORMAL_RESPONSE_BUFFER_LENGTH);
   
   switch (Op_Type)
   {
	  case QAT_OP_EXEC: 	     /* AT+TIME */
      {
      	rc = QAT_Response_Str(QAT_RC_OK, "AT+TIME: get usage of command\r\n"\
										 "AT+TIME?: get RTC time\r\n" \
										 "AT+TIME=<year[1980-2100]>,<month[1-12]>,<day[1-31]>,<hour[0-23]>,<minute[0-59]>,<second[0-59]>,<day_Of_Weak[0-6]>: set RTC time");
		break;
	  }
	  case QAT_OP_QUERY: 	     /* AT+TIME */
      {
		qapi_Time_t tm;
	    time_zone_t zone;
	    qapi_Status_t status;

	    status = qapi_Core_RTC_Julian_Get(&tm);
	    if ( QAPI_OK != status ) {
	        offset += snprintf(buffer+offset, NORMAL_RESPONSE_BUFFER_LENGTH-offset, "+TIME:Failed to get time. It would because time is not set");
	        return QAT_Response_Str(QAT_RC_ERROR, buffer);
	    }

	    qapi_Core_Time_Zone_Get(&zone);
	    offset += snprintf(buffer+offset, NORMAL_RESPONSE_BUFFER_LENGTH-offset, "+TIME:%d,%d,%d,%d,%d,%d,%d,%c%02d-%02d", \
			tm.year,tm.month,tm.day,tm.hour,tm.minute,tm.second,tm.day_Of_Week,zone.add_sub?'+':'-', zone.hour, zone.min);

	    rc = QAT_Response_Str(QAT_RC_OK, buffer);
        break;
      }
	  
	  case QAT_OP_EXEC_W_PARAM: 	     /* AT+TIME */
      {
      	qapi_Time_t tm;
        qapi_Status_t status;

		if ( Parameter_Count != 7 ) {
			return QAT_Response_Str(QAT_RC_ERROR, "AT+TIME=<year[1980-2100]>,<month[1-12]>,<day[1-31]>,<hour[0-23]>,<minute[0-59]>,<second[0-59]>,<day_Of_Weak[0-6]>: set RTC time");
		}

		// check year
		if ((Parameter_List[0].Integer_Is_Valid) && (Parameter_List[0].Integer_Value >= 1980) && (Parameter_List[0].Integer_Value <= 2100))
		{
			tm.year = Parameter_List[0].Integer_Value;
		}
		else
		{
			return QAT_Response_Str(QAT_RC_ERROR, "+TIME:Invalid year");
		}
		
		// check month
		if ((Parameter_List[1].Integer_Is_Valid) && (Parameter_List[1].Integer_Value >= 1) && (Parameter_List[1].Integer_Value <= 12))
		{
			tm.month = Parameter_List[1].Integer_Value;
		}
		else
		{
			return QAT_Response_Str(QAT_RC_ERROR, "+TIME:Invalid month");
		}
		
		// check day
		if ((Parameter_List[2].Integer_Is_Valid) && (Parameter_List[2].Integer_Value >= 1) && (Parameter_List[2].Integer_Value <= 31))
		{
			tm.day = Parameter_List[2].Integer_Value;
		}
		else
		{
			return QAT_Response_Str(QAT_RC_ERROR, "+TIME:Invalid day");
		}
		
		// check hour
		if ((Parameter_List[3].Integer_Is_Valid) && (Parameter_List[3].Integer_Value >= 0) && (Parameter_List[3].Integer_Value <= 23))
		{
			tm.hour = Parameter_List[3].Integer_Value;
		}
		else
		{
			return QAT_Response_Str(QAT_RC_ERROR, "+TIME:Invalid hour");
		}
		
		// check minute
		if ((Parameter_List[4].Integer_Is_Valid) && (Parameter_List[4].Integer_Value >= 0) && (Parameter_List[4].Integer_Value <= 59))
		{
			tm.minute = Parameter_List[4].Integer_Value;
		}
		else
		{
			return QAT_Response_Str(QAT_RC_ERROR, "+TIME:Invalid minute");
		}
		
		// check second
		if ((Parameter_List[5].Integer_Is_Valid) && (Parameter_List[5].Integer_Value >= 0) && (Parameter_List[5].Integer_Value <= 59))
		{
			tm.second = Parameter_List[5].Integer_Value;
		}
		else
		{
			return QAT_Response_Str(QAT_RC_ERROR, "+TIME:Invalid second");
		}
		
		// check day of the week
		if ((Parameter_List[6].Integer_Is_Valid) && (Parameter_List[6].Integer_Value >= 0) && (Parameter_List[6].Integer_Value <= 6))
		{
			tm.day_Of_Week = Parameter_List[6].Integer_Value;
		}
		else
		{
			return QAT_Response_Str(QAT_RC_ERROR, "+TIME:Invalid day_Of_Week");
		}
		
		status = qapi_Core_RTC_Julian_Set(&tm);
		if (0 != status ) {
			offset += snprintf(buffer+offset, NORMAL_RESPONSE_BUFFER_LENGTH-offset, "+TIME:FAIL,%d", status);
		    return QAT_Response_Str(QAT_RC_ERROR, buffer);
		}
		rc = QAT_Response_Str(QAT_RC_OK, NULL);
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
static QAT_Command_Status_t Extend_Command_Deep_Sleep(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{
   char buffer[NORMAL_RESPONSE_BUFFER_LENGTH]={0};
   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
   
   switch (Op_Type)
   {
	  case QAT_OP_EXEC:		     /* AT+SLEEP */
      {	
		 snprintf(buffer, NORMAL_RESPONSE_BUFFER_LENGTH, "+SLEEP=<1:AON timer wkup|2:Ext wkup>,<sleep duration in us(min 16000)>");
		 rc = QAT_Response_Str(QAT_RC_OK, buffer);
         break;
      }
      
	  case QAT_OP_EXEC_W_PARAM:		     /* AT+SLEEP= */
      {	
		 if (Parameter_Count != 2 || !Parameter_List ||
        	!Parameter_List[0].Integer_Is_Valid || !Parameter_List[1].Integer_Is_Valid) 
        {
        	rc = QAT_Response_Str(QAT_RC_ERROR, "+SLEEP=<1:AON timer wkup|2:Ext wkup>,<sleep duration in us(min 16000)>");
			return rc;
	    }
			
		rc = QAT_Response_Str(QAT_RC_OK, NULL);

	    if (Parameter_List[0].Integer_Value == DEEP_SLP_WKUP_EXT) {
	        printf("Ext wakeup indefinite deepsleep supported");
	        nt_socpm_en_indef_deep_sleep(TRUE);
	    }

		rc = qat_power_state_event_handler(PWR_EVT_WMAC_PREPARE_TO_SLEEP);
		
	    uint64_t slp_time = (uint64_t)Parameter_List[1].Integer_Value;
	    qapi_pm_enable(1);

		sleep(1);
	    if(QAPI_OK != qapi_deepsleep_enter(Parameter_List[0].Integer_Value, slp_time))
			rc = qat_power_state_event_handler(PWR_EVT_WMAC_FAIL_TO_SLEEP);
        break;
      }
      
      default:
         ;
   }
   
   return rc;
}

#if LWIP_DNS
void qat_dnsc_found_callback(const char *name, const ip_addr_t *ipaddr, void *arg)
{
    (void)arg;

	char buffer[NORMAL_RESPONSE_BUFFER_LENGTH]={0};
	
    if(ipaddr)
        snprintf(buffer, NORMAL_RESPONSE_BUFFER_LENGTH, "+EVT:dns_get:%s,%s",name,ipaddr_ntoa(ipaddr));
    else
        snprintf(buffer, NORMAL_RESPONSE_BUFFER_LENGTH, "+EVT:dns_fail:%s\n", name);
	
	QAT_Response_Str(QAT_RC_QUIET, buffer);
}
#endif

/**
   @brief DNSC

   This command will change the current group to its parent. No parameters are
   expected for this command.

   @param[in] Op_Type          The input command type.
   @param[in] Parameter_Count  Number of parameters that were entered into the
                               command line.
   @param[in] Parameter_List   List of parameters entered into the command line.
*/
static QAT_Command_Status_t Extend_Command_DNSC(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{

   char buffer[NORMAL_RESPONSE_BUFFER_LENGTH]={0};
   QAT_Command_Status_t rc = QAT_STATUS_SUCCESS_E;
   int offset = 0;
   
#if LWIP_DNS 
   switch (Op_Type)
   {
	  case QAT_OP_EXEC:		     /* AT+DNSC */
      {	
		 snprintf(buffer, NORMAL_RESPONSE_BUFFER_LENGTH, "\nAT+DNSC:  show the usage of command\n\r" \
		 						"AT+DNSC=dnsshow show the current list of dns servers \n\r" \
                                "AT+DNSC=addsvr,<ip>: add a DNS server \n\r" \
                                "AT+DNSC=delsvr,<ip>: delete a DNS server \n\r" \
                                "AT+DNSC=gethostbyname,<hostname>: resolve a hostname (string) into an IP address \n\r" \
                                "AT+DNSC=gethostbyname2,<hostname>,<iptype>: like dns_gethostbyname, but returned address type can be controlled\n\r" \
                                "                                       iptype: v4, v6 , v4v6, v6v4 \n\r");
		 rc = QAT_Response_Str(QAT_RC_OK, buffer);
         break;
      }
      
	  case QAT_OP_EXEC_W_PARAM:		     /* AT+DNSC= */
      {	

	    uint32_t indx;
	    const ip_addr_t * server_addr;
	    char* cmd, *hostname;
	    ip_addr_t ip_addr;

	    cmd = Parameter_List[0].String_Value;
		if(strncmp(cmd, "dnsshow", 7) == 0)
		{
			//get DNS list
	        for(indx = 0; indx< DNS_MAX_SERVERS; indx++) {
	            server_addr = (ip_addr_t *)dns_getserver(indx);
	            if(!ip_addr_isany_val(*server_addr)) {
	                offset += snprintf(buffer+offset, NORMAL_RESPONSE_BUFFER_LENGTH-offset, "+DNSC:%d,%s\r\n",indx, ipaddr_ntoa(server_addr));
	            }
	        }
	        rc = QAT_Response_Str(QAT_RC_OK, buffer);
		}
	    else if (strncmp(cmd, "addsvr", 6) == 0) 
		{
	        if (Parameter_Count < 2) 
			{
	            rc = QAT_Response_Str(QAT_RC_ERROR, "+DNSC:AT+DNSC=addsvr,<ip>: add a DNS server");
				return rc;
	        }
	    	if (!ipaddr_aton(Parameter_List[1].String_Value, &ip_addr)) 
			{
	    		rc = QAT_Response_Str(QAT_RC_ERROR, "+DNSC:Invalid IP Address. Please try again");
	    		return rc;
	    	}

	        for(indx = 0; indx< DNS_MAX_SERVERS; indx++) {
	            server_addr = (ip_addr_t *)dns_getserver(indx);
	            if(ip_addr_isany_val(*server_addr)) {
	                break;
	            }
	        }
			
	        if(indx != DNS_MAX_SERVERS) {
	            dns_setserver(indx, &ip_addr);
	            rc = QAT_Response_Str(QAT_RC_OK, NULL);
	        }else {
	            rc = QAT_Response_Str(QAT_RC_ERROR, "+DNSC:add DNS server failed, the array is full now.");
	        }
			return rc;
	    }
		else if (strncmp(cmd, "delsvr", 6) == 0) 
		{
	        if (Parameter_Count < 2) {
	            rc = QAT_Response_Str(QAT_RC_ERROR, "+DNSC:AT+DNSC=delsvr,<ip>: delete a DNS server ");
				return rc;
	        }
			
	    	if (!ipaddr_aton(Parameter_List[1].String_Value, &ip_addr)) {
	    		rc = QAT_Response_Str(QAT_RC_ERROR, "+DNSC:Invalid IP Address. Please try again");
	    		return rc;
	    	}

	        for(indx = 0; indx< DNS_MAX_SERVERS; indx++) {
	            server_addr = (ip_addr_t *)dns_getserver(indx);
	            if((!ip_addr_isany_val(*server_addr)) && ip_addr_cmp(server_addr, &ip_addr)) 
				{
	                dns_setserver(indx, NULL);
	                rc = QAT_Response_Str(QAT_RC_OK, NULL);
					return rc;
	            }
	        }

			rc = QAT_Response_Str(QAT_RC_ERROR, "+DNSC:no DNS server to delete");
			return rc;
			
	    }
		else if (strcmp(cmd, "gethostbyname") == 0) 
		{
	        err_t result;
	        if (Parameter_Count < 2) {
	            rc = QAT_Response_Str(QAT_RC_ERROR, "AT+DNSC=gethostbyname,<hostname>: resolve a hostname (string) into an IP address ");
	    		return rc;
	        }
	        hostname = Parameter_List[1].String_Value;
	        result = dns_gethostbyname(hostname, &ip_addr, qat_dnsc_found_callback, NULL);

	        if( result == ERR_OK) {
	            snprintf(buffer, NORMAL_RESPONSE_BUFFER_LENGTH, "+DNSC:%s,%s\n",hostname,ipaddr_ntoa(&ip_addr));
				rc = QAT_Response_Str(QAT_RC_OK, buffer);
	        }
	        else if(result != ERR_INPROGRESS){
	            snprintf(buffer, NORMAL_RESPONSE_BUFFER_LENGTH, "+DNSC:FAIL,%d.\n",result);
				rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
	        }
			else{
				rc = QAT_Response_Str(QAT_RC_OK, NULL);
			}
			
	    }
		else if (strcmp(cmd, "gethostbyname2") == 0) 
		{
	        err_t result;
	        char * type;
	        uint8_t addr_type = 0;
	        if (Parameter_Count < 3) {
	            rc = QAT_Response_Str(QAT_RC_ERROR, "+DNSC:AT+DNSC=gethostbyname2,<hostname>,<iptype>: like dns_gethostbyname, but returned address type can be controlled\n\r" \
                                "                                       iptype: v4, v6 , v4v6, v6v4 \n\n");
	    		return rc;
	        }
	        hostname = Parameter_List[1].String_Value;
	        type = Parameter_List[2].String_Value;

	        if(strcmp(type, "v4") == 0)
	            addr_type = LWIP_DNS_ADDRTYPE_IPV4;
	        else if(strcmp(type, "v6") == 0)
	            addr_type = LWIP_DNS_ADDRTYPE_IPV6;
	        else if(strcmp(type, "v4v6") == 0)
	            addr_type = LWIP_DNS_ADDRTYPE_IPV4_IPV6;
	        else if(strcmp(type, "v6v4") == 0)
	            addr_type = LWIP_DNS_ADDRTYPE_IPV6_IPV4;
	        else {
	            rc = QAT_Response_Str(QAT_RC_ERROR, "+DNSC:invalid type.\n");
	            return rc;
	        }

	        result = dns_gethostbyname_addrtype(hostname, &ip_addr, qat_dnsc_found_callback, NULL,addr_type);

	        if( result == ERR_OK) {
	            snprintf(buffer, NORMAL_RESPONSE_BUFFER_LENGTH, "+DNSC:%s,%s\n",hostname,ipaddr_ntoa(&ip_addr));
				rc = QAT_Response_Str(QAT_RC_OK, buffer);
	        }
	        else if(result != ERR_INPROGRESS){
	            snprintf(buffer, NORMAL_RESPONSE_BUFFER_LENGTH, "+DNSC:FAIL,%d.\n",result);
				rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
	        }
			else{
				rc = QAT_Response_Str(QAT_RC_OK, NULL);
			}
	    }
		else
		{
			rc = QAT_Response_Str(QAT_RC_ERROR, "+DNSC: command not supported");
		}
        break;
      }
      
      default:
         ;
   }
#else

	rc = QAT_Response_Str(QAT_RC_ERROR, NULL);

#endif

   return rc;
}


/**
   @brief SNTPC

   This command will change the current group to its parent. No parameters are
   expected for this command.

   @param[in] Op_Type          The input command type.
   @param[in] Parameter_Count  Number of parameters that were entered into the
                               command line.
   @param[in] Parameter_List   List of parameters entered into the command line.
*/
static QAT_Command_Status_t Extend_Command_SNTPC(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{
   char buffer[NORMAL_RESPONSE_BUFFER_LENGTH]={0};
   QAT_Command_Status_t rc = QAT_STATUS_SUCCESS_E;
   int offset = 0;
   uint8_t id = 0;
   uint8_t started;
   char *ptr = NULL;
   char *cmd = NULL;
   ip_addr_t ip_addr;
   
#if CONFIG_SNTP_CLIENT_DEMO 
   switch (Op_Type)
   {
	  case QAT_OP_EXEC:		     /* AT+SNTPC */
      {	
		snprintf(buffer, NORMAL_RESPONSE_BUFFER_LENGTH, "\n\nAT+SNTPC: show usage of command\n\r" \
						"AT+SNTPC?: show status of SNTP\n\r" \
						"AT+SNTPC=[start|stop]\n\r" \
						"AT+SNTPC=setOpMode,<0(unicast)|1(broadcast)>\n\r" \
						"AT+SNTPC=setServer,<IP addr|name>,[id]\n\r");
		rc = QAT_Response_Str(QAT_RC_OK, buffer);
		break;
      }
      case QAT_OP_QUERY:		     /* AT+SNTPC? */
      {	
		char *name = NULL;
		const ip_addr_t *addr = NULL;
		uint8_t i = 0;

		cmd = Parameter_List[0].String_Value;

		started = sntp_enabled();
		offset += snprintf(buffer+offset, NORMAL_RESPONSE_BUFFER_LENGTH-offset, "+SNTPC:%s\r\n", started ? "started" : "stopped");

		for(i = 0;i < SNTP_MAX_SERVERS;i++)
		{
			name = sntp_getservername(i);
			addr = sntp_getserver(i);
			offset += snprintf(buffer+offset, NORMAL_RESPONSE_BUFFER_LENGTH-offset, "+SNTPC:%d,%s,%s", i, name != NULL ? name : "****", addr != NULL ? ipaddr_ntoa(addr) : "****");
			if(sntp_getkodreceived(i) != 0)
			{
				offset += snprintf(buffer+offset, NORMAL_RESPONSE_BUFFER_LENGTH-offset, ",KOD");
			}
			offset += snprintf(buffer+offset, NORMAL_RESPONSE_BUFFER_LENGTH-offset, "\r\n");
		}
		rc = QAT_Response_Str(QAT_RC_OK, buffer);
			
		break;
      }
	  case QAT_OP_EXEC_W_PARAM:		     /* AT+SNTPC= */
      {	
		cmd = Parameter_List[0].String_Value;

		/* Sntpc  start */
	    if (strncmp(cmd, "start", 5) == 0)
	    {
	        sntp_init();
			rc = QAT_Response_Str(QAT_RC_OK, NULL);
			
	        return rc;
	    }
	    /* Sntpc  stop */
	    else if (strncmp(cmd, "stop", 4) == 0)
	    {
	        sntp_stop();
			rc = QAT_Response_Str(QAT_RC_OK, NULL);
			
	        return rc;
	    }
		/* Sntpc  set operating class */
	    else if (strncmp(cmd, "setOpMode", 9) == 0)
	    {
	    	uint8_t opMode = 0;
			
			started = sntp_enabled();
			
			if(Parameter_Count <= 1)
			{
				rc = QAT_Response_Str(QAT_RC_ERROR, "+SNTPC:AT+SNTPC=setOpMode,<0(unicast)|1(broadcast)>");
				return rc;
			}
			
			if(Parameter_Count >= 2 && Parameter_List[1].Integer_Is_Valid)
			{
				opMode = Parameter_List[1].Integer_Value;
				if(opMode < 2)
				{
					if(!started)
					{
						sntp_setoperatingmode(opMode);
						rc = QAT_Response_Str(QAT_RC_OK, NULL);
					}
					else
					{
						rc = QAT_Response_Str(QAT_RC_ERROR, "+SNTPC:Operating mode must not be set while SNTP client is running");
					}
	        		return rc;
				}
			}
			rc = QAT_Response_Str(QAT_RC_ERROR, "+SNTPC:the operating class for SNTP client should be 0 or 1");
			
			return rc;
	    }
		/* Sntpc  setServer <IP addr | name> [id] */
	    else if (strncmp(cmd, "setServer", 8) == 0)
	    {
			if(Parameter_Count <= 1)
			{
				rc = QAT_Response_Str(QAT_RC_ERROR, "+SNTPC:AT+SNTPC=setServer,<IP addr|name>,[id]");
				return rc;
			}

			if( Parameter_Count >= 3 && Parameter_List[2].Integer_Is_Valid)
	    	{
	    		id = Parameter_List[2].Integer_Value;
				if(id >= SNTP_MAX_SERVERS)
				{
					rc = QAT_Response_Str(QAT_RC_ERROR, "+SNTPC:Domain name or address cannot be more then 64 bytes");
					return rc;
				}
	    	}
			
	    	if( Parameter_Count >= 2){
				
	        	ptr = Parameter_List[1].String_Value;
				
				if(strlen(ptr) > 64)
				{
					rc = QAT_Response_Str(QAT_RC_ERROR, "+SNTPC:id exceed the max number of SNTP servers");
					return rc;
				}
	        	if(ipaddr_aton(ptr, &ip_addr))
				{
					sntp_setserver(id, &ip_addr);
	        	}
				else
				{
					sntp_setservername(id, ptr);
				}
				rc = QAT_Response_Str(QAT_RC_OK, NULL);
				return rc;
	    	}
	    }
	    break;
	  }
      
      default:
         ;
   }
#else

	rc = QAT_Response_Str(QAT_RC_ERROR, NULL);

#endif

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
	  case QAT_OP_EXEC:
	  {
		 rc = QAT_Response_Str(QAT_RC_OK, "AT+CMD: get usage of command\r\n"\
										 "AT+CMD?: get available AT commands\r\n");
		 break;
	  }
	  case QAT_OP_QUERY: 	     /* AT+CMD */
      {
		 buffer = malloc(CMD_STR_BUFFER_LENGTH);
		 
         if(!buffer)
         {
             QAT_Response_Str(QAT_RC_ERROR, NULL);
             return rc;
         }

		 memset(buffer, 0, CMD_STR_BUFFER_LENGTH);
		 
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

QAT_Command_Status_t qat_power_state_event_handler(uint8_t evt)
{
	QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;

    if (evt == PWR_EVT_WMAC_PREPARE_TO_SLEEP) {
        rc = QAT_Response_Str(QAT_RC_QUIET, "+EVT:lp_presleep");
    }

    if (evt == PWR_EVT_WMAC_FAIL_TO_SLEEP) {
        rc = QAT_Response_Str(QAT_RC_QUIET, "+EVT:lp_sleepfail");
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

