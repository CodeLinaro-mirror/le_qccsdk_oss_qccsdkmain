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

/*-------------------------------------------------------------------------
 * Parameters define
 *-----------------------------------------------------------------------*/
#define HTTPC_COMMAND_LIST_SIZE                      (sizeof(QAT_HTTPC_Command_List) / sizeof(QAT_Command_t))
#define QAT_ATCMD_BUF_LEN	                          1400
#define VERSION_STR_BUFFER_LENGTH 					  256
#define INFO_STR_BUFFER_LENGTH					      256
#define HTTP_STR_BUFFER_LENGTH					      256
#define CMD_STR_BUFFER_LENGTH					      1024
#define TIMEOUT_MS                                    10000  //10s
#define MAX_TIMEOUT_MS                                1800000  //1800s
#define QAT_HTTPC_MAXIMUM_NUMBER_OF_PARAMETERS        15
#define QAT_HTTPC_MAXIMUM_NUMBER_OF_KEY_VALUE         (QAT_HTTPC_MAXIMUM_NUMBER_OF_PARAMETERS-2)
#define QAT_HTTPC_CLIENT_INDEX                        1
#define HTTP_HOST_STR_BUFFER_LENGTH					  50
#define HTTP_BODY_BUFFER_SIZE                         10000
#define HTTP_WAIT_RSP_TIME                            100  //100*100(HTTP_WAIT_RSP_CYCLE_INTERVAL) = 10000ms =10s
#define HTTP_WAIT_RSP_CYCLE_INTERVAL                  100  //ms
#define FILE_PATH_STR_BUFFER_LENGTH                   32
#define HTTP_URL_STR_BUFFER_LENGTH                    256
#define HTTPS_DEFAULT_PORT                            443
#define HTTP_DEFAULT_PORT                             80
#define HTTPC_DEFAULT_IP_PREFER                       0   //0:ipv4, 1:ipv6
#define QAT_HTTPC_MAX_HEADER_FIELD                    10
#define QAT_MAX_CHUNK_SIZE                            1000
#define QAT_CHUNK_INTERVAL                            10  //ms



struct at_header_field{
    char *name;
    char *value;
};

struct at_https_global_config {
#define AT_HTTPS_NOT_AUTH        0
#define AT_HTTPS_SERVER_AUTH     1
#define AT_HTTPS_CLIENT_AUTH     2
#define AT_HTTPS_BOTH_AUTH       3
    uint8_t https_auth_type;
    char ca_file[FILE_PATH_STR_BUFFER_LENGTH];
    char cert_file[FILE_PATH_STR_BUFFER_LENGTH];
    char key_file[FILE_PATH_STR_BUFFER_LENGTH];

    uint32_t http_port;
    uint32_t https_port;
    uint8_t httpc_ip_prefer;
    uint8_t is_pre_buffer;
    uint32_t url_size;
    char *url;

    //temporary resource for one at cmd opertaion
    char *temp_url;
    char *send_buff;      //len <= HTTP_BODY_BUFFER_SIZE
    uint32_t buff_offset;
    uint32_t data_len;
    uint8_t header_field_num;
    struct at_header_field header_field[QAT_HTTPC_MAX_HEADER_FIELD];
};

typedef enum {
	QAT_HTTP = 1,
	QAT_HTTPS
} qat_HTTP_type;

typedef enum {
	/*supported http client methods */
	QAT_HEAD_CONTENT_TYPE,
	QAT_HEAD_ACCEPT,                  
	QAT_HEAD_CACHE_CONTROL,             
	QAT_HEAD_USER_AGENT,
	QAT_HEAD_AUTHORIZATION,
	QAT_HEAD_MAX
} qat_head_type;


typedef enum {
	/*supported http client methods */
	QAT_HTTP_CLIENT_HEAD = 1,
	QAT_HTTP_CLIENT_GET,
	QAT_HTTP_CLIENT_POST,
	QAT_HTTP_CLIENT_PUT
} qat_HTTPc_Method;

typedef enum {
	/*supported http client methods */
	QAT_CONTENT_TYPE_X_WWW_FORM_URLENCODED,  //application/x-www-form-urlencoded, default value
	QAT_CONTENT_TYPE_JSON,                   //application/json
	QAT_CONTENT_TYPE_ZIP,                    //application/zip
	QAT_CONTENT_TYPE_FORM_DATA,              // multipart/form-data
	QAT_CONTENT_TYPE_TEXT_XML                // text/xml
} qat_content_type;

typedef enum {
	QAT_NET_CFG_HTTP_PORT,
	QAT_NET_CFG_HTTPS_PORT,            
	QAT_NET_CFG_IP_PREFER,
	QAT_NET_CFG_PRE_ACCLOCATE_SSL_BUFFER,
	QAT_NET_CFG_CACHE_DATA,
	QAT_NET_CFG_MAX
} net_cfg_type;

