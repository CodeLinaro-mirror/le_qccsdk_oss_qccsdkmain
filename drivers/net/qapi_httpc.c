/*
 *  * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <stddef.h>         /* NULL */
#include "qapi_net_status.h"
#include "qapi_httpc.h"     /* QAPI_NET_HTTPC_* */
#include "qcom_http_client.h"

/****************************************************************************
 ***************************************************************************/
static
qapi_Status_t httpc_convert_err_code(int32_t err)
{
    qapi_Status_t status;

    switch (err)
    {
        case HTTPC_OK:
            status = QAPI_OK;
            break;

        case HTTPC_ERR_NONE_SESS:
            status = QAPI_NET_STATUS_HTTPC_NONEXISTENT_SESSION;
            break;

        case HTTPC_ERR_CONN:
            status = QAPI_NET_STATUS_HTTPC_CONNECT_FAILED;
            break;

        case HTTPC_ERR_SEND:
            status = QAPI_NET_STATUS_HTTPC_SOCKET_SEND_FAILED;
            break;

        case HTTPC_ERR_NO_MEMORY:
            status = QAPI_NET_STATUS_HTTPC_NO_MEMORY;
            break;

        case HTTPC_ERR_BUSY:
            status = QAPI_NET_STATUS_HTTPC_BUSY;
            break;

        case HTTPC_ERR_INVALID_PARAM:
            status = QAPI_NET_STATUS_HTTPC_INVALID_PARAM;
            break;

        case HTTPC_ERR_SSL_CONN:
            status = QAPI_NET_STATUS_HTTPC_SSL_CONN_ERROR;
            break;

        case HTTPC_ERR_SOCKET_OPEN:
            status = QAPI_NET_STATUS_HTTPC_SOCKET_CREATION_FAILED;
            break;

        default:
            status = QAPI_ERROR;
            break;
    }
    return status;
}

/****************************************************************************
 ***************************************************************************/
qapi_Status_t qapi_Net_HTTPc_Start(void)
{
    int err;

    err = http_client_start();
    return httpc_convert_err_code(err);
}

/****************************************************************************
 ***************************************************************************/
qapi_Status_t qapi_Net_HTTPc_Stop(void)
{
    int err;

    err = http_client_stop();
    return httpc_convert_err_code(err);
}

/****************************************************************************
 ***************************************************************************/
qapi_Net_HTTPc_handle_t
qapi_Net_HTTPc_New_sess2(
            uint32_t timeout,
            uint32_t isHttps,
            qapi_HTTPc_CB_t callback,
            void* arg,
            uint16_t httpc_Max_Body_Length,
            uint16_t httpc_Max_Header_Length,
            uint16_t httpc_Rx_Buffer_Size,
            uint16_t ip_prefer,
            uint16_t ssl_pre_buffer,
            uint8_t https_auth_type)
{
    return (qapi_Net_HTTPc_handle_t)http_client_newsess(
            timeout,
            isHttps,
            callback,
            arg,
            httpc_Max_Body_Length,
            httpc_Max_Header_Length,
            httpc_Rx_Buffer_Size,
            ip_prefer,
            ssl_pre_buffer,
            https_auth_type);
}

/****************************************************************************
 ***************************************************************************/
qapi_Status_t qapi_Net_HTTPc_Free_sess(qapi_Net_HTTPc_handle_t handle)
{
    int err;

    if (handle == NULL)
    {
        return QAPI_ERROR;
    }

    err = http_client_freesess((httpclient_sess *)handle);
    return httpc_convert_err_code(err);
}

/****************************************************************************
 ***************************************************************************/
qapi_Status_t qapi_Net_HTTPc_Connect(qapi_Net_HTTPc_handle_t handle, const char *URL, uint16_t port)
{
    int err;

    if (handle == NULL || URL == NULL)
    {
        return QAPI_ERROR;
    }
    err = http_client_connect((httpclient_sess *)handle, URL, port);
    return httpc_convert_err_code(err);
}

/****************************************************************************
 ***************************************************************************/
qapi_Status_t qapi_Net_HTTPc_Disconnect(qapi_Net_HTTPc_handle_t handle)
{
    int err;

    if (handle == NULL)
    {
        return QAPI_ERROR;
    }
    err = http_client_disconnect((httpclient_sess *)handle);
    return httpc_convert_err_code(err);
}

/****************************************************************************
 ***************************************************************************/
qapi_Status_t qapi_Net_HTTPc_Request(qapi_Net_HTTPc_handle_t handle, qapi_Net_HTTPc_Method_e cmd, const char *URL)
{
    int err;

    if (handle == NULL || URL == NULL)
    {
        return QAPI_ERROR;
    }
    err = http_client_request((httpclient_sess *)handle, (HTTPC_REQUEST_CMD_E)cmd, URL);
    return httpc_convert_err_code(err);
}

/****************************************************************************
 ***************************************************************************/
