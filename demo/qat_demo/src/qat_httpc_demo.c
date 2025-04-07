/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/
#include "qapi_version.h"
#include "qapi_rtc.h"
#include "qapi_heap_status.h"
#include "qat.h"
#include "qat_api.h"
#include "nt_osal.h"
#include "httpc_demo.h"
#include "qapi_status.h"
#include "qapi_console.h"
#include "qcli_api.h"
#include "qat_httpc_demo.h"
#include "qapi_httpc.h"


/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/

qbool_t QAT_Data_Transfer_Mode_Handle(uint32_t Length, uint8_t *Buffer);

#if 0
static QAT_Command_Status_t Extend_Command_HttpHead(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{

   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;

   switch (Op_Type)
   {
      case QAT_OP_EXEC:		     /* AT+WRTMEM */
      {	
         snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCHEAD:<index>,<"req_header">\r\n");
         rc = QAT_Response_Str(QAT_RC_OK, buffer);

         break;
      }
      
      case QAT_OP_EXEC_W_PARAM: 	     /* AT+WRTMEM */
      {


         break;
      }
      
      default:
         ;
   }
   
//rlt:
       memset((void*)buffer, 0, HTTP_STR_BUFFER_LENGTH);
       return rc;

}
#endif

static QAT_Command_Status_t Extend_Command_HttpGetSize(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{

   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
   qapi_Status_t result = QAPI_OK;
   char buffer[HTTP_STR_BUFFER_LENGTH];
   char *url = NULL;
   int32_t timeout = TIMEOUT_MS;


   switch (Op_Type)
   {
      case QAT_OP_EXEC:		     /* AT+WRTMEM */
      {	
         snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPGETSIZE=<\"url\">,[<timeout>]\r\n");
         rc = QAT_Response_Str(QAT_RC_OK, buffer);
         break;
      }
      
      case QAT_OP_EXEC_W_PARAM: 	     /* AT+WRTMEM */
      {
        if(Parameter_Count > 2 || Parameter_Count < 1)
        {
            snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPGETSIZE: Invalid Parameter Count %d\r\n", Parameter_Count);
            rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
            goto rlt;
        }

        url = Parameter_List[0].String_Value;
        if(strlen(url) == 0)
        {
            if(g_https_cfg.url_size != 0)
            {
                url = g_https_cfg.url;
            }
            else
            {
                 snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPGETSIZE: there is no valid url\r\n");
                 rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                 goto rlt;
            }   
        }
        else
        {
            if(!validate_url(url))
            {
                snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPGETSIZE: Invalid url\r\n");
                rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                goto rlt;
            }
        }
 
        if(Parameter_Count == 2)
        {
            timeout = Parameter_List[1].Integer_Value;
            if(!Parameter_List[1].Integer_Is_Valid || (timeout < 0 || timeout>MAX_TIMEOUT_MS))
            {
                 snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPGETSIZE: Invalid timeout value\r\n");
                 rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                 goto rlt;
            }
        }

         result = at_httpc_getsize(url,timeout);
         break;
      }

      case QAT_OP_QUERY: 	     /* AT+WRTMEM */
      {

         break;
      }
      
      default:
         ;
   }
   
rlt:
    memset((void*)buffer, 0, HTTP_STR_BUFFER_LENGTH);
    return rc;

}

static QAT_Command_Status_t Extend_Command_HttpGet(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{

   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
   qapi_Status_t result = QAPI_OK;
   char buffer[HTTP_STR_BUFFER_LENGTH];
   char *url = NULL;
   int32_t timeout = TIMEOUT_MS;

   switch (Op_Type)
   {
      case QAT_OP_EXEC:		     /* AT+WRTMEM */
      {	
         snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCGET=<\"url\">,[<timeout>]\r\n");
         rc = QAT_Response_Str(QAT_RC_OK, buffer);

         break;
      }
      
      case QAT_OP_EXEC_W_PARAM: 	     /* AT+WRTMEM */
      {

        if(Parameter_Count > 2 || Parameter_Count < 1)
        {
             snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCGET: Invalid Parameter Count %d\r\n", Parameter_Count);
             rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
             goto rlt;
        }

        url = Parameter_List[0].String_Value;
        if(strlen(url) == 0)
        {
             if(g_https_cfg.url_size != 0)
             {
                url = g_https_cfg.url;
             }
             else
             {
                snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCGET: there is no valid url\r\n");
                rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                goto rlt;
             }   
        }
        else
        {
            if(!validate_url(url))
            {
                snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCGET: Invalid url\r\n");
                rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                goto rlt;
            }
        }

        if(Parameter_Count == 2)
        {
            timeout = Parameter_List[1].Integer_Value;
            if(!Parameter_List[1].Integer_Is_Valid || (timeout < 0 || timeout>MAX_TIMEOUT_MS))
            {
                 snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCGET: Invalid timeout value\r\n");
                 rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                 goto rlt;
            }
        }
        

        result = at_httpc_get(url,timeout);

        break;
      }
      
      default:
         ;
   }
   
rlt:
    memset((void*)buffer, 0, HTTP_STR_BUFFER_LENGTH);
    return rc;
}

static QAT_Command_Status_t Extend_Command_HttpPost(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{

   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
   qapi_Status_t result = QAPI_OK;
   char buffer[HTTP_STR_BUFFER_LENGTH];


   switch (Op_Type)
   {
      case QAT_OP_EXEC:		     /* AT+WRTMEM */
      {	
         snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPPOST=<\"url\">,<length>,[<\"http_req_header\">],[...]\r\n");
         rc = QAT_Response_Str(QAT_RC_OK, buffer);
         break;
      }
      
      case QAT_OP_EXEC_W_PARAM: 	     /* AT+WRTMEM */
      {
            if(Parameter_Count < 2)
            {
                snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPPOST:Invalid Parameter Count %d\r\n", Parameter_Count);
                rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                goto rlt;
            }


            if(!validate_url(Parameter_List[0].String_Value))
            {
                snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPPOST: Invalid url\r\n");
                rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                goto rlt;
            }

            if(FALSE == saveUrl(Parameter_List[0].String_Value))
            {
                snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPPOST:Save URL fail \r\n");
                rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                goto rlt;
            }

            if(strlen(g_https_cfg.temp_url) == 0)
            {
                if(g_https_cfg.url_size != 0)
                {
                    g_https_cfg.temp_url = g_https_cfg.url;
                }
                else
                {
                    snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPPOST:there is no valid url\r\n");
                    rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                    goto rlt;
                }   
            }

            if(!Parameter_List[1].Integer_Is_Valid || Parameter_List[1].Integer_Value < 1)
            {
                snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPPOST:Invalid length value\r\n");
                rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                goto rlt;
            }

            if (Parameter_List[1].Integer_Value > HTTP_BODY_BUFFER_SIZE) 
            {
                snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPPOST: data length can't bigger than %d:\r\n",HTTP_BODY_BUFFER_SIZE);
                rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                goto rlt;
            }

            g_https_cfg.data_len = Parameter_List[1].Integer_Value;
            
            //save header field
            if(Parameter_Count >2)
            {
                uint8_t headerfield_num = Parameter_Count -2;
                for(uint8_t i=0; i < headerfield_num; i++)
                {
                    if(FALSE == saveheaderfield(Parameter_List[2+i].String_Value))
                    {
                        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPPOST: save headerfield fail\r\n");
                        rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                        goto rlt;
                    }
                }
            }

            //malloc buffer
            if(!create_send_buffer(g_https_cfg.data_len))
            {
                snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPPOST: buffer malloc fail \r\n");
                rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                goto rlt;
            }
            
            extern Cur_Data_Mode_Cmd_t Cur_Data_Mode_Cmd;
            snprintf(Cur_Data_Mode_Cmd.cur_data_mode_commnd, AT_COMMAND_LENGTH, "+HTTPPOST");

            QAT_Transfer_Mode_set(QAT_Transfer_Mode_ONLINE_DATA_E,QAT_Data_Transfer_Mode_Handle);
            QAT_Response_Str(QAT_RC_OK, NULL);
            snprintf(buffer, HTTP_STR_BUFFER_LENGTH, ">\r\n");
            QAT_Response_Str(QAT_RC_QUIET_NO_CR, buffer);

            break;
      }

      case QAT_OP_EXEC_IN_DATA_MODEL:
      {
            uint32_t revlen = Parameter_Count;
            char * output_buf = (char*)Parameter_List;

            if(revlen < g_https_cfg.data_len)
            {
                if(g_https_cfg.send_buff == NULL){
                    if(!create_send_buffer(g_https_cfg.data_len))
                    {
                        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPPOST: buffer malloc fail \r\n");
                        rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                        QAT_Transfer_Mode_set(QAT_Transfer_Mode_AT_COMMAND_E,NULL);
                        goto rlt;
                    }
                }
                
                savedata(output_buf);

                if(g_https_cfg.buff_offset < g_https_cfg.data_len)
                {
                    QAT_Response_Str(QAT_RC_OK, NULL);
                    snprintf(buffer, HTTP_STR_BUFFER_LENGTH, ">\r\n");
                    QAT_Response_Str(QAT_RC_QUIET_NO_CR, buffer);
                }
                else
                {
                    result = at_httpc_post(g_https_cfg.temp_url,g_https_cfg.data_len,g_https_cfg.send_buff);

                    QAT_Transfer_Mode_set(QAT_Transfer_Mode_AT_COMMAND_E,NULL);
                    reset_temp_resource();
                }
            }
            else  //send directly
            {
                result = at_httpc_post(g_https_cfg.temp_url,g_https_cfg.data_len,output_buf);
                QAT_Transfer_Mode_set(QAT_Transfer_Mode_AT_COMMAND_E,NULL);
                reset_temp_resource();
            }
            
           break;
      }
      
      default:
         ;
   }
rlt:
   memset((void*)buffer, 0, HTTP_STR_BUFFER_LENGTH);
   return rc;
}

static QAT_Command_Status_t Extend_Command_HttpPut(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{

   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
   qapi_Status_t result = QAPI_OK;
   char buffer[HTTP_STR_BUFFER_LENGTH];

   switch (Op_Type)
   {
      case QAT_OP_EXEC:
      {
         snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPPUT=<\"url\">,<content_type>,<length>,[<\"http_req_header\">],[...]\r\n");
         rc = QAT_Response_Str(QAT_RC_OK, buffer);
         break;
      }

      case QAT_OP_EXEC_W_PARAM: 	     /* AT+WRTMEM */
      {
         if(Parameter_Count < 3)
         {
            snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPPUT:Invalid Parameter Count %d\r\n", Parameter_Count);
            rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
            goto rlt;
         }

         
         if(!validate_url(Parameter_List[0].String_Value))
         {
             snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPPUT: Invalid url\r\n");
             rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
             goto rlt;
         }
         
         if(FALSE == saveUrl(Parameter_List[0].String_Value))
         {
            snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPPUT:Save URL fail\r\n");
            rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
            goto rlt;
         }

         if(strlen(g_https_cfg.temp_url) == 0)
         {
            if(g_https_cfg.url_size != 0)
            {
               g_https_cfg.temp_url = g_https_cfg.url;
            }
            else
            {
               snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPPUT:there is no valid url\r\n");
               rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
               goto rlt;
            }   
         }
         
         uint8_t content_type = (uint8_t)Parameter_List[1].Integer_Value;
         if(!Parameter_List[1].Integer_Is_Valid || 
            (content_type < QAT_CONTENT_TYPE_X_WWW_FORM_URLENCODED || content_type > QAT_CONTENT_TYPE_TEXT_XML))
         {
            snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPPUT:Invalid content type\r\n");
            rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
            goto rlt;
         }

         if(!Parameter_List[2].Integer_Is_Valid || Parameter_List[2].Integer_Value < 1)
         {
            snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPPUT:Invalid length value\r\n");
            rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
            goto rlt;
         }

         if (Parameter_List[2].Integer_Value > HTTP_BODY_BUFFER_SIZE) 
         {
            snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPPUT: data length can't bigger than %d:\r\n",HTTP_BODY_BUFFER_SIZE);
            rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
            goto rlt;
         }

         //g_https_cfg.content_type = content_type;
         if(!save_content_type(content_type))
         {
             snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPPUT: save headerfield content_type fail\r\n");
             rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
             goto rlt;
         }
         
         g_https_cfg.data_len = Parameter_List[2].Integer_Value;

         if(Parameter_Count >3)
         {
           uint8_t headerfield_num = Parameter_Count -3;
           for(uint8_t i=0; i < headerfield_num; i++)
           {
                if(FALSE == saveheaderfield(Parameter_List[3+i].String_Value))
                {
                    snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPPUT: save headerfield fail\r\n");
                    rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                    goto rlt;
                }
           }
         }

        extern Cur_Data_Mode_Cmd_t Cur_Data_Mode_Cmd;
        snprintf(Cur_Data_Mode_Cmd.cur_data_mode_commnd, AT_COMMAND_LENGTH, "+HTTPPUT");

        QAT_Transfer_Mode_set(QAT_Transfer_Mode_ONLINE_DATA_E,QAT_Data_Transfer_Mode_Handle);
        QAT_Response_Str(QAT_RC_OK, NULL);
        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, ">\r\n");
        QAT_Response_Str(QAT_RC_QUIET_NO_CR, buffer);

        break;
      }

      case QAT_OP_EXEC_IN_DATA_MODEL:
      {
            uint32_t revlen = Parameter_Count;
            char * output_buf = (char*)Parameter_List;
            //printf("test revlen:%d\n",revlen);
            if(revlen < g_https_cfg.data_len)
            {
                if(g_https_cfg.send_buff == NULL){
                    if(!create_send_buffer(g_https_cfg.data_len))
                    {
                        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPPUT: buffer malloc fail \r\n");
                        rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                        QAT_Transfer_Mode_set(QAT_Transfer_Mode_AT_COMMAND_E,NULL);
                        goto rlt;
                    }
                }
                
                savedata(output_buf);

                if(g_https_cfg.buff_offset < g_https_cfg.data_len)
                {
                    QAT_Response_Str(QAT_RC_OK, NULL);
                    snprintf(buffer, HTTP_STR_BUFFER_LENGTH, ">\r\n");
                    QAT_Response_Str(QAT_RC_QUIET_NO_CR, buffer);
                }
                else
                {
                    //printf("test send data len:%d\n",g_https_cfg.data_len);
                    result = at_httpc_put(g_https_cfg.temp_url,g_https_cfg.data_len,g_https_cfg.send_buff);
                    QAT_Transfer_Mode_set(QAT_Transfer_Mode_AT_COMMAND_E,NULL);
                    reset_temp_resource();
                }
            }
            else  //send directly
            {
                result = at_httpc_put(g_https_cfg.temp_url,g_https_cfg.data_len,output_buf);
                QAT_Transfer_Mode_set(QAT_Transfer_Mode_AT_COMMAND_E,NULL);
                reset_temp_resource();
            }
      
            break;
      }
      
      default:
         ;
   }
   
rlt:
   memset((void*)buffer, 0, HTTP_STR_BUFFER_LENGTH);
   return rc;
}

