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
#include "fcntl.h"
#include "nt_osal.h"
#include "qurt_mutex.h"
#include "wifi_fw_version.h"
#include "wifi_fw_pmu_ts_cfg.h"
#include "mqtt_client_demo.h"

#include "qapi_status.h"
#include "qapi_console.h"

#include "qcli_api.h"

/* MQTT API headers. */
#include "core_mqtt.h"
#include "core_mqtt_state.h"

/*Include backoff algorithm header for retry logic.*/
#include "backoff_algorithm.h"

/*-------------------------------------------------------------------------
 * Function Declarations
 *-----------------------------------------------------------------------*/
static QAT_Command_Status_t Extend_Command_MqttLongClientID(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_MqttLongUserName(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_MqttLongPassword(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_MqttInit(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_MqttAlpn(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_MqttSni(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_MqttConn(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_MqttPub(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_MqttPubRaw(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_MqttSub(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_MqttUnSub(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_MqttDisconnect(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_MqttDestroy(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);


/* The following is the complete command list for the QAT common command demo. */
/** List of global commands that are supported when in a group. */
static QAT_Command_t QAT_MQTT_Command_List[] =
{
	{"+MQTTINIT",   Extend_Command_MqttInit,      QAT_OP_EXEC|QAT_OP_EXEC_W_PARAM},
	// {"+MQTTLONGCLIENTID",   Extend_Command_MqttLongClientID,   QAT_OP_EXEC|QAT_OP_EXEC_W_PARAM},
   // {"+MQTTLONGUSERNAME",   Extend_Command_MqttLongUserName,   QAT_OP_EXEC|QAT_OP_EXEC_W_PARAM},
   // {"+MQTTLONGPASSWORD",   Extend_Command_MqttLongPassword,   QAT_OP_EXEC|QAT_OP_EXEC_W_PARAM},
   // {"+MQTTALPN",   Extend_Command_MqttAlpn,   QAT_OP_EXEC|QAT_OP_EXEC_W_PARAM},
   // {"+MQTTSNI",   Extend_Command_MqttSni,   QAT_OP_EXEC|QAT_OP_EXEC_W_PARAM},
   {"+MQTTCONN",   Extend_Command_MqttConn,   QAT_OP_EXEC|QAT_OP_QUERY|QAT_OP_EXEC_W_PARAM},
   {"+MQTTPUB",   Extend_Command_MqttPub,   QAT_OP_EXEC|QAT_OP_EXEC_W_PARAM},
   {"+MQTTPUBRAW",   Extend_Command_MqttPubRaw,   QAT_OP_EXEC_IN_DATA_MODEL},
   {"+MQTTSUB",   Extend_Command_MqttSub,   QAT_OP_EXEC|QAT_OP_QUERY|QAT_OP_EXEC_W_PARAM},
   {"+MQTTUNSUB",   Extend_Command_MqttUnSub,   QAT_OP_EXEC|QAT_OP_EXEC_W_PARAM},
   {"+MQTTDISCONN",   Extend_Command_MqttDisconnect,   QAT_OP_EXEC|QAT_OP_EXEC_W_PARAM},
   {"+MQTTDESTROY",   Extend_Command_MqttDestroy,   QAT_OP_EXEC|QAT_OP_EXEC_W_PARAM},
};

/*-------------------------------------------------------------------------
 * External parameters
 *-----------------------------------------------------------------------*/
 extern HTC_Context_t HTC_Context;

/*-------------------------------------------------------------------------
 * Parameters define
 *-----------------------------------------------------------------------*/

#define MQTT_COMMAND_LIST_SIZE                      (sizeof(QAT_MQTT_Command_List) / sizeof(QAT_Command_t))

#define VERSION_STR_BUFFER_LENGTH 					  256
#define INFO_STR_BUFFER_LENGTH					      256
#define WRTMEM_STR_BUFFER_LENGTH					  128
#define CMD_STR_BUFFER_LENGTH					      1024

/*-------------------------------------------------------------------------
 * Preprocessor Definitions, Constants, and Type Declarations
 *-----------------------------------------------------------------------*/

#define MQTT_CLIENT_PRINTF(...)     printf(__VA_ARGS__)

/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/

// qapi_Status_t at_mqttc_connect(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
// {
//    uint32_t index = 0;  
//    uint32_t sessionIndex = 0;
//    qbool_t  isIntegerValid = false;

//    if (Parameter_Count < 3)
//    {
//       mqtt_client_help();
//       return QAPI_ERR_INVALID_PARAM;
//    }

//    /*             [0]      [1]
//    +MQTTCONN=<session_id>,<host>,<port>,<username>,<password>,<keepalive>,<clientid>,<clean session>
//    */

//    sessionIndex = Parameter_List[index].Integer_Value;
//    isIntegerValid = Parameter_List[index].Integer_Is_Valid;
   

//    if (isIntegerValid == false)
//    {
//       MQTT_CLIENT_PRINTF("Invalid session id %s\n", Parameter_List[index].String_Value);
//       goto end;
//    }

//    if (sessionIndex > MQTT_DEMO_SESSION_NUM)
//    {
//       MQTT_CLIENT_PRINTF("Invalid session id %d\n", sessionIndex);
//       goto end;
//    }

//    index ++;
//    MQTTClientSession_t *pMqttClientSess = &mqtt_client_sess[sessionIndex];
//    pMqttClientSess->sessionIndex = sessionIndex;

//    if ((pMqttClientSess->mqttState != MQTT_INIT)
//          && pMqttClientSess->mqttState != MQTT_FORCE_DISCONNECT)
//    {
//       MQTT_CLIENT_PRINTF("MQTT state:%d, execute disconnect command.\n", pMqttClientSess->mqttState);
//       goto end;
//    }

//    cleanupConnectInfo(pMqttClientSess);
//    uint16_t keepAliveSeconds = 0;
//    while (index < Parameter_Count)
//    {
//       //  <host>
//       if (index == 1)    
//       {
//          pMqttClientSess->serverInfo.hostNameLength = strlen(Parameter_List[index].String_Value);

//          pMqttClientSess->serverInfo.pHostName = malloc(pMqttClientSess->serverInfo.hostNameLength + 1);

//          if (pMqttClientSess->serverInfo.pHostName == NULL)
//          {
//                MQTT_CLIENT_PRINTF("MQTT malloc hostname (%d) fail\n",
//                                  pMqttClientSess->serverInfo.hostNameLength + 1);
//                goto fail;
//          }

//          memcpy((char *)pMqttClientSess->serverInfo.pHostName, Parameter_List[index].String_Value, pMqttClientSess->serverInfo.hostNameLength + 1);
//          index++;

//       }
//       // <port>
//       else if (index == 2)
//       {
//          pMqttClientSess->serverInfo.port = Parameter_List[index].Integer_Value;
//          index++;
//       }
//       // <username>
//       else if (index == 3)
//       {
//          pMqttClientSess->connectInfo.userNameLength = strlen(Parameter_List[index].String_Value);

//          pMqttClientSess->connectInfo.pUserName = malloc(pMqttClientSess->connectInfo.userNameLength + 1);

//          if (pMqttClientSess->connectInfo.pUserName == NULL)
//          {
//                MQTT_CLIENT_PRINTF("MQTT malloc username (%d) fail\n",
//                                  pMqttClientSess->connectInfo.userNameLength + 1);
//                goto fail;
//          }

//          memcpy((char *)pMqttClientSess->connectInfo.pUserName, Parameter_List[index].String_Value,
//                   pMqttClientSess->connectInfo.userNameLength + 1);

//          index++;
//       }
//       // <password>
//       else if (index == 4)
//       {
//          pMqttClientSess->connectInfo.passwordLength = strlen(Parameter_List[index].String_Value);

//          pMqttClientSess->connectInfo.pPassword = malloc(pMqttClientSess->connectInfo.passwordLength + 1);

//          if (pMqttClientSess->connectInfo.pPassword == NULL)
//          {
//                MQTT_CLIENT_PRINTF("MQTT malloc password (%d) fail\n",
//                                  pMqttClientSess->connectInfo.passwordLength + 1);
//                goto fail;
//          }

//          memcpy((char *)pMqttClientSess->connectInfo.pPassword, Parameter_List[index].String_Value,
//                   pMqttClientSess->connectInfo.passwordLength + 1);

//          index++;
//       }
//       // <clientid>
//       else if (index == 6)
//       {
//          /* The client identifier is used to uniquely identify this MQTT client to
//             * the MQTT broker. In a production device the identifier can be something
//             * unique, such as a device serial number. */
//          pMqttClientSess->connectInfo.clientIdentifierLength = strlen(Parameter_List[index].String_Value);

//          pMqttClientSess->connectInfo.pClientIdentifier = malloc(pMqttClientSess->connectInfo.clientIdentifierLength + 1);

//          if (pMqttClientSess->connectInfo.pClientIdentifier == NULL)
//          {
//                MQTT_CLIENT_PRINTF("MQTT malloc client id (%d) fail\n",
//                                  pMqttClientSess->connectInfo.clientIdentifierLength + 1);
//                goto fail;
//          }

//          memcpy((char *)pMqttClientSess->connectInfo.pClientIdentifier, Parameter_List[index].String_Value,
//                   pMqttClientSess->connectInfo.clientIdentifierLength + 1);

//          index++;
//       }
//       // <keepalive>
//       else if (index == 5)
//       {
//          if(Parameter_List[index].Integer_Value <= 0)
//          {
//                MQTT_CLIENT_PRINTF("Value of keepalive must be a positive integer.\n");
//                goto fail;
//          }

//          keepAliveSeconds = Parameter_List[index].Integer_Value;
//          if((PACKET_TX_TIMEOUT_MS/1000 < keepAliveSeconds) || (PACKET_RX_TIMEOUT_MS/1000 < keepAliveSeconds))
//          {
//                MQTT_CLIENT_PRINTF("Value of keepalive has been limited to the minimum value of PACKET_TX_TIMEOUT_MS/1000 and BPACKET_RX_TIMEOUT_MS/1000.\n");
//                MQTT_CLIENT_PRINTF("PACKET_TX_TIMEOUT_MS and BPACKET_RX_TIMEOUT_MS can be modified in core_mqtt_config.h, both default to 120000.\n");
//          }

//          pMqttClientSess->connectInfo.keepAliveSeconds = Parameter_List[index].Integer_Value;
//          index++;
//       }
//       // <clean session>
//       else if (index == 7)
//       {
//          pMqttClientSess->connectInfo.cleanSession = true;
//       }
//       else
//       {
//          MQTT_CLIENT_PRINTF("Invalid parameter %s\n", Parameter_List[index].String_Value);
//          index++;
//          return QAPI_ERR_INVALID_PARAM;
//       }
//    }


//    /* The client identifier is null, set default value. */
//    if (pMqttClientSess->connectInfo.pClientIdentifier == NULL)
//    {
//       pMqttClientSess->connectInfo.clientIdentifierLength = CLIENT_IDENTIFIER_LENGTH;

//       pMqttClientSess->connectInfo.pClientIdentifier = malloc(pMqttClientSess->connectInfo.clientIdentifierLength + 1);

//       if (pMqttClientSess->connectInfo.pClientIdentifier == NULL)
//       {
//          MQTT_CLIENT_PRINTF("MQTT malloc defaut client id (%d) fail\n",
//                               pMqttClientSess->connectInfo.clientIdentifierLength + 1);
//          goto fail;
//       }

//       memcpy((char *)pMqttClientSess->connectInfo.pClientIdentifier, CLIENT_IDENTIFIER,
//             pMqttClientSess->connectInfo.clientIdentifierLength + 1);

//    }

//    if (pMqttClientSess->serverInfo.pHostName == NULL)
//    {
//       MQTT_CLIENT_PRINTF("Error:hostname is not configured\n");
//       goto fail;
//    }

//    /* LWT Info. */
//    pMqttClientSess->lwtInfo.pTopicName = MQTT_EXAMPLE_TOPIC;
//    pMqttClientSess->lwtInfo.topicNameLength = MQTT_EXAMPLE_TOPIC_LENGTH;
//    pMqttClientSess->lwtInfo.pPayload = MQTT_EXAMPLE_MESSAGE;
//    pMqttClientSess->lwtInfo.payloadLength = strlen(MQTT_EXAMPLE_MESSAGE);
//    pMqttClientSess->lwtInfo.qos = MQTTQoS0;
//    pMqttClientSess->lwtInfo.dup = false;
//    pMqttClientSess->lwtInfo.retain = false;

//    int returnStatus = EXIT_SUCCESS;

//    returnStatus = initializeMqtt(pMqttClientSess);

//    if (returnStatus == EXIT_SUCCESS)
//    {
//       returnStatus = connectToServerWithBackoffRetries(pMqttClientSess, CONNECTION_RETRY_MAX_ATTEMPTS);
//    }

//    if (returnStatus == EXIT_SUCCESS)
//    {
//       pMqttClientSess->mqttState = MQTT_CONNECTED;

//       MQTT_CLIENT_PRINTF("MQTT connect successfully.\n");
//    }
//    else
//    {
//       pMqttClientSess->mqttState = MQTT_DISCONNECT;
//    }

//    if (mqttThreadCreated == false)
//    {
//       if (nt_qurt_thread_create(mqttc_task, "mqtt_client_task", 2048, pMqttClientSess, 6, NULL) != pdPASS)
//       {
//          MQTT_CLIENT_PRINTF("MQTT main thread create fail\n");
//       }
//       else
//       {
//          mqttThreadCreated = true;
//       }
//    }

// end:
//    return QAPI_OK;
// fail:
//    cleanupConnectInfo(pMqttClientSess);

//    return QAPI_ERR_INVALID_PARAM;
// }

// qapi_Status_t at_mqttc_init(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
// {
//    uint32_t index = 0;
//    uint32_t sessionIndex = 0;
//    qbool_t  isIntegerValid = false;
//    char buffer[WRTMEM_STR_BUFFER_LENGTH];

//    if (Parameter_Count < 1)
//    {
//       return QAPI_ERR_INVALID_PARAM;
//    }


//    /*                 [0]   [1]
//    +MQTTINIT=<session_id>,<transport scheme>,<ca_file>,<cert_file>,<key_file>,<sni>,<alpn protocol_name>
//    */

//    sessionIndex = Parameter_List[index].Integer_Value;
//    isIntegerValid = Parameter_List[index].Integer_Is_Valid;

//    if (isIntegerValid == false)
//    {
//       MQTT_CLIENT_PRINTF("Invalid session id %s\n", Parameter_List[index].String_Value);
//       goto end;
//    }

//    if (sessionIndex > MQTT_DEMO_SESSION_NUM)
//    {
//       MQTT_CLIENT_PRINTF("Invalid session id %d\n", sessionIndex);
//       goto end;
//    }

//    MQTTClientSession_t *pMqttClientSess = &mqtt_client_sess[sessionIndex];

//    if ((pMqttClientSess->mqttState != MQTT_INIT)
//          && pMqttClientSess->mqttState != MQTT_FORCE_DISCONNECT)
//    {
//       MQTT_CLIENT_PRINTF("MQTT state:%d, execute disconnect command.\n", pMqttClientSess->mqttState);
//       goto end;
//    }

//    pMqttClientSess->sessionIndex = sessionIndex;
//    cleanupNetworkCredentials(pMqttClientSess);
   
//    index++;
//    while (index < Parameter_Count)
//    {
  
//       switch (index)
//       {
//          case 1:
//             //  <transport scheme>
//             {

//                if (strcasecmp(Parameter_List[index].String_Value, "ssl") == 0)
//                {
//                      pMqttClientSess->mqttTransportScheme = MQTT_OVER_SSL;
//                }
//                else if (strcasecmp(Parameter_List[index].String_Value, "tcp") == 0)
//                {
//                      pMqttClientSess->mqttTransportScheme = MQTT_OVER_TCP;
//                }
//                else
//                {
//                      MQTT_CLIENT_PRINTF("Invalid -s parameter : %s\n", Parameter_List[index].String_Value);
//                      goto fail;
//                }

//             }
//             break;
         
//          case 2:
//             // <ca_file>
//             {
//                const char *filename = Parameter_List[index].String_Value;
//                int rootCaLen = 0;

// #ifdef CONFIG_FILE_SYSTEM
//                int file = open(filename, O_RDONLY, 0);

//                if (file < 0)
//                {
//                      MQTT_CLIENT_PRINTF("open root CA : %s fail.\n", filename);
//                      goto fail;
//                }

//                rootCaLen = lseek(file, 0, SEEK_END);

//                if (rootCaLen <= 0)
//                {
//                      MQTT_CLIENT_PRINTF("Error root CA : %s len:%d.\n", filename, rootCaLen);
//                      close(file);
//                      goto fail;
//                }

//                uint8_t *rootCa = malloc(rootCaLen + 1);

//                if (rootCa == NULL)
//                {
//                      MQTT_CLIENT_PRINTF("root CA malloc len:%d fail.\n", rootCaLen);
//                      close(file);
//                      goto fail;
//                }

//                pMqttClientSess->tlsCredentials.pRootCa = rootCa;

//                lseek(file, 0, SEEK_SET);

//                read(file, rootCa, rootCaLen);
//                close(file);
//                rootCa[rootCaLen] = 0;

//                pMqttClientSess->tlsCredentials.rootCaSize = rootCaLen + 1;
// #endif
//             }
//             break;
         
//          case 3:
//             // <cert_file>
//             {
//                const char *filename = Parameter_List[index].String_Value;
//                int clientCertLen = 0;

// #ifdef CONFIG_FILE_SYSTEM
//                int file = open(filename, O_RDONLY, 0);

//                if (file < 0)
//                {
//                      MQTT_CLIENT_PRINTF("open client cert : %s fail.\n", filename);
//                      goto fail;
//                }

//                clientCertLen = lseek(file, 0, SEEK_END);

//                if (clientCertLen <= 0)
//                {
//                      MQTT_CLIENT_PRINTF("Error client cert : %s len:%d.\n", filename, clientCertLen);
//                      close(file);
//                      goto fail;
//                }

//                uint8_t *clientCert = malloc(clientCertLen + 1);

//                if (clientCert == NULL)
//                {
//                      MQTT_CLIENT_PRINTF("client cert malloc len:%d fail.\n", clientCertLen);
//                      close(file);
//                      goto fail;
//                }

//                pMqttClientSess->tlsCredentials.pClientCert = clientCert;

//                lseek(file, 0, SEEK_SET);

//                read(file, clientCert, clientCertLen);

//                clientCert[clientCertLen] = 0;
//                close(file);

//                pMqttClientSess->tlsCredentials.clientCertSize = clientCertLen + 1;
//             }
// #endif
//             break;
         
//          case 4:
//             // <key_file>
//             {
//                const char *filename = Parameter_List[index].String_Value;
//                int clientKeyLen = 0;

// #ifdef CONFIG_FILE_SYSTEM
//                int file = open(filename, O_RDONLY, 0);

//                if (file < 0)
//                {
//                      MQTT_CLIENT_PRINTF("open client key : %s fail.\n", filename);
//                      goto fail;
//                }

//                clientKeyLen = lseek(file, 0, SEEK_END);

//                if (clientKeyLen <= 0)
//                {
//                      MQTT_CLIENT_PRINTF("Error client key : %s len:%d.\n", filename, clientKeyLen);
//                      close(file);
//                      goto fail;
//                }

//                uint8_t *clientKey = malloc(clientKeyLen + 1);

//                if (clientKey == NULL)
//                {
//                      MQTT_CLIENT_PRINTF("client key malloc len:%d fail.\n", clientKeyLen);
//                      close(file);
//                      goto fail;
//                }

//                pMqttClientSess->tlsCredentials.pPrivateKey = clientKey;

//                lseek(file, 0, SEEK_SET);

//                read(file, clientKey, clientKeyLen);
//                close(file);

//                clientKey[clientKeyLen] = 0;

//                pMqttClientSess->tlsCredentials.privateKeySize = clientKeyLen + 1;

// #endif
//             }
//             break;
         
//          case 5:
//             // <sni>
//             {
//                pMqttClientSess->tlsCredentials.disableSni = Parameter_List[index].Integer_Value;
//             }
//             break;
         
//          case 6:
//             // <alpn protocol_name>
//             {
//                pMqttClientSess->tlsCredentials.pcAlpnProtocols[0] = malloc(strlen(Parameter_List[index].String_Value) + 1);

//                if (pMqttClientSess->tlsCredentials.pcAlpnProtocols[0] == NULL)
//                {
//                      MQTT_CLIENT_PRINTF("ALPN malloc(%d) failed\n", strlen(Parameter_List[index].String_Value) + 1);
//                      goto fail;
//                }

//                memcpy((char *)pMqttClientSess->tlsCredentials.pcAlpnProtocols[0], Parameter_List[index].String_Value, strlen(Parameter_List[index].String_Value) + 1);
//                pMqttClientSess->tlsCredentials.pAlpnProtos = pMqttClientSess->tlsCredentials.pcAlpnProtocols;
//             }
//             break;

//          default:
//             break;
//       }
      
//       index++;
//    }


//    if (pMqttClientSess->mqttTransportScheme == MQTT_OVER_SSL)
//    {
//       /*SSL mode must configure CA certificates */
//       if (pMqttClientSess->tlsCredentials.pRootCa == NULL)
//       {
//          MQTT_CLIENT_PRINTF("MQTT session:%d init fail, SSL should configure CA certificates.\n",
//                               pMqttClientSess->sessionIndex);
//          goto fail;
//       }
//    }

//    pMqttClientSess->mqttState = MQTT_INIT;

//    snprintf(buffer, WRTMEM_STR_BUFFER_LENGTH, "+MQTTINIT: session %d init successfully\r\n", pMqttClientSess->sessionIndex);
//    QAT_Response_Str(QAT_RC_QUIET, buffer);

// end:
//    return QAPI_OK;
// fail:
//    cleanupNetworkCredentials(pMqttClientSess);

//    return QAPI_ERR_INVALID_PARAM;
// }

static QAT_Command_Status_t Extend_Command_MqttDestroy(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{
   char *buffer;
   char *buffer_test;
   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
   qapi_Status_t result = QAPI_OK;
   uint32_t sessionIndex = 0;
   qbool_t  isIntegerValid = false;

   switch (Op_Type)
   {
      case QAT_OP_EXEC:		     /* AT+WRTMEM */
      {	
	      buffer = malloc(CMD_STR_BUFFER_LENGTH);
         if(!buffer)
         {
            snprintf(buffer, CMD_STR_BUFFER_LENGTH, "+MQTTCONN:Malloc Fail\r\n");
            QAT_Response_Str(QAT_RC_ERROR, buffer);
            return rc;
         }
         snprintf(buffer, CMD_STR_BUFFER_LENGTH, "+MQTTDESTROY=<session_id>\r\n");
         rc = QAT_Response_Str(QAT_RC_OK, buffer);
         memset((void*)buffer, 0, CMD_STR_BUFFER_LENGTH);
         free(buffer);
         buffer = NULL;
         break;
      }
      
      case QAT_OP_EXEC_W_PARAM: 	     /* AT+WRTMEM */
      {
         // if( Parameter_Count != 8 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid || !Parameter_List[1].Integer_Is_Valid|| !Parameter_List[2].Integer_Is_Valid) {
         //    rc = QAT_Response_Str(QAT_RC_ERROR, "Wrong Input, AT+MQTTINIT for hint\r\n");
         //       return rc;
         // }
         
         buffer = malloc(CMD_STR_BUFFER_LENGTH);
         if(!buffer)
         {
            QAT_Response_Str(QAT_RC_ERROR, NULL);
            return rc;
         }

         sessionIndex = Parameter_List[0].Integer_Value;
         isIntegerValid = Parameter_List[0].Integer_Is_Valid;

         if (isIntegerValid == false || sessionIndex >= MQTT_DEMO_SESSION_NUM)
         {
            snprintf(buffer, CMD_STR_BUFFER_LENGTH, "Invalid session id %s\n", Parameter_List[0].String_Value);
            rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
            
         }else{
            result = mqttc_destroy(sessionIndex);
            if(result == QAPI_OK)
            {
               rc = QAT_Response_Str(QAT_RC_OK, NULL);
            }else{
               snprintf(buffer, CMD_STR_BUFFER_LENGTH, "+MQTTCONN:FAIL,%d\r\n",result);
               rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
            }
         }

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

static QAT_Command_Status_t Extend_Command_MqttAlpn(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{
   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
   return rc;
}

static QAT_Command_Status_t Extend_Command_MqttSni(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{
   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
   return rc;
}

static QAT_Command_Status_t Extend_Command_MqttConn(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{
   char *buffer_test;
   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
   qapi_Status_t result = QAPI_OK;
   uint32_t sessionIndex = 0;
   qbool_t  isIntegerValid = false;
   char buffer[WRTMEM_STR_BUFFER_LENGTH];

   switch (Op_Type)
   {
      case QAT_OP_EXEC:		     /* AT+WRTMEM */
      {	
         snprintf(buffer, CMD_STR_BUFFER_LENGTH, "+MQTTCONN=<session_id>,<host>,<port>,<username>,<password>,<keepalive>,<clientid>,<clean session>\r\n");
         rc = QAT_Response_Str(QAT_RC_OK, buffer);

         break;
      }
      
      case QAT_OP_EXEC_W_PARAM: 	     /* AT+WRTMEM */
      {

         sessionIndex = Parameter_List[0].Integer_Value;
         isIntegerValid = Parameter_List[0].Integer_Is_Valid;

         if (isIntegerValid == false || sessionIndex >= MQTT_DEMO_SESSION_NUM)
         {
            snprintf(buffer, CMD_STR_BUFFER_LENGTH, "+MQTTCONN:Invalid session id %s\n", Parameter_List[0].String_Value);
            rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
            
         }else{
            result = mqttc_connect(Parameter_Count,(QAPI_Console_Parameter_t *) Parameter_List);
            if(result == QAPI_OK)
            {
               rc = QAT_Response_Str(QAT_RC_OK, NULL);
            }else{
               snprintf(buffer, CMD_STR_BUFFER_LENGTH, "+MQTTCONN:FAIL,%d\r\n",result);
               rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
            }
         }
         break;
      }

      case QAT_OP_QUERY: 	     /* AT+WRTMEM */
      {
         mqttc_connect_info_query();
         QAT_Response_Str(QAT_RC_OK, NULL);
         break;
      }
      
      default:
         ;
   }
   
   return rc;
}

static QAT_Command_Status_t Extend_Command_MqttPub(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{
   char *buffer;
   char *buffer_test;
   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
   qapi_Status_t result = QAPI_OK;
   uint32_t sessionIndex = 0;
   qbool_t  isIntegerValid = false;

   switch (Op_Type)
   {
      case QAT_OP_EXEC:		     /* AT+WRTMEM */
      {	
	      buffer = malloc(CMD_STR_BUFFER_LENGTH);
         if(!buffer)
         {
            snprintf(buffer, CMD_STR_BUFFER_LENGTH, "+MQTTPUB:Malloc Fail\r\n");
            QAT_Response_Str(QAT_RC_ERROR, buffer);
            return rc;
         }
         snprintf(buffer, CMD_STR_BUFFER_LENGTH, "+MQTTPUB=<session_id>,<\"topic\">,<qos_level>,<\"message\">,<retain>\r\n");
         rc = QAT_Response_Str(QAT_RC_OK, buffer);
         memset((void*)buffer, 0, CMD_STR_BUFFER_LENGTH);
         free(buffer);
         buffer = NULL;
         break;
      }
      
      case QAT_OP_EXEC_W_PARAM: 	     /* AT+WRTMEM */
      {
         
         buffer = malloc(CMD_STR_BUFFER_LENGTH);
         if(!buffer)
         {
            QAT_Response_Str(QAT_RC_ERROR, NULL);
            return rc;
         }

         sessionIndex = Parameter_List[0].Integer_Value;
         isIntegerValid = Parameter_List[0].Integer_Is_Valid;

         if (isIntegerValid == false || sessionIndex >= MQTT_DEMO_SESSION_NUM)
         {
            snprintf(buffer, CMD_STR_BUFFER_LENGTH, "Invalid session id %s\n", Parameter_List[0].String_Value);
            rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
            
         }else{
            result = mqttc_publish(Parameter_Count, (QAPI_Console_Parameter_t *)Parameter_List);
            if(result == QAPI_OK)
            {
               rc = QAT_Response_Str(QAT_RC_OK, NULL);
            }else{
               snprintf(buffer, CMD_STR_BUFFER_LENGTH, "+MQTTPUB:FAIL,%d\r\n",result);
               rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
            }
         }

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

static QAT_Command_Status_t Extend_Command_MqttPubRaw(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{
   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
   return rc;
}

static QAT_Command_Status_t Extend_Command_MqttSub(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{
   char *buffer;
   char *buffer_test;
   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
   qapi_Status_t result = QAPI_OK;
   uint32_t sessionIndex = 0;
   qbool_t  isIntegerValid = false;

   switch (Op_Type)
   {
      case QAT_OP_QUERY:
      {
         mqttc_sub_info_query();
         QAT_Response_Str(QAT_RC_OK, NULL);
         break;
      }

      case QAT_OP_EXEC:		     /* AT+WRTMEM */
      {	
	      buffer = malloc(CMD_STR_BUFFER_LENGTH);
         if(!buffer)
         {
            snprintf(buffer, CMD_STR_BUFFER_LENGTH, "+MQTTSUB:Malloc Fail\r\n");
            QAT_Response_Str(QAT_RC_ERROR, buffer);
            return rc;
         }
         snprintf(buffer, CMD_STR_BUFFER_LENGTH, "+MQTTSUB=<session_id>,<\"topic\">,<requested_qos>\r\n");
         rc = QAT_Response_Str(QAT_RC_OK, buffer);
         memset((void*)buffer, 0, CMD_STR_BUFFER_LENGTH);
         free(buffer);
         buffer = NULL;
         break;
      }
      
      case QAT_OP_EXEC_W_PARAM: 	     /* AT+WRTMEM */
      {
         // if( Parameter_Count != 8 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid || !Parameter_List[1].Integer_Is_Valid|| !Parameter_List[2].Integer_Is_Valid) {
         //    rc = QAT_Response_Str(QAT_RC_ERROR, "Wrong Input, AT+MQTTINIT for hint\r\n");
         //       return rc;
         // }
         
         buffer = malloc(CMD_STR_BUFFER_LENGTH);
         if(!buffer)
         {
            QAT_Response_Str(QAT_RC_ERROR, NULL);
            return rc;
         }

         sessionIndex = Parameter_List[0].Integer_Value;
         isIntegerValid = Parameter_List[0].Integer_Is_Valid;

         if (isIntegerValid == false || sessionIndex >= MQTT_DEMO_SESSION_NUM)
         {
            snprintf(buffer, CMD_STR_BUFFER_LENGTH, "Invalid session id %s\n", Parameter_List[0].String_Value);
            rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
            
         }else{
            result = mqttc_subscribe(Parameter_Count, (QAPI_Console_Parameter_t *)Parameter_List);
            if(result == QAPI_OK)
            {
               rc = QAT_Response_Str(QAT_RC_OK, NULL);
            }else{
               snprintf(buffer, CMD_STR_BUFFER_LENGTH, "+MQTTSUB:FAIL,%d\r\n",result);
               rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
            }
         }

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

static QAT_Command_Status_t Extend_Command_MqttUnSub(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{
 char *buffer;
   char *buffer_test;
   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
   qapi_Status_t result = QAPI_OK;
   uint32_t sessionIndex = 0;
   qbool_t  isIntegerValid = false;

   switch (Op_Type)
   {
      case QAT_OP_EXEC:		     /* AT+WRTMEM */
      {	
	      buffer = malloc(CMD_STR_BUFFER_LENGTH);
         if(!buffer)
         {
            snprintf(buffer, CMD_STR_BUFFER_LENGTH, "+MQTTUNSUB:Malloc Fail\r\n");
            QAT_Response_Str(QAT_RC_ERROR, buffer);
            return rc;
         }
         snprintf(buffer, CMD_STR_BUFFER_LENGTH, "+MQTTUNSUB=<session_id>,<\"topic\">\r\n");
         rc = QAT_Response_Str(QAT_RC_OK, buffer);
         memset((void*)buffer, 0, CMD_STR_BUFFER_LENGTH);
         free(buffer);
         buffer = NULL;
         break;
      }
      
      case QAT_OP_EXEC_W_PARAM: 	     /* AT+WRTMEM */
      {
         // if( Parameter_Count != 8 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid || !Parameter_List[1].Integer_Is_Valid|| !Parameter_List[2].Integer_Is_Valid) {
         //    rc = QAT_Response_Str(QAT_RC_ERROR, "Wrong Input, AT+MQTTINIT for hint\r\n");
         //       return rc;
         // }
         
         buffer = malloc(CMD_STR_BUFFER_LENGTH);
         if(!buffer)
         {
            QAT_Response_Str(QAT_RC_ERROR, NULL);
            return rc;
         }

         sessionIndex = Parameter_List[0].Integer_Value;
         isIntegerValid = Parameter_List[0].Integer_Is_Valid;

         if (isIntegerValid == false || sessionIndex >= MQTT_DEMO_SESSION_NUM)
         {
            snprintf(buffer, CMD_STR_BUFFER_LENGTH, "Invalid session id %s\n", Parameter_List[0].String_Value);
            rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
            
         }else{
            result = mqttc_unsubscribe(Parameter_Count, (QAPI_Console_Parameter_t *)Parameter_List);
            if(result == QAPI_OK)
            {
               rc = QAT_Response_Str(QAT_RC_OK, NULL);
            }else{
               snprintf(buffer, CMD_STR_BUFFER_LENGTH, "+MQTTUNSUB:FAIL,%d\r\n",result);
               rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
            }
         }

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

static QAT_Command_Status_t Extend_Command_MqttDisconnect(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{
   char *buffer;
   char *buffer_test;
   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
   qapi_Status_t result = QAPI_OK;
   uint32_t sessionIndex = 0;
   qbool_t  isIntegerValid = false;

   switch (Op_Type)
   {
      case QAT_OP_EXEC:		     /* AT+WRTMEM */
      {	
	      buffer = malloc(CMD_STR_BUFFER_LENGTH);
         if(!buffer)
         {
            snprintf(buffer, CMD_STR_BUFFER_LENGTH, "+MQTTDISCONN:Malloc Fail\r\n");
            QAT_Response_Str(QAT_RC_ERROR, buffer);
            return rc;
         }
         snprintf(buffer, CMD_STR_BUFFER_LENGTH, "+MQTTDISCONN=<session_id>\r\n");
         rc = QAT_Response_Str(QAT_RC_OK, buffer);
         memset((void*)buffer, 0, CMD_STR_BUFFER_LENGTH);
         free(buffer);
         buffer = NULL;
         break;
      }
      
      case QAT_OP_EXEC_W_PARAM: 	     /* AT+WRTMEM */
      {
         // if( Parameter_Count != 8 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid || !Parameter_List[1].Integer_Is_Valid|| !Parameter_List[2].Integer_Is_Valid) {
         //    rc = QAT_Response_Str(QAT_RC_ERROR, "Wrong Input, AT+MQTTINIT for hint\r\n");
         //       return rc;
         // }
         
         buffer = malloc(CMD_STR_BUFFER_LENGTH);
         if(!buffer)
         {
            QAT_Response_Str(QAT_RC_ERROR, NULL);
            return rc;
         }

         sessionIndex = Parameter_List[0].Integer_Value;
         isIntegerValid = Parameter_List[0].Integer_Is_Valid;

         if (isIntegerValid == false || sessionIndex >= MQTT_DEMO_SESSION_NUM)
         {
            snprintf(buffer, CMD_STR_BUFFER_LENGTH, "Invalid session id %s\n", Parameter_List[0].String_Value);
            rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
            
         }else{
            result = mqttc_disconnect(Parameter_Count, (QAPI_Console_Parameter_t *)Parameter_List);
            if(result == QAPI_OK)
            {
               rc = QAT_Response_Str(QAT_RC_OK, NULL);
            }else{
               snprintf(buffer, CMD_STR_BUFFER_LENGTH, "+MQTTDISCONN:FAIL,%d\r\n",result);
               rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
            }
         }

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


/**
   @brief Processes the Extend command from the QAT.

   This command will change the current group to its parent. No parameters are
   expected for this command.

   @param[in] Op_Type          The input command type.
   @param[in] Parameter_Count  Number of parameters that were entered into the
                               command line.
   @param[in] Parameter_List   List of parameters entered into the command line.
*/
static QAT_Command_Status_t Extend_Command_MqttLongClientID(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
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
static QAT_Command_Status_t Extend_Command_MqttLongUserName(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
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
static QAT_Command_Status_t Extend_Command_MqttLongPassword(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{

   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;

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
static QAT_Command_Status_t Extend_Command_MqttInit(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{
   char *buffer;
   char *buffer_test;
   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
   qapi_Status_t result = QAPI_OK;
   uint32_t sessionIndex = 0;
   qbool_t  isIntegerValid = false;

   switch (Op_Type)
   {
      case QAT_OP_EXEC:		     /* AT+WRTMEM */
      {	
	      buffer = malloc(CMD_STR_BUFFER_LENGTH);
         if(!buffer)
         {
            snprintf(buffer, CMD_STR_BUFFER_LENGTH, "+MQTTINIT:Malloc Fail\r\n");
            QAT_Response_Str(QAT_RC_ERROR, buffer);
            return rc;
         }
         snprintf(buffer, CMD_STR_BUFFER_LENGTH, "+MQTTINIT=<session_id>,<transport scheme>,<ca_file>,<cert_file>,<key_file>,<sni>,<alpn protocol_name>\r\n");
         rc = QAT_Response_Str(QAT_RC_OK, buffer);
         memset((void*)buffer, 0, CMD_STR_BUFFER_LENGTH);
         free(buffer);
         buffer = NULL;
         break;
      }
      
      case QAT_OP_EXEC_W_PARAM: 	     /* AT+WRTMEM */
      {
         // if( Parameter_Count != 8 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid || !Parameter_List[1].Integer_Is_Valid|| !Parameter_List[2].Integer_Is_Valid) {
         //    rc = QAT_Response_Str(QAT_RC_ERROR, "Wrong Input, AT+MQTTINIT for hint\r\n");
         //       return rc;
         // }
         
         buffer = malloc(CMD_STR_BUFFER_LENGTH);
         if(!buffer)
         {
            QAT_Response_Str(QAT_RC_ERROR, NULL);
            return rc;
         }

         sessionIndex = Parameter_List[0].Integer_Value;
         isIntegerValid = Parameter_List[0].Integer_Is_Valid;

         if (isIntegerValid == false || sessionIndex >= MQTT_DEMO_SESSION_NUM)
         {
            snprintf(buffer, CMD_STR_BUFFER_LENGTH, "+MQTTINIT:Invalid session id %s\n", Parameter_List[0].String_Value);
            rc = QAT_Response_Str(QAT_RC_ERROR, buffer);

         }else{
            result = mqttc_init(Parameter_Count,(QAPI_Console_Parameter_t *) Parameter_List);
            if(result == QAPI_OK)
            {
               rc = QAT_Response_Str(QAT_RC_OK, NULL);
            }else{
               snprintf(buffer, CMD_STR_BUFFER_LENGTH, "+MQTTINIT:FAIL,%d\r\n",result);
               rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
            }
         }

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

void Initialize_QAT_Mqtt_Demo (void)
{
	qbool_t RetVal;
	RetVal = QAT_Register_Command_Group(QAT_MQTT_Command_List, MQTT_COMMAND_LIST_SIZE);
	if(RetVal == false)
   {
      printf("Failed to register MQTT command group.\n");
   }
}