#ifdef HTTP_TUNNEL
qapi_Status_t qapi_Net_HTTPc_Tunnel_To_HTTPS(qapi_Net_HTTPc_handle_t handle, const char *calist, const char *URL)
{
    int err;

    if (handle == NULL || URL == NULL)
    {
        return QAPI_ERROR;
    }
    err = http_client_tunnel_to_https((httpclient_sess *)handle, calist, URL);
    return httpc_convert_err_code(err);
}
#endif
/****************************************************************************
 ***************************************************************************/
qapi_Status_t qapi_Net_HTTPc_Set_Body(qapi_Net_HTTPc_handle_t handle, const char *body, uint32_t bodylen)
{
    int err;

    if (handle == NULL || body == NULL)
    {
        return QAPI_ERROR;
    }
    err = http_client_set_body((httpclient_sess *)handle, body, bodylen);
    return httpc_convert_err_code(err);
}

/****************************************************************************
 ***************************************************************************/
qapi_Status_t qapi_Net_HTTPc_Set_Param(qapi_Net_HTTPc_handle_t handle, const char *key, const char *value)
{
    int err;

    if (handle == NULL || key == NULL || value == NULL)
    {
        return QAPI_ERROR;
    }
    err = http_client_set_param((httpclient_sess *)handle,key,value);
    return httpc_convert_err_code(err);
}

/****************************************************************************
 ***************************************************************************/
qapi_Status_t qapi_Net_HTTPc_Add_Header_Field(qapi_Net_HTTPc_handle_t handle, const char *type, const char *value)
{
    int err;

    if (handle == NULL || type == NULL || value == NULL)
    {
        return QAPI_ERROR;
    }
    err = http_client_add_header_field((httpclient_sess *)handle,type,value);
    return httpc_convert_err_code(err);
}

/****************************************************************************
 ***************************************************************************/
qapi_Status_t qapi_Net_HTTPc_Clear_Header(qapi_Net_HTTPc_handle_t handle)
{
    int err;

    if (handle == NULL)
    {
        return QAPI_ERROR;
    }
    err = http_client_clear_header((httpclient_sess *)handle);
    return httpc_convert_err_code(err);
}

/****************************************************************************
 ***************************************************************************/
qapi_Status_t qapi_Net_HTTPc_Configure_SSL(qapi_Net_HTTPc_handle_t handle, qapi_Ssl_Config_t *ssl_Cfg)
{
    int err;

    if (handle == NULL || ssl_Cfg == NULL)
    {
        return QAPI_ERROR;
    }
    err = http_client_sslconfigure((httpclient_sess *)handle, ssl_Cfg);
    return httpc_convert_err_code(err);
}

/****************************************************************************
 ***************************************************************************/
qapi_Status_t qapi_Net_HTTPc_Configure_Cert(qapi_Net_HTTPc_handle_t handle, qapi_Ssl_Cert_t *ssl_Crt)
{
    int err;

    if (handle == NULL || ssl_Crt == NULL)
    {
        return QAPI_ERROR;
    }
    err = http_client_certconfigure((httpclient_sess *)handle, ssl_Crt);
    return httpc_convert_err_code(err);
}

/****************************************************************************
 ***************************************************************************/
qapi_Status_t qapi_Net_HTTPc_CB_Enable_Adding_Header(qapi_Net_HTTPc_handle_t handle,uint16_t enable)
{
    int err;

    if (handle == NULL)
    {
        return QAPI_ERROR;
    }
    err = http_client_cb_eable_adding_header((httpclient_sess *)handle,enable);
    return httpc_convert_err_code(err);
}

/****************************************************************************
 ***************************************************************************/
qapi_Status_t qapi_Net_HTTPc_Send_Data(qapi_Net_HTTPc_handle_t handle, const char *buf, uint32_t length)
{
    int err;

    if (handle == NULL || buf == NULL || length == 0)
    {
        return QAPI_ERROR;
    }
    err = http_client_senddata((httpclient_sess *)handle, (char *)buf, length);
    return httpc_convert_err_code(err);
}

/****************************************************************************
 ***************************************************************************/
qapi_Status_t qapi_Net_HTTPc_Send_Chunk(qapi_Net_HTTPc_handle_t handle, qapi_Net_HTTPc_Method_e cmd, const char *URL, const char *chunk, uint32_t chunk_size, uint8_t chunk_flag, int32_t total_size)
{
    int err;

    if (handle == NULL || URL == NULL)
    {
        return QAPI_ERROR;
    }
    err = http_client_send_chunk((httpclient_sess *)handle, cmd, URL, chunk, chunk_size, chunk_flag, total_size);
    return httpc_convert_err_code(err);
}

/****************************************************************************
 ***************************************************************************/
qapi_Status_t qapi_Net_HTTPc_Rlease_Pre_allocate_buffer(void)
{
    int err;

    err = http_client_release_pre_allcoate_buffer();
    return httpc_convert_err_code(err);
}