static QAT_Command_Status_t Extend_Command_HttpUrlCfg(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    char buffer[HTTP_STR_BUFFER_LENGTH];

    switch (Op_Type)
    {
        case QAT_OP_EXEC:		     /* AT+WRTMEM */
        {	
             snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPURLCFG=<url length>\r\n");
             rc = QAT_Response_Str(QAT_RC_OK, buffer);
             break;
        }
      
        case QAT_OP_EXEC_W_PARAM: 	     /* AT+WRTMEM */
        {
        
            int len = 0;

            if(Parameter_List[0].Integer_Is_Valid)
            {
                 len = Parameter_List[0].Integer_Value;
            }
            else
            {
                snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPURLCFG: invalid url length \r\n");
                rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                goto rlt;
            }
            
            if (len < 0) {
                snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPURLCFG: invalid url length \r\n");
                rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                goto rlt;
            }
            
            if (len == 0) 
            {
                if (g_https_cfg.url != NULL) {
                    free(g_https_cfg.url);
                    g_https_cfg.url_size = 0;
                    g_https_cfg.url = NULL;
                }
                snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPURLCFG: delete url successfully \r\n");
                rc = QAT_Response_Str(QAT_RC_OK, buffer);
                goto rlt;
            }

            
            if (len > HTTP_URL_STR_BUFFER_LENGTH) 
            {
                snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPURLCFG: url length can't bigger than %d:\r\n",HTTP_URL_STR_BUFFER_LENGTH);
                rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                goto rlt;
            }

            if (g_https_cfg.url != NULL) {
                 free(g_https_cfg.url);
                 g_https_cfg.url = NULL;
            }
            
            //more 1 bit for '\0'
            g_https_cfg.url = malloc(len+1);  
            if (!g_https_cfg.url) {
                snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPURLCFG: malloc fail \r\n");
                rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                goto rlt;
            }

            memset(g_https_cfg.url, 0, len+1);
            g_https_cfg.url_size = len;
            g_https_cfg.buff_offset = 0;

            extern Cur_Data_Mode_Cmd_t Cur_Data_Mode_Cmd;
            snprintf(Cur_Data_Mode_Cmd.cur_data_mode_commnd, AT_COMMAND_LENGTH, "+HTTPURLCFG");

            QAT_Transfer_Mode_set(QAT_Transfer_Mode_ONLINE_DATA_E,QAT_Data_Transfer_Mode_Handle);
            QAT_Response_Str(QAT_RC_OK, NULL);
            snprintf(buffer, HTTP_STR_BUFFER_LENGTH, ">\r\n");
            QAT_Response_Str(QAT_RC_QUIET_NO_CR, buffer);
            break;
      }
      case QAT_OP_EXEC_IN_DATA_MODEL:
      {
            //revlen is contain the '\0'
            uint32_t revlen = Parameter_Count;
            char * output_buf = (char*)Parameter_List;
            if(revlen > 0)
            {
                g_https_cfg.buff_offset += 
                        snprintf((char*)(g_https_cfg.url + g_https_cfg.buff_offset), 
                        g_https_cfg.url_size - g_https_cfg.buff_offset +1,"%s", output_buf) - 1;
            }

            if(g_https_cfg.buff_offset < g_https_cfg.url_size)
            {
                QAT_Response_Str(QAT_RC_OK, NULL);
                snprintf(buffer, HTTP_STR_BUFFER_LENGTH, ">\r\n");
                QAT_Response_Str(QAT_RC_QUIET_NO_CR, buffer);
            }
            else
            {
               //validate the URL
               if(validate_url(g_https_cfg.url))
               {
                    rc = QAT_Response_Str(QAT_RC_OK, NULL);
               }
               else
               {
                    if (g_https_cfg.url != NULL) {
                        free(g_https_cfg.url);
                        g_https_cfg.url_size = 0;
                        g_https_cfg.url = NULL;
                    }
                    
                    snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPURLCFG: invalid url \r\n");
                    rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
               }
               
                QAT_Transfer_Mode_set(QAT_Transfer_Mode_AT_COMMAND_E,NULL);
                reset_temp_resource();
            }
  
            break;
      }

      case QAT_OP_QUERY:
      {	
         snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPURLCFG:%d,%s\r\n",g_https_cfg.url_size,g_https_cfg.url);
         rc = QAT_Response_Str(QAT_RC_OK, buffer);
         
         break;
      }
      
      default:
         ;
   }
   
rlt:
   memset((void*)buffer, 0, HTTP_STR_BUFFER_LENGTH);
   return rc;
}

static QAT_Command_Status_t Extend_Command_HttpSslCfg(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{
   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
   qapi_Status_t result = QAPI_OK;
   qbool_t  isIntegerValid = false;
   char buffer[HTTP_STR_BUFFER_LENGTH];

   switch (Op_Type)
   {
      case QAT_OP_EXEC:		     /* AT+WRTMEM */
      {	
        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPSSLCFG=<scheme>,[<\"cert_file\">],[<\"key_file\">],[<\"ca_file\">]\r\n");
        rc = QAT_Response_Str(QAT_RC_OK, buffer);
        break;
      }
      
      case QAT_OP_EXEC_W_PARAM: 	     /* AT+WRTMEM */
      {
        int scheme;
        int cert_len = 0;
        int key_len = 0;
        int ca_len = 0;
#if 0
        char cert_file[32] = {0};
        char key_file[32] = {0};
        char ca_file[32] = {0};
        int cert_file_valid = 0, key_file_valid = 0, ca_file_valid = 0;

        AT_CMD_PARSE_OPT_STRING(1, cert_file, sizeof(cert_file), cert_file_valid);
        AT_CMD_PARSE_OPT_STRING(2, key_file, sizeof(key_file), key_file_valid);
        AT_CMD_PARSE_OPT_STRING(3, ca_file, sizeof(ca_file), ca_file_valid);

        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "test cert_file:%s,cert_file_valid:%d\r\n",cert_file,cert_file_valid);
        QAT_Response_Str(QAT_RC_OK, buffer);
#endif
        scheme = Parameter_List[0].Integer_Value;
        isIntegerValid = Parameter_List[0].Integer_Is_Valid;

        if(!isIntegerValid||(scheme>3) )
        {
           snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "Invalid scheme value\r\n");
           rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
           goto rlt;
        }

        if(Parameter_Count>4 )
        {
           snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "Invalid Parameter Count %d\r\n", Parameter_Count);
           rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
           goto rlt;
        }
       
        if(Parameter_Count>=1)
        {
           if(scheme == AT_HTTPS_NOT_AUTH)
           {
               resetSslInfo();
           }
           else if(scheme == AT_HTTPS_SERVER_AUTH)
           {
              if(Parameter_Count==2)
              {
                  ca_len = strlen(Parameter_List[1].String_Value);
                  
                  if(ca_len > 0 && ca_len <= FILE_PATH_STR_BUFFER_LENGTH)
                  {
                    resetSslInfo();
                    memcpy(g_https_cfg.ca_file, Parameter_List[1].String_Value, ca_len);
                  }
                  else
                  {
                    snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "the CA path is invalid \r\n");
                    rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                    goto rlt;
                  }
              }
              else
              {
                  snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "Parameter no match\r\n");
                  rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                  goto rlt;

              }
           }
           else if(scheme == AT_HTTPS_CLIENT_AUTH)
           {
              if(Parameter_Count==3)
              {
                  cert_len = strlen(Parameter_List[1].String_Value);
                  key_len = strlen(Parameter_List[2].String_Value);
                  
                  if((cert_len > 0 && cert_len <= FILE_PATH_STR_BUFFER_LENGTH) 
                    ||(key_len > 0 && key_len <= FILE_PATH_STR_BUFFER_LENGTH))
                  {
                    resetSslInfo();
                    memcpy(g_https_cfg.cert_file, Parameter_List[1].String_Value, cert_len);
                    memcpy(g_https_cfg.key_file, Parameter_List[2].String_Value, key_len);
                  }
                  else
                  {
                    snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "the CERT|KEY path is invalid \r\n");
                    rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                    goto rlt;
                  }
              }
              else
              {
                  snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "Parameter no match\r\n");
                  rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                  goto rlt;

              }
           }
           else if(scheme == AT_HTTPS_BOTH_AUTH)
           {
              if(Parameter_Count==4)
              {
                  cert_len = strlen(Parameter_List[1].String_Value);
                  key_len = strlen(Parameter_List[2].String_Value);
                  ca_len = strlen(Parameter_List[3].String_Value);
                  
                  if((cert_len > 0 && cert_len <= FILE_PATH_STR_BUFFER_LENGTH) 
                    ||(key_len > 0 && key_len <= FILE_PATH_STR_BUFFER_LENGTH)
                    ||(ca_len > 0 && ca_len <= FILE_PATH_STR_BUFFER_LENGTH))
                  {
                    resetSslInfo();
                    memcpy(g_https_cfg.cert_file, Parameter_List[1].String_Value, cert_len);
                    memcpy(g_https_cfg.key_file, Parameter_List[2].String_Value, key_len);
                    memcpy(g_https_cfg.ca_file, Parameter_List[3].String_Value, ca_len);
                  }
                  else
                  {
                    snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "the path is invalid \r\n");
                    rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                    goto rlt;
                  }
              }
              else
              {
                  snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "Parameter no match\r\n");
                  rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                  goto rlt;

              }
           }
           
           if(cert_len>FILE_PATH_STR_BUFFER_LENGTH ||key_len>FILE_PATH_STR_BUFFER_LENGTH||ca_len>FILE_PATH_STR_BUFFER_LENGTH)
           {
            snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "the path length should not be mroe than 32!\r\n");
            rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
            goto rlt;

           }
        }

        g_https_cfg.https_auth_type = scheme;
        QAT_Response_Str(QAT_RC_OK, NULL);
        
        break;
      }

      case QAT_OP_QUERY:
      {

        snprintf(buffer, HTTP_STR_BUFFER_LENGTH,"+HTTPSSLCFG:%d,\"%s\",\"%s\",\"%s\"\r\n", 
        g_https_cfg.https_auth_type, g_https_cfg.cert_file, g_https_cfg.key_file, g_https_cfg.ca_file);

        rc = QAT_Response_Str(QAT_RC_OK, buffer);
        break;
      }
      
      default:
         ;
   }