typedef enum{
    IP_V4,
    IP_V6
}ip_type;

typedef enum {
    QAT_SSL_PRE_BUFFER_INITIAL,
    QAT_SSL_PRE_BUFFER_ALLOCATE,
    QAT_SSL_PRE_BUFFER_RELEASE
} qat_pre_buffer_type;


/*-------------------------------------------------------------------------
 * Function Declarations
 *-----------------------------------------------------------------------*/
static int at_arg_is_string(const char *arg);
static int at_arg_is_null(const char *arg);
int at_arg_get_number(const char *arg, int *value);
int at_arg_get_hexstr_number(const char *arg, uint32_t *value);
int at_arg_get_string(const char *arg, char *string, int max);
qapi_Status_t at_httpc_stop();
qapi_Status_t at_httpc_request (int32_t opt, char *url, char *data_buf);
qapi_Status_t at_httpc_getsize (char *url, int32_t timeout);
qapi_Status_t at_httpc_get (char *url, int32_t timeout);
qapi_Status_t at_httpc_post (char *url, int32_t data_len,char *data);
qapi_Status_t at_httpc_put (char *url, int32_t data_len,char *data);
uint32_t splitKeyValuePairs(char *input, QAPI_Console_Parameter_t* Parameter_List);
void gethostURL(const char *url, char*hostURL);
qbool_t getpathURL(const char *url, char*pathURL);
void parseURL(const char *url, char *protocol, char *domain, char *path);
qapi_Status_t at_httpc_setbodydata(char *data_buf,uint32_t len);
qapi_Status_t at_httpc_addheaderfield(/*uint8_t headfield_type,*/uint8_t content_type);
qbool_t saveUrl(const char *url);
qbool_t saveheaderfield(const char *headerfield);
void resetSslInfo();
qbool_t isSecureSession(const char *url);
qbool_t create_send_buffer(int length);
void savedata(const char *data);
void reset_resource();
void reset_temp_resource();
qbool_t save_content_type(uint8_t content_type);
qbool_t is_succ_resp_code(int errorcode);
int validate_url(const char *url);


static QAT_Command_Status_t Extend_Command_HttpClient(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_HttpGetSize(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_HttpGet(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_HttpPost(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_HttpPut(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_HttpUrlCfg(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_HttpSslCfg(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_HttpNetCfg(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);


/* The following is the complete command list for the QAT common command demo. */
/** List of global commands that are supported when in a group. */
/*
AT+HTTPCLIENT:   send HTTP client request
AT+HTTPGETSIZE:  get HTTP resource size
AT+HTTPGET:      get HTTP resource
AT+HTTPPOST:     Post HTTP data
AT+HTTPPUT:      put HTTP data
AT+HTTPURLCFG:   set/get long HTTP URL
AT+HTTPSSLCFG:   set/get HTTP certificate
*/
static QAT_Command_t QAT_HTTPC_Command_List[] =
{
   {"+HTTPCLIENT",   Extend_Command_HttpClient,      QAT_OP_EXEC|QAT_OP_EXEC_W_PARAM},
   {"+HTTPGETSIZE",  Extend_Command_HttpGetSize,   QAT_OP_EXEC|QAT_OP_QUERY|QAT_OP_EXEC_W_PARAM},
   {"+HTTPGET",   Extend_Command_HttpGet,   QAT_OP_EXEC|QAT_OP_QUERY|QAT_OP_EXEC_W_PARAM},
   {"+HTTPPOST",   Extend_Command_HttpPost,   QAT_OP_EXEC|QAT_OP_EXEC_W_PARAM},
   {"+HTTPPUT",   Extend_Command_HttpPut,   QAT_OP_EXEC|QAT_OP_EXEC_W_PARAM},
   {"+HTTPURLCFG",   Extend_Command_HttpUrlCfg,   QAT_OP_EXEC|QAT_OP_EXEC_W_PARAM|QAT_OP_QUERY},
   {"+HTTPSSLCFG",   Extend_Command_HttpSslCfg,   QAT_OP_EXEC|QAT_OP_EXEC_W_PARAM|QAT_OP_QUERY},
   {"+HTTPNETCFG",    Extend_Command_HttpNetCfg,   QAT_OP_EXEC|QAT_OP_EXEC_W_PARAM|QAT_OP_QUERY},
};


#define AT_CMD_PARSE_OPT_STRING(i, string, max, valid) do { \
    if(Parameter_Count > i && !at_arg_is_null(Parameter_List[i].String_Value)) { \
        if (!at_arg_get_string(Parameter_List[i].String_Value, string, max)) { \
            return QAT_RC_ERROR; \
        } \
        valid = 1; \
    } \
} while(0);

#define AT_CMD_PARSE_NUMBER(i, num) do {\
    if (!at_arg_get_number(Parameter_List[i].Integer_Value, num)) { \
        return QAT_RC_ERROR; \
    } \
} while(0);