rlt:
   memset((void*)buffer, 0, HTTP_STR_BUFFER_LENGTH);
   return rc;
}

static QAT_Command_Status_t Extend_Command_HttpNetCfg(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{
   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
   qapi_Status_t result = QAPI_OK;
   qbool_t  isIntegerValid = false;
   char buffer[HTTP_STR_BUFFER_LENGTH];

   switch (Op_Type)
   {
      case QAT_OP_EXEC:		     /* AT+WRTMEM */
      {	
        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPNETCFG=<type>,<port>\r\n");
        rc = QAT_Response_Str(QAT_RC_OK, buffer);
        break;
      }
      
      case QAT_OP_EXEC_W_PARAM: 	     /* AT+WRTMEM */
      {

        if(Parameter_Count !=2)
        {
           snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "Invalid Parameter Count %d\r\n", Parameter_Count);
           rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
           goto rlt;
        }
        
        int type = Parameter_List[0].Integer_Value;
        isIntegerValid = Parameter_List[0].Integer_Is_Valid;

        if(!isIntegerValid||(type>2 ||type<1) )
        {
           snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "Invalid type value\r\n");
           rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
           goto rlt;
        }

        int port = Parameter_List[1].Integer_Value;
        isIntegerValid = Parameter_List[1].Integer_Is_Valid;
        
        if(!isIntegerValid)
        {
           snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "Invalid port value\r\n");
           rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
           goto rlt;
        }

        if(type == QAT_HTTP)
        {
            g_https_cfg.http_port = port;
            g_https_cfg.http_port_set = TRUE;

            //0 is reserved
            if(port == 0)
               g_https_cfg.http_port_set = FALSE; 

        }
        else if(type == QAT_HTTPS)
        {
            g_https_cfg.https_port = port;
            g_https_cfg.https_port_set = TRUE;

            //0 is reserved
            if(port == 0)
               g_https_cfg.https_port_set = FALSE;
        }
        
        QAT_Response_Str(QAT_RC_OK, NULL);
        
        break;
      }

      case QAT_OP_QUERY:
      {
        snprintf(buffer, HTTP_STR_BUFFER_LENGTH,"+HTTPNETCFG:http port:%d, https port:%d\r\n", 
        g_https_cfg.http_port, g_https_cfg.https_port);

        rc = QAT_Response_Str(QAT_RC_OK, buffer);
        break;
      }
      
      default:
         ;
   }
rlt:
   memset((void*)buffer, 0, HTTP_STR_BUFFER_LENGTH);
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
static QAT_Command_Status_t Extend_Command_HttpClient(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{

   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
   qapi_Status_t result = QAPI_OK;
   qbool_t  isIntegerValid = false;
   char buffer[HTTP_STR_BUFFER_LENGTH];
   char *url = NULL;
   char *data_buf = NULL;
   
   switch (Op_Type)
   {
      case QAT_OP_EXEC:		     /* AT+WRTMEM */
      {	
         snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCLIENT=<opt>,<content-type>,<\"url\">,[<\"data\">],[<\"http_req_header\">],[...]\r\n");
         rc = QAT_Response_Str(QAT_RC_OK, buffer);

         break;
      }
      
      case QAT_OP_EXEC_W_PARAM: 	     /* AT+WRTMEM */
      {
      
        if(Parameter_Count<3)
        {
           snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPC: Invalid Parameter Count %d\r\n", Parameter_Count);
           rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
           goto rlt;
        }
        
        int32_t opt = Parameter_List[0].Integer_Value;

        if(!Parameter_List[0].Integer_Is_Valid || (opt < QAT_HTTP_CLIENT_HEAD || opt > QAT_HTTP_CLIENT_PUT) )
        {
           snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPC: Invalid opt type\r\n");
           rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
           goto rlt;
        }
        
        int32_t content_type = Parameter_List[1].Integer_Value;
        
        if(!Parameter_List[1].Integer_Is_Valid || 
            (content_type < QAT_CONTENT_TYPE_X_WWW_FORM_URLENCODED || content_type > QAT_CONTENT_TYPE_TEXT_XML))
        {
           snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPC: Invalid content type\r\n");
           rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
           goto rlt;
        }

        url = Parameter_List[2].String_Value;

         if(strlen(url) == 0)
         {
            if(g_https_cfg.url_size != 0)
            {
               url = g_https_cfg.url;
            }
            else
            {
               snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPC: there is no valid url\r\n");
               rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
               goto rlt;
            }   
         }
         else
         {
            if(!validate_url(url))
            {
                snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPC: invalid url\r\n");
                rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                goto rlt;
            }
         }

         if(opt == QAT_HTTP_CLIENT_POST || opt == QAT_HTTP_CLIENT_PUT)
         {
            if(Parameter_Count >= 4)
            {
                data_buf = Parameter_List[3].String_Value;
                
                if(Parameter_Count >4)
                {
                   uint8_t headerfield_num = Parameter_Count - 4;
                   for(uint8_t i=0; i < headerfield_num; i++)
                   {
                        if(FALSE == saveheaderfield(Parameter_List[4+i].String_Value))
                        {
                            snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPC: save headerfield fail\r\n");
                            rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                            goto rlt;
                        }
                   }
                }
            }
            else
            {
                snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPC: Invalid Parameter Count\r\n");
                rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                goto rlt;
            }
            
         }
         else if(opt == QAT_HTTP_CLIENT_HEAD || opt == QAT_HTTP_CLIENT_GET)
         {
            if(Parameter_Count >=4)
            {
                uint8_t headerfield_num = Parameter_Count - 3;
                for(uint8_t i=0; i < headerfield_num; i++)
                {
                     if(FALSE == saveheaderfield(Parameter_List[3+i].String_Value))
                     {
                         snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPC: save headerfield fail\r\n");
                         rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                         goto rlt;
                     }
                }
            }
         }
         
        //g_https_cfg.content_type = content_type;
        if(!save_content_type(content_type))
        {
            snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPC: save headerfield content_type fail\r\n");
            rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
            goto rlt;
        }

        result = at_httpc_request(opt,url,data_buf);

        break;
      }
      
      default:
         ;
   }

rlt:
   memset((void*)buffer, 0, HTTP_STR_BUFFER_LENGTH);
   reset_temp_resource();
   return rc;
}

void Initialize_QAT_HttpC_Demo (void)
{
	qbool_t RetVal;
	RetVal = QAT_Register_Command_Group(QAT_HTTPC_Command_List, HTTPC_COMMAND_LIST_SIZE);
	if(RetVal == false)
   {
      printf("Failed to register HTTPC command group.\n");
   }
}

qapi_Status_t at_httpc_start ()
{
    qapi_Status_t rlt = QAPI_OK;

    //Construct start Command
    //httpc start
    uint32_t  Parameter_Count =1;
    QAPI_Console_Parameter_t Parameter_List[Parameter_Count];
    Parameter_List[0].Integer_Is_Valid =false;
    Parameter_List[0].String_Value ="start";
    rlt = httpc_command_handler(Parameter_Count,Parameter_List);

    return rlt;
}

qapi_Status_t at_httpc_new_session (char *url,int32_t timeout)
{
    qapi_Status_t rlt = QAPI_OK;

    //Construct new session Command
    //httpc new [-t <timeout_ms> -b <body_buffer_size> -h <header_buffer_size> -r <rx_buffer_size> -s -c <calist>]
    uint32_t  Parameter_Count =0;
    QAPI_Console_Parameter_t Parameter_List[QAT_HTTPC_MAXIMUM_NUMBER_OF_PARAMETERS];
    Parameter_List[Parameter_Count].Integer_Is_Valid =false;
    Parameter_List[Parameter_Count].String_Value ="new";
    Parameter_Count++;
 
    Parameter_List[Parameter_Count].String_Value = "-t";
    Parameter_Count++;
    Parameter_List[Parameter_Count].Integer_Is_Valid =true;
    Parameter_List[Parameter_Count].Integer_Value = timeout;
    Parameter_Count++;

    if(isSecureSession(url))
    {
        int scheme = g_https_cfg.https_auth_type;
        if (scheme == AT_HTTPS_NOT_AUTH) {
           //nothing to do
        } else if (scheme == AT_HTTPS_SERVER_AUTH) {
    
           Parameter_List[Parameter_Count].String_Value ="-s";
           Parameter_Count++;
           Parameter_List[Parameter_Count].String_Value ="-a";
           Parameter_Count++;
           Parameter_List[Parameter_Count].String_Value = g_https_cfg.ca_file;
           Parameter_Count++;
    
        } else if (scheme == AT_HTTPS_CLIENT_AUTH) {
            
           Parameter_List[Parameter_Count].String_Value ="-s";
           Parameter_Count++;
           Parameter_List[Parameter_Count].String_Value ="-c";
           Parameter_Count++;
           Parameter_List[Parameter_Count].String_Value = g_https_cfg.cert_file;
           Parameter_Count++;
           Parameter_List[Parameter_Count].String_Value ="-k";
           Parameter_Count++;
           Parameter_List[Parameter_Count].String_Value = g_https_cfg.key_file;
           Parameter_Count++;
           
        } else if (scheme == AT_HTTPS_BOTH_AUTH) {
            
           Parameter_List[Parameter_Count].String_Value ="-s";
           Parameter_Count++;
           Parameter_List[Parameter_Count].String_Value ="-c";
           Parameter_Count++;
           Parameter_List[Parameter_Count].String_Value = g_https_cfg.cert_file;
           Parameter_Count++;
           Parameter_List[Parameter_Count].String_Value ="-k";
           Parameter_Count++;
           Parameter_List[Parameter_Count].String_Value = g_https_cfg.key_file;
           Parameter_Count++;
           Parameter_List[Parameter_Count].String_Value ="-a";
           Parameter_Count++;
           Parameter_List[Parameter_Count].String_Value = g_https_cfg.ca_file;
           Parameter_Count++;
        }
    }
    
    rlt = httpc_command_handler(Parameter_Count,Parameter_List);

   return rlt;
}

qapi_Status_t at_httpc_conn(char *url)
{
    qapi_Status_t rlt = QAPI_OK;
    char host[HTTP_HOST_STR_BUFFER_LENGTH];
    
    //reset flag at_httpc_stop
    at_rec_state = QAPI_NET_HTTPC_RX_MORE_DATA;
    
    //Construct connect Command
    //httpc conn <client_num> <origin_server or proxy> [<port>]
    uint32_t  Parameter_Count =0;
    QAPI_Console_Parameter_t Parameter_List[QAT_HTTPC_MAXIMUM_NUMBER_OF_PARAMETERS];
    Parameter_List[Parameter_Count].Integer_Is_Valid =false;
    Parameter_List[Parameter_Count].String_Value ="conn";
    Parameter_Count++;

    Parameter_List[Parameter_Count].Integer_Is_Valid =true;
    Parameter_List[Parameter_Count].Integer_Value = QAT_HTTPC_CLIENT_INDEX;
    Parameter_Count++;

    gethostURL(url,host);
    Parameter_List[Parameter_Count].Integer_Is_Valid =false;
    Parameter_List[Parameter_Count].String_Value = host;
    Parameter_Count++;
    printf("conn host:%s\r\n",host);

    
    Parameter_List[Parameter_Count].Integer_Is_Valid =true;
    if(isSecureSession(url))
    {
        if(g_https_cfg.https_port_set)
        {
            Parameter_List[Parameter_Count].Integer_Value = g_https_cfg.https_port;
        }
        else
        {
            Parameter_List[Parameter_Count].Integer_Value = HTTPS_DEFAULT_PORT;
        }
    }
    else
    {
        if(g_https_cfg.http_port_set)
        {
            Parameter_List[Parameter_Count].Integer_Value = g_https_cfg.http_port;
        }
        else
        {
            Parameter_List[Parameter_Count].Integer_Value = HTTP_DEFAULT_PORT;
        }
    }
    
    Parameter_Count++;
    
    rlt = httpc_command_handler(Parameter_Count,Parameter_List);

    return rlt;
}

qapi_Status_t at_httpc_disconn (int32_t client_num)
{
    qapi_Status_t rlt = QAPI_OK;
    char host[HTTP_HOST_STR_BUFFER_LENGTH];

    //Construct connect Command
    //httpc disconnect <client_num>
    uint32_t  Parameter_Count =0;
    QAPI_Console_Parameter_t Parameter_List[QAT_HTTPC_MAXIMUM_NUMBER_OF_PARAMETERS];
    Parameter_List[Parameter_Count].Integer_Is_Valid =false;
    Parameter_List[Parameter_Count].String_Value ="disconnect";
    Parameter_Count++;

    Parameter_List[Parameter_Count].Integer_Is_Valid =true;
    Parameter_List[Parameter_Count].Integer_Value = client_num;
    Parameter_Count++;

    rlt = httpc_command_handler(Parameter_Count,Parameter_List);

    return rlt;
}


qapi_Status_t at_httpc_stop()
{
    qapi_Status_t rlt = QAPI_OK;

    //Construct start Command
    //httpc stop
    uint32_t  Parameter_Count =1;
    QAPI_Console_Parameter_t Parameter_List[Parameter_Count];
    Parameter_List[0].Integer_Is_Valid =false;
    Parameter_List[0].String_Value ="stop";
    rlt = httpc_command_handler(Parameter_Count,Parameter_List);
    
    at_httpc_method = 0;
    
    return rlt;
}

#if 0
qapi_Status_t at_httpc_prepare (char *url, int32_t timeout)
{
    qapi_Status_t rlt = QAPI_OK;
    char buffer[HTTP_STR_BUFFER_LENGTH];

#if 0
    //httpc stop
    rlt = at_httpc_stop();
    if(rlt != QAPI_OK)
    {
        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCLIENT: client stop fail\r\n");
        QAT_Response_Str(QAT_RC_OK, buffer);
        goto endpiont;
    }
#endif

    //httpc start
    rlt = at_httpc_start();
    if(rlt != QAPI_OK)
    {
        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCLIENT: client start fail\r\n");
        QAT_Response_Str(QAT_RC_OK, buffer);
        goto endpiont;
    }

    //httpc new session
     rlt = at_httpc_new_session(url,timeout);
    if(rlt != QAPI_OK)
    {
        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCLIENT: session setup fail\r\n");
        QAT_Response_Str(QAT_RC_OK, buffer);
        goto endpiont;
    }
    
    //httpc conn
    rlt = at_httpc_conn(url);
    if(rlt != QAPI_OK)
    {
        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCLIENT: connection fail\r\n");
        QAT_Response_Str(QAT_RC_OK, buffer);
        goto endpiont;
    }
    
endpiont:
    memset((void*)buffer, 0, HTTP_STR_BUFFER_LENGTH);
    return rlt;
}
#endif
qapi_Status_t at_httpc_setparameter(char *data_buf)
{
    qapi_Status_t rlt = QAPI_OK;
    char buffer[HTTP_STR_BUFFER_LENGTH];

    //process key/value data, example: key1=value1&key2=value2
    if(strstr(data_buf,"="))
    {
        uint32_t  Parameter_Count =0;
        QAPI_Console_Parameter_t Parameter_List[QAT_HTTPC_MAXIMUM_NUMBER_OF_PARAMETERS];
        Parameter_List[0].String_Value = "setparam";
        Parameter_Count++;
        Parameter_List[Parameter_Count].Integer_Is_Valid = true;
        Parameter_List[Parameter_Count].Integer_Value = QAT_HTTPC_CLIENT_INDEX;
        Parameter_Count++;

        
        uint32_t pair_count = 0;
        QAPI_Console_Parameter_t Parameter_List_temp[QAT_HTTPC_MAXIMUM_NUMBER_OF_PARAMETERS];
        pair_count = splitKeyValuePairs(data_buf, Parameter_List_temp);
        
        uint32_t Parameter_Count_temp = Parameter_Count;
        for(uint32_t i =0; i <pair_count; i++)
        {
            Parameter_Count = Parameter_Count_temp;
            Parameter_List[Parameter_Count].String_Value = Parameter_List_temp[2*i].String_Value;
            Parameter_Count++;
            Parameter_List[Parameter_Count].String_Value = Parameter_List_temp[2*i+1].String_Value;
            Parameter_Count++;
            
            rlt = httpc_command_handler(Parameter_Count,Parameter_List);
            if(rlt != QAPI_OK)
            {
               snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPC:set data fail\r\n");
               QAT_Response_Str(QAT_RC_ERROR, buffer);
               goto endpiont;
             }
        }
    }
    
 endpiont:
    memset((void*)buffer, 0, HTTP_STR_BUFFER_LENGTH);
    return rlt;
}

qapi_Status_t at_httpc_addheaderfield(/*uint8_t headfield_type,*/uint8_t index)
{
    qapi_Status_t rlt = QAPI_OK;
    char buffer[HTTP_STR_BUFFER_LENGTH];

    uint32_t  Parameter_Count =0;
    QAPI_Console_Parameter_t Parameter_List[QAT_HTTPC_MAXIMUM_NUMBER_OF_PARAMETERS];
    Parameter_List[Parameter_Count].String_Value = "addheaderfield";
    Parameter_Count++;
    Parameter_List[Parameter_Count].Integer_Is_Valid = true;
    Parameter_List[Parameter_Count].Integer_Value = QAT_HTTPC_CLIENT_INDEX;
    Parameter_Count++;
    
    Parameter_List[Parameter_Count].String_Value = g_https_cfg.header_field[index].name;
    Parameter_Count++;

    Parameter_List[Parameter_Count].String_Value = g_https_cfg.header_field[index].value;
    Parameter_Count++;

    printf("test set header name:%s, value:%s\n",g_https_cfg.header_field[index].name,g_https_cfg.header_field[index].value);

    rlt = httpc_command_handler(Parameter_Count,Parameter_List);
    if(rlt != QAPI_OK)
    {
       snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPC:add headerfield fail\r\n");
       QAT_Response_Str(QAT_RC_ERROR, buffer);
     }

    memset((void*)buffer, 0, HTTP_STR_BUFFER_LENGTH);
    return rlt;
}


qapi_Status_t at_httpc_setbodydata(char *data_buf,uint32_t len)
{
    qapi_Status_t rlt = QAPI_OK;
    char buffer[HTTP_STR_BUFFER_LENGTH];

    //httpc setbody <client_num> [<len>]
    uint32_t  Parameter_Count =0;
    QAPI_Console_Parameter_t Parameter_List[QAT_HTTPC_MAXIMUM_NUMBER_OF_PARAMETERS];
    Parameter_List[Parameter_Count].String_Value = "setbody";
    Parameter_Count++;
    Parameter_List[Parameter_Count].Integer_Is_Valid = true;
    Parameter_List[Parameter_Count].Integer_Value = QAT_HTTPC_CLIENT_INDEX;
    Parameter_Count++;


    Parameter_List[Parameter_Count].Integer_Is_Valid = true;
    Parameter_List[Parameter_Count].Integer_Value = len;
    Parameter_Count++;
    Parameter_List[Parameter_Count].String_Value = data_buf;
    Parameter_Count++;

    rlt = httpc_command_handler(Parameter_Count,Parameter_List);
    if(rlt != QAPI_OK)
    {
       snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPC:set data fail\r\n");
       QAT_Response_Str(QAT_RC_ERROR, buffer);
    }

    memset((void*)buffer, 0, HTTP_STR_BUFFER_LENGTH);
    return rlt;

}

qapi_Status_t at_httpc_request (int32_t opt, char *url, char *data_buf)
{
    qapi_Status_t rlt = QAPI_OK;
    char buffer[HTTP_STR_BUFFER_LENGTH]= {0};
    char path_url[HTTP_URL_STR_BUFFER_LENGTH]= {0};
    uint16 count = 0;
    qbool_t conn_enable = FALSE;

    //httpc stop
    rlt = at_httpc_stop();
    if(rlt != QAPI_OK)
    {
        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCLIENT: client stop fail\r\n");
        QAT_Response_Str(QAT_RC_ERROR, buffer);
        goto endpiont;
    }

    //httpc start
    rlt = at_httpc_start();
    if(rlt != QAPI_OK)
    {
        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCLIENT: client start fail\r\n");
        QAT_Response_Str(QAT_RC_ERROR, buffer);
        goto endpiont;
    }

    //httpc new session
     rlt = at_httpc_new_session(url,TIMEOUT_MS);
    if(rlt != QAPI_OK)
    {
        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCLIENT: session setup fail\r\n");
        QAT_Response_Str(QAT_RC_ERROR, buffer);
        goto endpiont;
    }
    
    //httpc conn
    rlt = at_httpc_conn(url);
    if(rlt != QAPI_OK)
    {
        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCLIENT: connection fail\r\n");
        QAT_Response_Str(QAT_RC_ERROR, buffer);
        goto endpiont;
    }
    conn_enable = TRUE;

    if(g_https_cfg.header_field_num > 0)
    {
       for(uint8_t i=0; i < g_https_cfg.header_field_num; i++)
       {
           rlt = at_httpc_addheaderfield(i);
           if(rlt != QAPI_OK)
           {
               snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCLIENT: add headerfield fail, index:%d\r\n",i);
               QAT_Response_Str(QAT_RC_ERROR, buffer);
               goto endpiont;
   
           }
       }
        
    }

    //httpc setparam <client_num> <key> <value>
    if((data_buf != NULL) && (opt==QAT_HTTP_CLIENT_POST ||opt==QAT_HTTP_CLIENT_PUT))
    {
        //process key/value data, example: key1=value1&key2=value2
#if 0
        if(strstr(data_buf,"="))
        {
            rlt = at_httpc_setparameter(data_buf);
            if(rlt!= QAPI_OK)
            {
                goto endpiont;
            }
        }
        else
        {
            //httpc setbody data
            uint32_t revlen = strlen(data_buf);
            rlt = at_httpc_setbodydata(data_buf,revlen);
            if(rlt!= QAPI_OK)
            {
                goto endpiont;
            }
        }
#endif
       //httpc setbody data
       uint32_t revlen = strlen(data_buf);
       rlt = at_httpc_setbodydata(data_buf,revlen);
       if(rlt!= QAPI_OK)
       {
           goto endpiont;
       }
    }


    //Construct client request Command
    //httpc {get | head | post | put | delete | patch} <client_num> [<url>] [<chunk_flag>] [<chunk_size>]
    uint32_t  Parameter_Count =0;
    QAPI_Console_Parameter_t Parameter_List[QAT_HTTPC_MAXIMUM_NUMBER_OF_PARAMETERS];
    //memset(Parameter_List, 0, sizeof(Parameter_List));

    Parameter_List[0].Integer_Is_Valid =false;
    switch(opt)
    {
        case QAT_HTTP_CLIENT_HEAD:
        {
           Parameter_List[0].String_Value = "head";
           Parameter_Count++;
           at_httpc_method = QAT_HTTP_HEAD;
           break;
        }
        case QAT_HTTP_CLIENT_GET:
        {
           Parameter_List[0].String_Value = "get";
           Parameter_Count++;
           at_httpc_method = QAT_HTTP_GET;
           break;
        }
        case QAT_HTTP_CLIENT_POST:
        {

           Parameter_List[0].String_Value = "post";
           Parameter_Count++;
           at_httpc_method = QAT_HTTP_POST;

           break;
        }
        case QAT_HTTP_CLIENT_PUT:
        {
           Parameter_List[0].String_Value = "put";
           Parameter_Count++;
           at_httpc_method = QAT_HTTP_PUT;

           break;
        }

        default:
         ;
            
    }
    
    Parameter_List[Parameter_Count].Integer_Is_Valid = true;
    Parameter_List[Parameter_Count].Integer_Value = QAT_HTTPC_CLIENT_INDEX;
    Parameter_Count++;

    if(!getpathURL(url,path_url))
    {
        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCLIENT: get object url fail\r\n");
        QAT_Response_Str(QAT_RC_ERROR, buffer);
        rlt = QAPI_ERR_INVALID_PARAM;
        goto endpiont;
    }
    Parameter_List[Parameter_Count].Integer_Is_Valid =false;
    Parameter_List[Parameter_Count].String_Value = path_url;
    Parameter_Count++;

    rlt = httpc_command_handler(Parameter_Count,Parameter_List);
    if(rlt != QAPI_OK)
    {
        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCLIENT: handle client request fail\r\n");
        QAT_Response_Str(QAT_RC_ERROR, buffer);
        goto endpiont;
    }


endpiont:

   while(1)
   {
      if((at_rec_state <= QAPI_NET_HTTPC_RX_FINISHED) || (rlt != QAPI_OK))
      {
           if(at_rec_state == QAPI_NET_HTTPC_RX_FINISHED)
           {
              if((at_httpc_method == QAT_HTTP_POST)||(at_httpc_method == QAT_HTTP_PUT))
              {
                  snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCLIENT: SEND OK, Resp_code: %d\r\n",at_rec_error_code);
                  QAT_Response_Str(QAT_RC_OK, buffer);
              }
              else
              {
                  QAT_Response_Str(QAT_RC_OK, NULL);
              }
           }
           else if(at_rec_state < QAPI_NET_HTTPC_RX_FINISHED)
           {
                if((at_httpc_method == QAT_HTTP_POST)||(at_httpc_method == QAT_HTTP_PUT))
                 {
                    snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCLIENT: SEND FAIL, error_code:%d\r\n",at_rec_state);
                    QAT_Response_Str(QAT_RC_ERROR, buffer);
                 }
                 else
                 {
                    snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCLIENT: operation FAIL, error_code:%d\r\n",at_rec_state);
                    QAT_Response_Str(QAT_RC_ERROR, buffer);
                 }
           }
           else
           {
                if((at_httpc_method == QAT_HTTP_POST)||(at_httpc_method == QAT_HTTP_PUT))
                {
                    snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCLIENT: SEND FAIL, error_code:%d\r\n",rlt);
                    QAT_Response_Str(QAT_RC_ERROR, buffer);
                }
                else
                {
                    snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCLIENT: operation FAIL, error_code:%d\r\n",rlt);
                    QAT_Response_Str(QAT_RC_ERROR, buffer);
                }
           }
           
           //httpc disconn
           if(conn_enable)
           {
              rlt = at_httpc_disconn(QAT_HTTPC_CLIENT_INDEX);
              if(rlt != QAPI_OK)
              {
                  snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCLIENT: session disconn fail\r\n");
                  QAT_Response_Str(QAT_RC_ERROR, buffer);
              }
           }
           
           //httpc stop
           rlt = at_httpc_stop();
           if(rlt != QAPI_OK)
           {
               snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCLIENT: client stop fail\r\n");
               QAT_Response_Str(QAT_RC_ERROR, buffer);
           }

         break;
      }
   
      sleep(1);
      count++;

      if(count >= HTTP_WAIT_RSP_TIME)
      {
        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCLIENT: timeout\r\n");
        QAT_Response_Str(QAT_RC_ERROR, buffer);

        //httpc disconn
        if(conn_enable)
        {
            rlt = at_httpc_disconn(QAT_HTTPC_CLIENT_INDEX);
            if(rlt != QAPI_OK)
            {
               snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCLIENT: session disconn fail\r\n");
               QAT_Response_Str(QAT_RC_ERROR, buffer);
            }
        }
        
        //httpc stop
        rlt = at_httpc_stop();
        if(rlt != QAPI_OK)
        {
           snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCLIENT: client stop fail\r\n");
           QAT_Response_Str(QAT_RC_ERROR, buffer);
        }
        break;
      }
   }

   memset((void*)buffer, 0, HTTP_STR_BUFFER_LENGTH);
   return rlt;
}

//get the contentlength the from head
qapi_Status_t at_httpc_getsize (char *url, int32_t timeout)
{
    qapi_Status_t rlt = QAPI_OK;
    char buffer[HTTP_STR_BUFFER_LENGTH]= {0};
    char path_url[HTTP_URL_STR_BUFFER_LENGTH]= {0};
    uint16 count = 0;
    uint32 max_time_wait = timeout/1000 < 1?1:timeout/1000;
    qbool_t conn_enable = FALSE;

    //httpc stop
    rlt = at_httpc_stop();
    if(rlt != QAPI_OK)
    {
        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCGETSIZE: client stop fail\r\n");
        QAT_Response_Str(QAT_RC_QUIET, buffer);
        goto endpiont;
    }

    //httpc start
    rlt = at_httpc_start();
    if(rlt != QAPI_OK)
    {
        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCGETSIZE: client start fail\r\n");
        QAT_Response_Str(QAT_RC_QUIET, buffer);
        goto endpiont;
    }

    //httpc new session
     rlt = at_httpc_new_session(url,timeout);
    if(rlt != QAPI_OK)
    {
        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCGETSIZE: session setup fail\r\n");
        QAT_Response_Str(QAT_RC_QUIET, buffer);
        goto endpiont;
    }
    
    //httpc conn
    rlt = at_httpc_conn(url);
    if(rlt != QAPI_OK)
    {
        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCGETSIZE: connection fail\r\n");
        QAT_Response_Str(QAT_RC_QUIET_NO_CR, buffer);
        goto endpiont;
    }
    conn_enable = TRUE;

    //Construct client request Command
    //httpc {get | head | post | put | delete | patch} <client_num> [<url>] [<chunk_flag>] [<chunk_size>]

    uint32_t  Parameter_Count =0;
    QAPI_Console_Parameter_t Parameter_List[QAT_HTTPC_MAXIMUM_NUMBER_OF_PARAMETERS];
    Parameter_List[0].Integer_Is_Valid =false;
    Parameter_List[0].String_Value = "head";
    Parameter_Count++;

    Parameter_List[Parameter_Count].Integer_Is_Valid = true;
    Parameter_List[Parameter_Count].Integer_Value = QAT_HTTPC_CLIENT_INDEX;
    Parameter_Count++;
    
    if(!getpathURL(url,path_url))
    {
        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCGETSIZE: get object url fail\r\n");
        QAT_Response_Str(QAT_RC_QUIET_NO_CR, buffer);
        rlt = QAPI_ERR_INVALID_PARAM;
        goto endpiont;
    }
    Parameter_List[Parameter_Count].Integer_Is_Valid =false;
    Parameter_List[Parameter_Count].String_Value = path_url;
    Parameter_Count++;
    
    //Parameter_List[Parameter_Count].Integer_Is_Valid =true;
    //Parameter_List[Parameter_Count].Integer_Value = QAT_HTTP_GETSIZE;
    //Parameter_Count++;
    at_httpc_method = QAT_HTTP_GETSIZE;

    rlt = httpc_command_handler(Parameter_Count,Parameter_List);
    if(rlt != QAPI_OK)
    {
        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCGETSIZE: handle client request fail\r\n");
        QAT_Response_Str(QAT_RC_QUIET, buffer);
        goto endpiont;
    }

endpiont:

   while(1)
   {
      if((at_rec_state <= QAPI_NET_HTTPC_RX_FINISHED) || (rlt != QAPI_OK))
      {
        if(at_rec_state == QAPI_NET_HTTPC_RX_FINISHED)
        {
            if(is_succ_resp_code(at_rec_error_code))
            {
                QAT_Response_Str(QAT_RC_OK, NULL);
            }
            else
            {
                QAT_Response_Str(QAT_RC_ERROR, NULL);
            }
            
        }
        else if(at_rec_state < QAPI_NET_HTTPC_RX_FINISHED)
        {
            snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCGETSIZE: HTTP Client Receive error: %d\r\n",at_rec_state);
            QAT_Response_Str(QAT_RC_ERROR, buffer);
        }
        else //rlt != QAPI_OK
        {
            QAT_Response_Str(QAT_RC_ERROR, NULL);
        }
        
        //httpc disconn
        if(conn_enable)
        {
            rlt = at_httpc_disconn(QAT_HTTPC_CLIENT_INDEX);
            if(rlt != QAPI_OK)
            {
                snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCGETSIZE: session disconn fail\r\n");
                QAT_Response_Str(QAT_RC_ERROR, buffer);
            }
        }
        
        //httpc stop
        rlt = at_httpc_stop();
        if(rlt != QAPI_OK)
        {
            snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCGETSIZE: client stop fail\r\n");
            QAT_Response_Str(QAT_RC_ERROR, buffer);
        }

         break;
      }
   
      sleep(1);
      count++;

      if(count >= max_time_wait)
      {
        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCGETSIZE: GET FAIL, timeout!\r\n");
        QAT_Response_Str(QAT_RC_ERROR, buffer);

        //httpc disconn
        if(conn_enable)
        {
            rlt = at_httpc_disconn(QAT_HTTPC_CLIENT_INDEX);
            if(rlt != QAPI_OK)
            {
                snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCGETSIZE: session disconn fail\r\n");
                QAT_Response_Str(QAT_RC_ERROR, buffer);
            }
        }
        
        //httpc stop
        rlt = at_httpc_stop();
        if(rlt != QAPI_OK)
        {
            snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCGETSIZE: client stop fail\r\n");
            QAT_Response_Str(QAT_RC_ERROR, buffer);
        }
        break;
      }
   }

   
   memset((void*)buffer, 0, HTTP_STR_BUFFER_LENGTH);
   return rlt;
}


qapi_Status_t at_httpc_get (char *url, int32_t timeout)
{
    qapi_Status_t rlt = QAPI_OK;
    char buffer[HTTP_STR_BUFFER_LENGTH] = {0};
    char path_url[HTTP_URL_STR_BUFFER_LENGTH] = {0};
    uint16 count = 0;
    uint32 max_time_wait = timeout/1000 < 1?1:timeout/1000;
    qbool_t conn_enable = FALSE;

    //httpc stop
    rlt = at_httpc_stop();
    if(rlt != QAPI_OK)
    {
        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCGET: client stop fail\r\n");
        QAT_Response_Str(QAT_RC_QUIET, buffer);
        goto endpiont;
    }

    //httpc start
    rlt = at_httpc_start();
    if(rlt != QAPI_OK)
    {
        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCGET: client start fail\r\n");
        QAT_Response_Str(QAT_RC_QUIET, buffer);
        goto endpiont;
    }

    //httpc new session
     rlt = at_httpc_new_session(url,timeout);
    if(rlt != QAPI_OK)
    {
        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCGET: session setup fail\r\n");
        QAT_Response_Str(QAT_RC_QUIET, buffer);
        goto endpiont;
    }
    
    //httpc conn
    rlt = at_httpc_conn(url);
    if(rlt != QAPI_OK)
    {
        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCGET: connection fail\r\n");
        QAT_Response_Str(QAT_RC_QUIET, buffer);
        goto endpiont;
    }
    conn_enable = TRUE;

    //Construct client request Command
    //httpc {get | head | post | put | delete | patch} <client_num> [<url>] [<chunk_flag>] [<chunk_size>]

    uint32_t  Parameter_Count =0;
    QAPI_Console_Parameter_t Parameter_List[QAT_HTTPC_MAXIMUM_NUMBER_OF_PARAMETERS];
    Parameter_List[0].Integer_Is_Valid =false;
    Parameter_List[0].String_Value = "get";
    Parameter_Count++;

    Parameter_List[Parameter_Count].Integer_Is_Valid = true;
    Parameter_List[Parameter_Count].Integer_Value = QAT_HTTPC_CLIENT_INDEX;
    Parameter_Count++;

    if(!getpathURL(url,path_url))
    {
        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCGET: get object url fail\r\n");
        QAT_Response_Str(QAT_RC_ERROR, buffer);
        rlt = QAPI_ERR_INVALID_PARAM;
        goto endpiont;
    }
    Parameter_List[Parameter_Count].Integer_Is_Valid =false;
    Parameter_List[Parameter_Count].String_Value = path_url;
    Parameter_Count++;

    //Parameter_List[Parameter_Count].Integer_Is_Valid =true;
    //Parameter_List[Parameter_Count].Integer_Value = QAT_HTTP_GET;
    //Parameter_Count++;
    at_httpc_method = QAT_HTTP_GET;

    rlt = httpc_command_handler(Parameter_Count,Parameter_List);
    if(rlt != QAPI_OK)
    {
        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCGET: handle client request fail\r\n");
        QAT_Response_Str(QAT_RC_QUIET, buffer);
        goto endpiont;
    }

endpiont:
    while(1)
    {
       if((at_rec_state <= QAPI_NET_HTTPC_RX_FINISHED) || (rlt != QAPI_OK))
       {
          if(at_rec_state == QAPI_NET_HTTPC_RX_FINISHED)
          {
                if(is_succ_resp_code(at_rec_error_code))
                {
                    QAT_Response_Str(QAT_RC_OK, NULL);
                }
                else
                {
                    QAT_Response_Str(QAT_RC_ERROR, NULL);
                }
                
           }
           else if(at_rec_state < QAPI_NET_HTTPC_RX_FINISHED)
           {
               snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCGET: HTTP Client Receive error: %d\r\n",at_rec_state);
               QAT_Response_Str(QAT_RC_ERROR, buffer);
           }
           else
           {
                QAT_Response_Str(QAT_RC_ERROR, NULL);
           }
           
           //httpc disconn
           if(conn_enable)
           {
             rlt = at_httpc_disconn(QAT_HTTPC_CLIENT_INDEX);
             if(rlt != QAPI_OK)
             {
               snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCGET: session disconn fail\r\n");
               QAT_Response_Str(QAT_RC_ERROR, buffer);
             }
           }
          
          //httpc stop
          rlt = at_httpc_stop();
          if(rlt != QAPI_OK)
          {
            snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCGET: client stop fail\r\n");
            QAT_Response_Str(QAT_RC_ERROR, buffer);
          }
          
          break;
       }

        sleep(1);
        count++;
        //printf("test count:%d, max:%d\n",count,max_time_wait);

        if(count>=max_time_wait)
        {
            snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCGET: GET FAIL, timeout\r\n");
            QAT_Response_Str(QAT_RC_ERROR, buffer);

            //httpc disconn
            if(conn_enable)
            {
                rlt = at_httpc_disconn(QAT_HTTPC_CLIENT_INDEX);
                if(rlt != QAPI_OK)
                {
                    snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCGET: session disconn fail\r\n");
                    QAT_Response_Str(QAT_RC_ERROR, buffer);
                }
            }
            
            //httpc stop
            rlt = at_httpc_stop();
            if(rlt != QAPI_OK)
            {
                snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCGET: client stop fail\r\n");
                QAT_Response_Str(QAT_RC_ERROR, buffer);
            }

            break;
        }
   
    }
   memset((void*)buffer, 0, HTTP_STR_BUFFER_LENGTH);
   return rlt;
}

qapi_Status_t at_httpc_put (char *url, int32_t data_len,char *data)
{
    qapi_Status_t rlt = QAPI_OK;
    char buffer[HTTP_STR_BUFFER_LENGTH]= {0};
    char path_url[HTTP_URL_STR_BUFFER_LENGTH]= {0};
    uint16 count = 0;
    qbool_t conn_enable = FALSE;

    //httpc stop
    rlt = at_httpc_stop();
    if(rlt != QAPI_OK)
    {
        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCPUT: client stop fail\r\n");
        QAT_Response_Str(QAT_RC_ERROR, buffer);
        goto endpiont;
    }

    //httpc start
    rlt = at_httpc_start();
    if(rlt != QAPI_OK)
    {
        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCPUT: client start fail\r\n");
        QAT_Response_Str(QAT_RC_ERROR, buffer);
        goto endpiont;
    }

    //httpc new session
     rlt = at_httpc_new_session(url,TIMEOUT_MS);
    if(rlt != QAPI_OK)
    {
        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCPUT: session setup fail\r\n");
        QAT_Response_Str(QAT_RC_ERROR, buffer);
        goto endpiont;
    }
    
    //httpc conn
    rlt = at_httpc_conn(url);
    if(rlt != QAPI_OK)
    {
        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCPUT: connection fail\r\n");
        QAT_Response_Str(QAT_RC_ERROR, buffer);
        goto endpiont;
    }
    conn_enable = TRUE;

    if(g_https_cfg.header_field_num > 0)
    {
       for(uint8_t i=0; i < g_https_cfg.header_field_num; i++)
       {
           rlt = at_httpc_addheaderfield(i);
           if(rlt != QAPI_OK)
           {
               snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCPUT: add headerfield fail, index:%d\r\n",i);
               QAT_Response_Str(QAT_RC_ERROR, buffer);
               goto endpiont;
   
           }
       }
        
    }

    //Construct client request Command
    //httpc {get | head | post | put | delete | patch} <client_num> [<url>] [<chunk_flag>] [<chunk_size>]
    //chunk_flag 0x00|0x01|0x80|0x81 = 0x00: non chunk encoded without http header;
    //                                 0x01: non chunk encoded with http header;
    //                                 0x80: chunk encoded without http header;
    //                                 0x81: chunk encoded with http header;
    uint32_t  Parameter_Count =0;
    QAPI_Console_Parameter_t Parameter_List[QAT_HTTPC_MAXIMUM_NUMBER_OF_PARAMETERS];
    Parameter_List[0].Integer_Is_Valid =false;
    Parameter_List[0].String_Value = "put";
    Parameter_Count++;

    
    Parameter_List[Parameter_Count].Integer_Is_Valid = true;
    Parameter_List[Parameter_Count].Integer_Value = QAT_HTTPC_CLIENT_INDEX;
    Parameter_Count++;

    if(!getpathURL(url,path_url))
    {
        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCPUT: get object url fail\r\n");
        QAT_Response_Str(QAT_RC_ERROR, buffer);
        rlt = QAPI_ERR_INVALID_PARAM;
        goto endpiont;
    }
    Parameter_List[Parameter_Count].Integer_Is_Valid =false;
    Parameter_List[Parameter_Count].String_Value = path_url;
    Parameter_Count++;

    at_httpc_method = QAT_HTTP_PUT;


    //send data
    if(data_len <= QAT_MAX_CHUNK_SIZE)
    {
        rlt = at_httpc_setbodydata(data, data_len);
        
        if(rlt == QAPI_OK)
        {
            rlt = httpc_command_handler(Parameter_Count,Parameter_List);
            if(rlt != QAPI_OK)
            {
                snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCPUT: handle client request fail\r\n");
                QAT_Response_Str(QAT_RC_ERROR, buffer);
                goto endpiont;
            }
        }
    }
    else  //if data len > QAT_MAX_CHUNK_SIZE, need use chunk
    {

        char * chunkdata = malloc(QAT_MAX_CHUNK_SIZE+1);
        if(chunkdata == NULL)
        {
            snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCPUT: malloc memory fail\r\n");
            QAT_Response_Str(QAT_RC_ERROR, buffer);
            rlt == QAPI_ERR_NO_MEMORY;
            goto endpiont;
        }

        uint16_t sendnum = (data_len+QAT_MAX_CHUNK_SIZE-1)/QAT_MAX_CHUNK_SIZE;
        char * senddata = data;
        uint16_t sendatalen = 0;
        uint16_t chunkflag = 0;
        Parameter_Count +=3;
        for(uint16_t count = 0; count < sendnum; count++)
        {
            if(count == sendnum-1)
            {
                sendatalen = data_len -QAT_MAX_CHUNK_SIZE*count;
                memcpy(chunkdata, senddata, sendatalen);
                chunkdata[sendatalen] = '\0';
            }
            else
            {
                sendatalen = QAT_MAX_CHUNK_SIZE;
                memcpy(chunkdata, senddata, sendatalen);
                chunkdata[sendatalen] = '\0';
            }
            

            rlt = at_httpc_setbodydata(chunkdata, sendatalen);

            if(rlt == QAPI_OK)
            {
                if(count == 0){
                    chunkflag = 129;
                }
                else{
                    chunkflag = 128;
                }
                
                Parameter_List[Parameter_Count-3].Integer_Is_Valid =true;
                Parameter_List[Parameter_Count-3].Integer_Value = chunkflag;

                Parameter_List[Parameter_Count-2].Integer_Is_Valid =true;
                Parameter_List[Parameter_Count-2].Integer_Value = sendatalen;
                
                Parameter_List[Parameter_Count-1].Integer_Is_Valid =true;
                Parameter_List[Parameter_Count-1].Integer_Value = data_len;
                
                rlt = httpc_command_handler(Parameter_Count,Parameter_List);
                if(rlt != QAPI_OK)
                {
                    snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCPUT: handle client request fail\r\n");
                    QAT_Response_Str(QAT_RC_ERROR, buffer);
                    goto endpiont;
                }
            }

            senddata += sendatalen;
            sys_msleep(QAT_CHUNK_INTERVAL);
            if(at_rec_state <= QAPI_NET_HTTPC_RX_FINISHED)
            {
                if(chunkdata){
                  free(chunkdata);
                 }
                goto endpiont;
            }
        }

        //send the chunk end flag
        Parameter_List[Parameter_Count-3].Integer_Is_Valid =true;
        Parameter_List[Parameter_Count-3].Integer_Value = 128;
        
        Parameter_List[Parameter_Count-2].Integer_Is_Valid =true;
        //no data
        Parameter_List[Parameter_Count-2].Integer_Value = 0;

        Parameter_List[Parameter_Count-1].Integer_Is_Valid =true;
        Parameter_List[Parameter_Count-1].Integer_Value = data_len;
        
        rlt = httpc_command_handler(Parameter_Count,Parameter_List);
        if(rlt != QAPI_OK)
        {
            snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCPUT: handle client request fail\r\n");
            QAT_Response_Str(QAT_RC_ERROR, buffer);
        }
        
        if(chunkdata){
          free(chunkdata);
        }
    }

endpiont:

    while(1)
    {
       if((at_rec_state <= QAPI_NET_HTTPC_RX_FINISHED) || (rlt != QAPI_OK))
       {
           if(at_rec_state == QAPI_NET_HTTPC_RX_FINISHED)
           {
                snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCPUT: SEND OK, Resp_code: %d\r\n",at_rec_error_code);
                QAT_Response_Str(QAT_RC_OK, buffer);
           }
           else if(at_rec_state < QAPI_NET_HTTPC_RX_FINISHED)
           {
                snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCPUT: SEND FAIL, error_code:%d\r\n",at_rec_state);
                QAT_Response_Str(QAT_RC_ERROR, buffer);
           }
           else
           {
                snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCPUT: SEND FAIL, error_code:%d\r\n",rlt);
                QAT_Response_Str(QAT_RC_ERROR, buffer);
           }
           
           //httpc disconn
           if(conn_enable)
           {
              rlt = at_httpc_disconn(QAT_HTTPC_CLIENT_INDEX);
              if(rlt != QAPI_OK)
              {
                snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCPUT: session disconn fail\r\n");
                QAT_Response_Str(QAT_RC_ERROR, buffer);
              }
           }
           
           //httpc stop
           rlt = at_httpc_stop();
           if(rlt != QAPI_OK)
           {
             snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCPUT: client stop fail\r\n");
             QAT_Response_Str(QAT_RC_ERROR, buffer);
           }
          
          break;
       }
       
       sleep(1);
       count++;

      if(count >= HTTP_WAIT_RSP_TIME)
      {
        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCPUT: SEND FAIL, timeout\r\n");
        QAT_Response_Str(QAT_RC_ERROR, buffer);

        //httpc disconn
        if(conn_enable)
        {
            rlt = at_httpc_disconn(QAT_HTTPC_CLIENT_INDEX);
            if(rlt != QAPI_OK)
            {
             snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCPUT: session disconn fail\r\n");
             QAT_Response_Str(QAT_RC_ERROR, buffer);
            }
        }

        //httpc stop
        rlt = at_httpc_stop();
        if(rlt != QAPI_OK)
        {
         snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCPUT: client stop fail\r\n");
         QAT_Response_Str(QAT_RC_ERROR, buffer);
        }
        
        break;
      }
    }

   memset((void*)buffer, 0, HTTP_STR_BUFFER_LENGTH);
   return rlt;
}

qapi_Status_t at_httpc_post (char *url, int32_t data_len,char *data)
{
    qapi_Status_t rlt = QAPI_OK;
    char buffer[HTTP_STR_BUFFER_LENGTH]= {0};
    char path_url[HTTP_URL_STR_BUFFER_LENGTH]= {0};
    uint16 count = 0;
    qbool_t conn_enable = FALSE;

    //httpc stop
    rlt = at_httpc_stop();
    if(rlt != QAPI_OK)
    {
        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCPOST: client stop fail\r\n");
        QAT_Response_Str(QAT_RC_ERROR, buffer);
        goto endpiont;
    }

    //httpc start
    rlt = at_httpc_start();
    if(rlt != QAPI_OK)
    {
        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCPOST: client start fail\r\n");
        QAT_Response_Str(QAT_RC_ERROR, buffer);
        goto endpiont;
    }

    //httpc new session
     rlt = at_httpc_new_session(url,TIMEOUT_MS);
    if(rlt != QAPI_OK)
    {
        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCPOST: session setup fail\r\n");
        QAT_Response_Str(QAT_RC_ERROR, buffer);
        goto endpiont;
    }

    //httpc conn
    rlt = at_httpc_conn(url);
    if(rlt != QAPI_OK)
    {
        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCPOST: connection fail\r\n");
        QAT_Response_Str(QAT_RC_ERROR, buffer);
        goto endpiont;
    }
    conn_enable = TRUE;

    if(g_https_cfg.header_field_num > 0)
    {
       for(uint8_t i=0; i < g_https_cfg.header_field_num; i++)
       {
           rlt = at_httpc_addheaderfield(i);
           if(rlt != QAPI_OK)
           {
               snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCPOST: add headerfield fail, index:%d\r\n",i);
               QAT_Response_Str(QAT_RC_ERROR, buffer);
               goto endpiont;
   
           }
       }
        
    }

    //Construct client request Command
    //httpc {get | head | post | put | delete | patch} <client_num> [<url>] [<chunk_flag>] [<chunk_size>]
    //chunk_flag 0x00|0x01|0x80|0x81 = 0x00: non chunk encoded without http header;
    //                                 0x01: non chunk encoded with http header;
    //                                 0x80: chunk encoded without http header;
    //                                 0x81: chunk encoded with http header;
    uint32_t  Parameter_Count =0;
    QAPI_Console_Parameter_t Parameter_List[QAT_HTTPC_MAXIMUM_NUMBER_OF_PARAMETERS];
    Parameter_List[0].Integer_Is_Valid =false;
    Parameter_List[0].String_Value = "post";
    Parameter_Count++;

    
    Parameter_List[Parameter_Count].Integer_Is_Valid = true;
    Parameter_List[Parameter_Count].Integer_Value = QAT_HTTPC_CLIENT_INDEX;
    Parameter_Count++;

    if(!getpathURL(url,path_url))
    {
        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCPOST: get object url fail\r\n");
        QAT_Response_Str(QAT_RC_ERROR, buffer);
        rlt = QAPI_ERR_INVALID_PARAM;
        goto endpiont;
    }
    Parameter_List[Parameter_Count].Integer_Is_Valid =false;
    Parameter_List[Parameter_Count].String_Value = path_url;
    Parameter_Count++;

    at_httpc_method = QAT_HTTP_POST;


    
    if(data_len <= QAT_MAX_CHUNK_SIZE)
    {
        rlt = at_httpc_setbodydata(data, data_len);
        
        if(rlt == QAPI_OK)
        {
            rlt = httpc_command_handler(Parameter_Count,Parameter_List);
            if(rlt != QAPI_OK)
            {
                snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCPOST: handle client request fail\r\n");
                QAT_Response_Str(QAT_RC_ERROR, buffer);
                goto endpiont;
            }
        }
    }
    else  //if data len > QAT_MAX_CHUNK_SIZE, need use chunk
    {

        char * chunkdata = malloc(QAT_MAX_CHUNK_SIZE+1);
        if(chunkdata == NULL)
        {
            snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCPOST: malloc memory fail\r\n");
            QAT_Response_Str(QAT_RC_ERROR, buffer);
            rlt == QAPI_ERR_NO_MEMORY;
            goto endpiont;
        }

        uint16_t sendnum = (data_len+QAT_MAX_CHUNK_SIZE-1)/QAT_MAX_CHUNK_SIZE;
        char * senddata = data;
        uint16_t sendatalen = 0;
        uint16_t chunkflag = 0;
        Parameter_Count +=3;
        for(uint16_t count = 0; count < sendnum; count++)
        {
            if(count == sendnum-1)
            {
                sendatalen = data_len -QAT_MAX_CHUNK_SIZE*count;
                memcpy(chunkdata, senddata, sendatalen);
                chunkdata[sendatalen] = '\0';
            }
            else
            {
                sendatalen = QAT_MAX_CHUNK_SIZE;
                memcpy(chunkdata, senddata, sendatalen);
                chunkdata[QAT_MAX_CHUNK_SIZE] = '\0';
            }
            

            rlt = at_httpc_setbodydata(chunkdata, sendatalen);
            //printf("post setbody\r\n");
            if(rlt == QAPI_OK)
            {
                if(count == 0){
                    chunkflag = 129;
                }
                else{
                    chunkflag = 128;
                }
                Parameter_List[Parameter_Count-3].Integer_Is_Valid =true;
                Parameter_List[Parameter_Count-3].Integer_Value = chunkflag;
  
                Parameter_List[Parameter_Count-2].Integer_Is_Valid =true;
                Parameter_List[Parameter_Count-2].Integer_Value = sendatalen;
                
                Parameter_List[Parameter_Count-1].Integer_Is_Valid =true;
                Parameter_List[Parameter_Count-1].Integer_Value = data_len;
                
                rlt = httpc_command_handler(Parameter_Count,Parameter_List);
                if(rlt != QAPI_OK)
                {
                    snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCPOST: handle client request fail\r\n");
                    QAT_Response_Str(QAT_RC_ERROR, buffer);
                    goto endpiont;
                }
            }

            senddata += sendatalen;
            sys_msleep(QAT_CHUNK_INTERVAL);
            if(at_rec_state <= QAPI_NET_HTTPC_RX_FINISHED)
            {
                if(chunkdata){
                  free(chunkdata);
                 }
                goto endpiont;
            }
        }

        //send the chunk end flag
        Parameter_List[Parameter_Count-3].Integer_Is_Valid =true;
        Parameter_List[Parameter_Count-3].Integer_Value = 128;
        
        Parameter_List[Parameter_Count-2].Integer_Is_Valid =true;
        //no data
        Parameter_List[Parameter_Count-2].Integer_Value = 0;

        Parameter_List[Parameter_Count-1].Integer_Is_Valid =true;
        Parameter_List[Parameter_Count-1].Integer_Value = data_len;

        
        rlt = httpc_command_handler(Parameter_Count,Parameter_List);
        if(rlt != QAPI_OK)
        {
            snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCPOST: handle client request fail\r\n");
            QAT_Response_Str(QAT_RC_ERROR, buffer);
        }
        
        if(chunkdata){
            free(chunkdata);
        }
    }
    

endpiont:

   while(1)
   {
      if((at_rec_state <= QAPI_NET_HTTPC_RX_FINISHED) || (rlt != QAPI_OK))
      {
           if((at_rec_state == QAPI_NET_HTTPC_RX_FINISHED))
           {
               snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCPOST: SEND OK, Resp_code: %d\r\n",at_rec_error_code);
               QAT_Response_Str(QAT_RC_OK, buffer);
           }
           else if(at_rec_state < QAPI_NET_HTTPC_RX_FINISHED)
           {
                snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCPOST: SEND FAIL, error_code:%d\r\n",at_rec_state);
                QAT_Response_Str(QAT_RC_ERROR, buffer);
           }
           else
           {
               snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCPOST: SEND FAIL, error_code:%d\r\n",rlt);
               QAT_Response_Str(QAT_RC_ERROR, buffer);
           }


           //httpc disconn
           if(conn_enable)
           {
              rlt = at_httpc_disconn(QAT_HTTPC_CLIENT_INDEX);
              if(rlt != QAPI_OK)
              {
                snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCPOST: session disconn fail\r\n");
                QAT_Response_Str(QAT_RC_ERROR, buffer);
              }
           }
           
           //httpc stop
           rlt = at_httpc_stop();
           if(rlt != QAPI_OK)
           {
             snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCPOST: client stop fail\r\n");
             QAT_Response_Str(QAT_RC_ERROR, buffer);
           }

         break;
      }

      sleep(1);
      count++;

      if(count>HTTP_WAIT_RSP_TIME)
      {
        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCPOST: SEND FAIL, timeout\r\n");
        QAT_Response_Str(QAT_RC_ERROR, buffer);

        //httpc disconn
        if(conn_enable)
        {
            rlt = at_httpc_disconn(QAT_HTTPC_CLIENT_INDEX);
            if(rlt != QAPI_OK)
            {
             snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCPOST: session disconn fail\r\n");
             QAT_Response_Str(QAT_RC_ERROR, buffer);
            }
        }
        
        //httpc stop
        rlt = at_httpc_stop();
        if(rlt != QAPI_OK)
        {
         snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+HTTPCPOST: client stop fail\r\n");
         QAT_Response_Str(QAT_RC_ERROR, buffer);
        }
        
        break;
      }
      
   }

   memset((void*)buffer, 0, HTTP_STR_BUFFER_LENGTH);
   return rlt;
}

uint32_t splitKeyValuePairs(char *input, QAPI_Console_Parameter_t* Parameter_List)
{
   uint32_t count=0;
   char *pair;
   char *key;
   char *value;
   
   // First split the string by '&'
   while ((pair=strsep(&input, "&")) != NULL) 
   {
       // For each part, split it by '='
       value = pair;
       key = strsep(&value, "=");

        if (value != NULL)
        {
          printf("Key: %s, Value: %s\n", key, value);
          Parameter_List[count].String_Value = key;
          count++;
          Parameter_List[count].String_Value = value;
          count++;
          if(count > QAT_HTTPC_MAXIMUM_NUMBER_OF_KEY_VALUE)
          {
              break;
          }
        }
    }

   return count/2;
}

void parseURL(const char *url, char *protocol, char *domain, char *path)
{ 

   const char *protocol_end = strstr(url, "://"); 
   if (protocol_end != NULL) 
   { 
        snprintf(protocol, protocol_end - url + 1, url);
    	url = protocol_end + 3;
	} 
	else 
	{ 
       snprintf(protocol, HTTP_URL_STR_BUFFER_LENGTH, "");
	} 

	const char *path_start = strchr(url, '/');
	if (path_start != NULL) 
	{ 
	  snprintf(domain, path_start - url + 1, url);

	  snprintf(path, HTTP_URL_STR_BUFFER_LENGTH, path_start);
	 }
	else 
	{ 

	  snprintf(domain, HTTP_URL_STR_BUFFER_LENGTH, url);
	  snprintf(path, HTTP_URL_STR_BUFFER_LENGTH, "");
    } 
} 

//get host like http://www.baidu.com
void gethostURL(const char *url, char*hostURL)
{
    const char *orignalurl = url;
    const char *protocol_end = strstr(url, "://"); 
    if (protocol_end != NULL) 
    { 
    	url = protocol_end + 3;
        
        const char *path_start = strchr(url, '/');
    	if (path_start != NULL) 
    	{ 
		  snprintf(hostURL, path_start - orignalurl + 1, orignalurl);
    	 }
    	else 
    	{ 
		  snprintf(hostURL, HTTP_URL_STR_BUFFER_LENGTH, orignalurl);
        } 

    } 
    else 
    { 
        const char *path_start = strchr(url, '/');
    	if (path_start != NULL) 
    	{ 
		  snprintf(hostURL, path_start - orignalurl + 1, orignalurl);
    	 }
    	else 
    	{ 
		  snprintf(hostURL, HTTP_URL_STR_BUFFER_LENGTH, orignalurl);
        } 
    }
    printf("host: %s\n", hostURL);

}

qbool_t getpathURL(const char *url, char*pathURL)
{
    qbool_t rlt=FALSE;
    const char *orignalurl = url;

    const char *protocol_end = strstr(url, "://"); 
    if (protocol_end != NULL) 
    { 
    	url = protocol_end + 3;
        
        const char *path_start = strchr(url, '/');
    	if (path_start != NULL)
    	{ 
            snprintf(pathURL, HTTP_URL_STR_BUFFER_LENGTH, path_start);
            rlt=TRUE;
    	}
    	else 
    	{ 
    	    pathURL = "/";
            rlt=TRUE;
        } 

    } 
    else 
    { 
        const char *path_start = strchr(url, '/');
    	//if ((path_start != NULL) && (strlen(path_start) > 1) )
    	if (path_start != NULL)
    	{ 
		  snprintf(pathURL, HTTP_URL_STR_BUFFER_LENGTH, path_start);
          rlt=TRUE;
    	}
    	else 
    	{ 
    	   pathURL = "/";
           rlt=TRUE;
        } 
    }
    printf("object URL: %s\n", pathURL);
    return rlt;
}

qbool_t saveUrl(const char *url)
{
   qbool_t rlt = FALSE;

    if (g_https_cfg.temp_url!= NULL) 
    {
         free(g_https_cfg.temp_url);
         g_https_cfg.temp_url = NULL;
    }
    
    uint32 len = strlen(url);

    if(len>0)
    {
        g_https_cfg.temp_url = malloc(len +1);
        
        if (!g_https_cfg.temp_url) {
            printf("malloc fail \r\n");
            return rlt;
        }
    
        memset(g_https_cfg.temp_url, 0, len+1);
        memcpy(g_https_cfg.temp_url, url, len);
        g_https_cfg.temp_url[len] = '\0';

        rlt= TRUE;
    }
    else  //user may can use "" as url,that means the configured url will be used
    {
        rlt= TRUE;
    } 

    return rlt;
}

qbool_t saveheaderfield(const char *headerfield)
{
   qbool_t rlt = FALSE;
   char buffer[HTTP_STR_BUFFER_LENGTH];

   if(g_https_cfg.header_field_num < QAT_HTTPC_MAX_HEADER_FIELD)
   {
       struct at_header_field* header_field = &g_https_cfg.header_field[g_https_cfg.header_field_num];
       if(header_field->name != NULL)
       {
           free(header_field->name);
           header_field->name = NULL;
       }
       if(header_field->value != NULL)
       {
           free(header_field->value);
           header_field->value = NULL;
       }
       
       const char *name_end = strstr(headerfield, ":"); 
       if (name_end != NULL) 
       { 
           uint32 name_len = name_end - headerfield;
           if(name_len > 0)
           {
                header_field->name = malloc(name_len +1);
                if (!header_field->name) {
                    snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+httpc:memory malloc fail\r\n");
                    QAT_Response_Str(QAT_RC_ERROR, buffer);
                    return rlt;
                }
            
                memset(header_field->name, 0, name_len+1);
                snprintf(header_field->name, name_len + 1, headerfield);
                
                uint32 len = strlen(headerfield);

                header_field->value = malloc(len -name_len + 1);
                memcpy(header_field->value, name_end+1, len -name_len);
                header_field->value[len -name_len] = '\0';
                
                g_https_cfg.header_field_num ++;

                printf("test num:%d, name:%s,value:%s \n",g_https_cfg.header_field_num,header_field->name,header_field->value);

                rlt= TRUE;
           }
    	} 

       
   }
   else
   {
        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+httpc:the number of header_field is bigger than %d\r\n",QAT_HTTPC_MAX_HEADER_FIELD);
        QAT_Response_Str(QAT_RC_ERROR, buffer);
   }

   return rlt;
}

qbool_t save_content_type(uint8_t content_type)
{
   qbool_t rlt = FALSE;
   char buffer[HTTP_STR_BUFFER_LENGTH];
   char* name = "Content-Type";

   if(g_https_cfg.header_field_num < QAT_HTTPC_MAX_HEADER_FIELD)
   {
       struct at_header_field* header_field = &g_https_cfg.header_field[g_https_cfg.header_field_num];
       if(header_field->name != NULL)
       {
           free(header_field->name);
           header_field->name = NULL;
       }
       if(header_field->value != NULL)
       {
           free(header_field->value);
           header_field->value = NULL;
       }

       int name_len = strlen(name);
       header_field->name = malloc(name_len +1);
       if (!header_field->name) {
            snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+httpc:memory malloc fail\r\n");
            QAT_Response_Str(QAT_RC_ERROR, buffer);
            return rlt;
       }
    
       memset(header_field->name, 0, name_len+1);
       snprintf(header_field->name, name_len + 1, name);
        
       switch(content_type)
       {
            case QAT_CONTENT_TYPE_X_WWW_FORM_URLENCODED:
            {
               //it's default type, like: Content-Type: application/x-www-form-urlencoded
               char* value = "application/x-www-form-urlencoded";
               uint32 val_len = strlen(value);
               
               header_field->value = malloc(val_len + 1);
               if (!header_field->name) {
                    snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+httpc:memory malloc fail\r\n");
                    QAT_Response_Str(QAT_RC_ERROR, buffer);
                    return rlt;
               }
               
               memset(header_field->value, 0, val_len+1);
               snprintf(header_field->value, val_len + 1, value);
               rlt = TRUE;
               
               break;
            }
            case QAT_CONTENT_TYPE_JSON:
            {
               char* value = "application/json";
               uint32 val_len = strlen(value);
               
               header_field->value = malloc(val_len + 1);
               if (!header_field->name) {
                    snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+httpc:memory malloc fail\r\n");
                    QAT_Response_Str(QAT_RC_ERROR, buffer);
                    return rlt;
               }
               
               memset(header_field->value, 0, val_len+1);
               snprintf(header_field->value, val_len + 1, value);
               rlt = TRUE;
               
               break;
            }
            case QAT_CONTENT_TYPE_ZIP:
            {
               char* value = "application/zip";
               uint32 val_len = strlen(value);
               
               header_field->value = malloc(val_len + 1);
               if (!header_field->name) {
                    snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+httpc:memory malloc fail\r\n");
                    QAT_Response_Str(QAT_RC_ERROR, buffer);
                    return rlt;
               }
               
               memset(header_field->value, 0, val_len+1);
               snprintf(header_field->value, val_len + 1, value);
               rlt = TRUE;
               
               break;
            }
            case QAT_CONTENT_TYPE_FORM_DATA:
            {

               char* value = "multipart/form-data";
               uint32 val_len = strlen(value);
               
               header_field->value = malloc(val_len + 1);
               if (!header_field->name) {
                    snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+httpc:memory malloc fail\r\n");
                    QAT_Response_Str(QAT_RC_ERROR, buffer);
                    return rlt;
               }
               
               memset(header_field->value, 0, val_len+1);
               snprintf(header_field->value, val_len + 1, value);
               rlt = TRUE;
               
               break;
            }
            case QAT_CONTENT_TYPE_TEXT_XML:
            {
               char* value = "text/xml";
               uint32 val_len = strlen(value);
               
               header_field->value = malloc(val_len + 1);
               if (!header_field->name) {
                    snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+httpc:memory malloc fail\r\n");
                    QAT_Response_Str(QAT_RC_ERROR, buffer);
                    return rlt;
               }
               
               memset(header_field->value, 0, val_len+1);
               snprintf(header_field->value, val_len + 1, value);
               rlt = TRUE;

               break;
            }

            default:
              return rlt
             ;
       }

       g_https_cfg.header_field_num ++;
   }
   else
   {
        snprintf(buffer, HTTP_STR_BUFFER_LENGTH, "+httpc:the number of header_field is bigger than %d\r\n",QAT_HTTPC_MAX_HEADER_FIELD);
        QAT_Response_Str(QAT_RC_ERROR, buffer);
   }

   return rlt;
}


void resetSslInfo()
{
    g_https_cfg.https_auth_type = 0;
    memset(g_https_cfg.cert_file, 0, FILE_PATH_STR_BUFFER_LENGTH);
    memset(g_https_cfg.key_file, 0, FILE_PATH_STR_BUFFER_LENGTH);
    memset(g_https_cfg.ca_file, 0, FILE_PATH_STR_BUFFER_LENGTH);
}

qbool_t isSecureSession(const char *url)
{
   qbool_t rlt = FALSE;

    if(strncmp(url, "https://", 8) == 0)
    {
        rlt = TRUE;
    }

    else if(strncmp(url, "http://", 7) == 0)
    {
        rlt = FALSE;
    }

    return rlt;
}

qbool_t create_send_buffer(int length)
{
    qbool_t rlt = FALSE;

    if (g_https_cfg.send_buff != NULL) {
         free(g_https_cfg.send_buff);
         g_https_cfg.send_buff = NULL;
    }

    //more 1 bit for '\0'
    g_https_cfg.send_buff = malloc(length+1);  
    if (!g_https_cfg.send_buff) {
       return rlt;
    }

    memset(g_https_cfg.send_buff, 0, length+1);
    rlt = TRUE;
    
    return rlt;
}

void savedata(const char *data)
{ 
   //the length of send_buff is  g_https_cfg.data_len +1, more one bit for '\0'
   g_https_cfg.buff_offset += 
    snprintf((char*)(g_https_cfg.send_buff + g_https_cfg.buff_offset), g_https_cfg.data_len - g_https_cfg.buff_offset +1,
            "%s", data);
}

void reset_temp_resource()
{
    //g_https_cfg.content_type = QAT_CONTENT_TYPE_X_WWW_FORM_URLENCODED;
    for(uint8 i =0; i<g_https_cfg.header_field_num; i++ )
    {
       struct at_header_field* header_field = &g_https_cfg.header_field[i];
       if(header_field->name != NULL)
       {
           free(header_field->name);
           header_field->name = NULL;
       }
       if(header_field->value != NULL)
       {
           free(header_field->value);
           header_field->value = NULL;
       }
    }
    
    g_https_cfg.header_field_num = 0;
    g_https_cfg.buff_offset =0;
    
    if (g_https_cfg.send_buff != NULL) {
         free(g_https_cfg.send_buff);
         g_https_cfg.send_buff = NULL;
    }

    if (g_https_cfg.temp_url != NULL) {
         free(g_https_cfg.temp_url);
         g_https_cfg.temp_url = NULL;
    }
}

qbool_t is_succ_resp_code(int errorcode)
{
    if (errorcode >= 200 && errorcode < 300) 
    {
      return TRUE;
    }
    return FALSE;
}

int is_valid_char(char c) {
    return isalnum(c) || c == '.' || c == '-' || c == '/';
}

int validate_url(const char *url) 
{
    const char *http_start = "http://";
    size_t http_start_len = strlen(http_start);

    const char *https_start = "https://";
    size_t https_start_len = strlen(https_start);

    const char *host_start = NULL;

    // check the start chars
    if(strncmp(url, https_start, https_start_len)== 0)
    {
        host_start = url + https_start_len;
    }
    else if (strncmp(url, http_start, http_start_len)== 0)
    {
        host_start = url + http_start_len;
    }
    else
    {
        return 0;
    }

        

    // check host
    const char *host_end = strchr(host_start, '/');
    if (!host_end) {
        host_end = url + strlen(url);
    }

    // check the chars of host
    for (const char *p = host_start; p < host_end; ++p) {
        if (!is_valid_char(*p)) {
            return 0;
        }
    }

    // check the chars of path
    for (const char *p = host_end; *p != '\0'; ++p) {
        if (!is_valid_char(*p)) {
            return 0;
        }
    }

    return 1;
}


#if 0
void reset_resource()
{
    resetSslInfo();

    g_https_cfg.http_port = HTTP_DEFAULT_PORT;
    g_https_cfg.http_port_set = FALSE;
    g_https_cfg.https_port = HTTPS_DEFAULT_PORT;
    g_https_cfg.https_port_set = FALSE;
    
    for(uint8 i =0; i<g_https_cfg.header_field_num; i++ )
    {
       struct at_header_field* header_field = &g_https_cfg.header_field[i];
       if(header_field->name != NULL)
       {
           free(header_field->name);
           header_field->name = NULL;
       }
       if(header_field->value != NULL)
       {
           free(header_field->value);
           header_field->value = NULL;
       }
    }
    
    g_https_cfg.header_field_num = 0;
    g_https_cfg.buff_offset =0;
    
    if (g_https_cfg.send_buff != NULL) {
         free(g_https_cfg.send_buff);
         g_https_cfg.send_buff = NULL;
    }

    if (g_https_cfg.url != NULL) {
         free(g_https_cfg.url);
         g_https_cfg.url = NULL;
    }

    if (g_https_cfg.temp_url != NULL) {
         free(g_https_cfg.temp_url);
         g_https_cfg.temp_url = NULL;
    }
}
#endif
static int at_arg_is_string(const char *arg)
{
    int len = strlen(arg);

    if (arg[0] != '\"' || arg[len - 1] != '\"') {
        return 0;
    }

    return 1;
}

static int at_arg_is_null(const char *arg)
{
    if (strlen(arg) <= 0)
        return 1;
    else
        return 0;
}

int at_arg_get_number(const char *arg, int *value)
{
    int i;

    for (i=0; i<strlen(arg); i++) {
        if (!((arg[i] >= '0' && arg[i] <= '9') || (i == 0 && arg[i] == '-')))
            return 0;
    }

    *value = atoi(arg);
    return 1;
}

int at_arg_get_hexstr_number(const char *arg, uint32_t *value)
{
    char *endptr;

    if (arg == NULL || value == NULL) {
        return 0;
    }

    *value = strtoul(arg, &endptr, 16);

    // Invalid characters after the number
    if (endptr == arg || *endptr != '\0') {
        return 0;
    }

    return 1;
}

int at_arg_get_string(const char *arg, char *string, int max)
{
    int len;

    if (!at_arg_is_string(arg))
        return 0;

    len = strlen(arg)-2;
    if (len >= max)
        return 0;

    strlcpy(string, arg+1, max);
    string[len] = '\0';
    return 1;
}


