/*
 *  * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _QAPI_HTTPC_H_
#define _QAPI_HTTPC_H_

#include <stdint.h>
#include "qapi_status.h"   /* qapi_Status_t */


/** @addtogroup qapi_networking_httpc
@{ */

/**
 * @brief For use with qapi_Net_HTTPc_Request().
 */
typedef enum {
	/*supported http client methods */
	QAPI_NET_HTTP_CLIENT_GET_E = 1,
	QAPI_NET_HTTP_CLIENT_HEAD_E,
	QAPI_NET_HTTP_CLIENT_POST_E,
	QAPI_NET_HTTP_CLIENT_PUT_E,
	QAPI_NET_HTTP_CLIENT_DELETE_E,
	QAPI_NET_HTTP_CLIENT_CONNECT_E,
	QAPI_NET_HTTP_CLIENT_PATCH_E
} qapi_Net_HTTPc_Method_e;

/**
 * @brief HTTP client callback state. For use with #qapi_HTTPc_CB_t.
 */
typedef enum
{
    QAPI_NET_HTTPC_RX_ERROR_SERVER_CLOSED = -8,
    /**< Server closes the connection. */

    QAPI_NET_HTTPC_RX_ERROR_RX_PROCESS = -7,
    /**< Size section of a chunk is longer than the RX buffer length. */

    QAPI_NET_HTTPC_RX_ERROR_RX_HTTP_HEADER = -6,
    /**< Header section is longer than the RX buffer length. */

    QAPI_NET_HTTPC_RX_ERROR_INVALID_RESPONSECODE = -5,
    /**< Status code is less than 100 or greater than 999. */

    QAPI_NET_HTTPC_RX_ERROR_CLIENT_TIMEOUT = -4,
    /**< Request times out. */

    QAPI_NET_HTTPC_RX_ERROR_NO_BUFFER = -3,
    /**< RESERVED. */

    QAPI_NET_HTTPC_RX_CONNECTION_CLOSED = -2,
    /**< RESERVED. */

    QAPI_NET_HTTPC_RX_ERROR_CONNECTION_CLOSED = -1,
    /**< Connection is closed due to error on socket read. */

    QAPI_NET_HTTPC_RX_FINISHED = 0,
    /**< Response is completely received. */

    QAPI_NET_HTTPC_RX_MORE_DATA = 1,
    /**< Response is partially received. */

    QAPI_NET_HTTPC_RX_TUNNEL_ESTABLISHED = 2,
    /**< Tunnel to the origin server is established. */

    QAPI_NET_HTTPC_RX_DATA_FROM_TUNNEL = 3,
    /**< Receiving data from origin server. */

    QAPI_NET_HTTPC_RX_TUNNEL_CLOSED = 4,
    /**< Server closes the tunnel. */

    QAPI_NET_HTTPC_RX_CHUNK_CONTINUE = 5,
    /**< Receiving CONTINUE from  server */

} qapi_Net_HTTPc_CB_State_e;

#define QAPI_NET_HTTPC_CHUNKED_MASK       0x80    /*HTTP client chunk encoded*/
#define QAPI_NET_HTTPC_WITH_HEADER_MASK   0x01

typedef struct qapi_Ssl_Config {
    uint16_t         protocol;
    int              force_ciphersuite[2];
    char             *server_name;
    char             *alpn_string;
} qapi_Ssl_Config_t;

typedef struct qapi_Ssl_Cert {
    uint8_t *pRootCa;
    uint32_t rootCaSize;
    uint8_t *pClientCert;
    uint32_t clientCertSize;
    uint8_t *pPrivateKey;
    uint32_t privateKeySize;
} qapi_Ssl_Cert_t;

/**
 * @brief HTTP client response. For use with #qapi_HTTPc_CB_t.
 */
typedef struct {
    uint32_t    length;
    /**< Length of the data. */
    
    uint32_t    contentlength;
    /**< Length of the content. */

    uint32_t    resp_Code;
    /**< Response code. */

    const uint8_t *    data;
    /**< Data associated with the response if not NULL. */
} qapi_Net_HTTPc_Response_t;

/**
 * @brief User registered callback for returning response message.
 */
typedef void (*qapi_HTTPc_CB_t)(
        void* arg,
        /**< Argument passed in qapi_Net_HTTPc_New_sess2(). */

        int32_t state,
        /**< State in qapi_Net_HTTPc_CB_State_e. */

        void* value
        /**< Pointer to qapi_Net_HTTPc_Response_t. */
        );

/**
 * @brief HTTP client session handle.
 */
typedef void* qapi_Net_HTTPc_handle_t;

/**
 * @brief (Re)starts the HTTP client module.
 *
 * @details Normally, this is called to start or restart the client after it was
 *          stopped via a call to qapi_Net_HTTPc_Stop().
 *
 * @return On success, 0 is returned. On error, QAPI_NET_STATUS_HTTPC_xxx is returned.
 */
qapi_Status_t qapi_Net_HTTPc_Start(void);

/**
 * @brief Stops the HTTP client module.
 *
 * @return On success, 0 is returned. On error, QAPI_NET_STATUS_HTTPC_xxx is returned.
 */
qapi_Status_t qapi_Net_HTTPc_Stop(void);

/**
 * @brief Starts a new a HTTP client session.
 *
 * @param[in] timeout  Timeout (in ms) on an HTTP request in this session.
 *
 * @param[in] isHttps  indicate requst is http or https.
 *
 * @param[in] callback  Pointer to the user callback function (see qapi_HTTPc_CB_t)
 *
 * @param[in] arg    Argument for the callback function.
 *
 * @param[in] httpc_Max_Body_Length  Size in bytes of message-body buffer for HTTP request.
 *
 * @param[in] httpc_Max_Header_Length   Size in bytes of header buffer for HTTP request.
 *
 * @param[in] httpc_Rx_Buffer_Size   Size in bytes of RX buffer for HTTP response.
 *                                   If size is less than 512, system will use 512.
 * @param[in] ip_prefer   prefer to select an ip type, ipv4 or ipv6
 * 
 * @param[in] ssl_Cfg   ssl_pre_buffer parameters.
 *
 * @param[in] https_auth_type   https_auth_type parameters.
 *
 * @return
 * On success, a non-NULL handle is returned; on error, NULL is returned.
 */
qapi_Net_HTTPc_handle_t qapi_Net_HTTPc_New_sess2(
        uint32_t                timeout,
        uint32_t                isHttps,
        qapi_HTTPc_CB_t         callback,
        void*                   arg,
        uint16_t                httpc_Max_Body_Length,
        uint16_t                httpc_Max_Header_Length,
        uint16_t                httpc_Rx_Buffer_Size,
        uint16_t                ip_prefer,
        uint16_t                ssl_pre_buffer,
        uint8_t                 https_auth_type);

/**
 * @brief Frees an HTTP client session.
 *
 * @details Disconnects from the server and frees the memory.
 *
 * @param[in] handle    HTTP client session handle.
 *
 * @return On success, 0 is returned. On error, QAPI_NET_STATUS_HTTPC_xxx is returned.
 */
qapi_Status_t qapi_Net_HTTPc_Free_sess(qapi_Net_HTTPc_handle_t handle);

/**
 * @brief Connects to an HTTP server in Blocking mode.
 *
 * @param[in] handle  HTTP client session handle.
 *
 * @param[in] server  Pointer to server or proxy, e.g. "192.168.2.100" or "www.example.com"
 *
 * @param[in] port  Port of the server.
 *
 * @return On success, 0 is returned. On error, QAPI_NET_STATUS_HTTPC_xxx is returned.
 */
qapi_Status_t qapi_Net_HTTPc_Connect(qapi_Net_HTTPc_handle_t handle, const char *server, uint16_t port);

/**
 * @brief Disconnects an HTTP client session from the server.
 *
 * @param[in] handle  HTTP client session handle.
 *
 * @return On success, 0 is returned. On error, QAPI_NET_STATUS_HTTPC_xxx is returned.
 */
qapi_Status_t qapi_Net_HTTPc_Disconnect(qapi_Net_HTTPc_handle_t handle);

/**
 * @brief Send an HTTP request to an HTTP server or proxy.
 *
 * @param[in] handle  HTTP client session handle.
 *
 * @param[in] cmd   HTTP request (see qapi_Net_HTTPc_Method_e)
 *
 * @param[in] URL   Pointer to the request URL, e.g.
 *                  "index.html" or "/cgi/mycgi.pl" if cmd is not QAPI_NET_HTTP_CLIENT_CONNECT_E,
 *                  "www.example.com:22" if cmd is QAPI_NET_HTTP_CLIENT_CONNECT_E.
 *
 * @return On success, 0 is returned. On error, QAPI_NET_STATUS_HTTPC_xxx is returned.
 */
qapi_Status_t qapi_Net_HTTPc_Request(qapi_Net_HTTPc_handle_t handle, qapi_Net_HTTPc_Method_e cmd, const char *URL);

/**
 * @brief Send an HTTP CONNECT request to a proxy for establishing a connection to an HTTPS origin server.
 *
 * @param[in] handle  HTTP client session handle.
 *
 * @param[in] calist  A file (on the local file system) containing
 *                    CA certificates, which are in SharkSSL format. This
 *                    is used for authenticating the origin HTTPS server.
 *                    It can be set to NULL.
 *
 * @param[in] URL   Pointer to the request URL, e.g. "www.example.com:22"
 *
 * @return On success, 0 is returned. On error, QAPI_NET_STATUS_HTTPC_xxx is returned.
 */
qapi_Status_t qapi_Net_HTTPc_Tunnel_To_HTTPS(qapi_Net_HTTPc_handle_t handle, const char *calist, const char *URL);

/**
 * @brief Sets the body on an HTTP client session.
 *
 * @param[in] handle  HTTP client session handle.
 *
 * @param[in] body   Pointer to the body.
 *
 * @param[in] body_Length  Length of the body.
 *
 * @return On success, 0 is returned. On error, QAPI_NET_STATUS_HTTPC_xxx is returned.
 */
qapi_Status_t qapi_Net_HTTPc_Set_Body(qapi_Net_HTTPc_handle_t handle, const char *body, uint32_t body_Length);

/**
 * @brief Forms a URL-encoded string on an HTTP client session.
 *
 * @param[in] handle    HTTP client session handle.
 *
 * @param[in] key   Pointer to the key.
 *
 * @param[in] value    Pointer to the value.
 *
 * @code
 *        // The following calls generate a URL-encoded string which will be
 *        // the message body for POST request or
 *        // the query string for GET request.
 *        qapi_Net_HTTPc_Set_Param(handle, "name", "Lucy");
 *        qapi_Net_HTTPc_Set_Param(handle, "neighbors", "Fred & Ethel");
 *
 *        // For example, if user calls
 *        // qapi_Net_HTTPc_Request(handle, QAPI_NET_HTTP_CLIENT_GET_E, "index.html");
 *        // the start line, "GET /index.html?name=Lucy&neighbors=Fred+%26+Ethel HTTP/1.1\r\n",
 *        // is generated.
 * @endcode \n
 *
 * @return On success, 0 is returned. On error, QAPI_NET_STATUS_HTTPC_xxx is returned.
 */
qapi_Status_t qapi_Net_HTTPc_Set_Param(qapi_Net_HTTPc_handle_t handle, const char *key, const char *value);

/**
 * @brief Sets the header field for an HTTP client session.
 *
 * @param[in] handle   HTTP client session handle.
 *
 * @param[in] type     Pointer to the type.
 *
 * @param[in] value    Pointer to the value.
 *
 * @note    If this API is not called, the system will send the following headers for user:
 *          - Host: <hostname>:<port>\r\n
 *          - Accept: text/html, <asterisk>/<asterisk>\r\n
 *          - User-Agent: IOE Client\r\n
 *          - Connection: keep-alive\r\n
 *          - Cache-control: no-cache\r\n
 *          Additionally, for POST, PUT and PATCH requests if message body has data:
 *          - Content-length: <nn>\r\n
 *          - Content-Type: application/x-www-form-urlencoded\r\n (for POST request)
 *
 *          If this API is called, the system will send the following headers for user:
 *          - Host: <hostname>:<port>\r\n
 *          - Connection: keep-alive\r\n
 *          - User's own headers added by calling this API
 *          Additionally, for POST, PUT and PATCH requests if message body has data:
 *          - Content-length: <nn>\r\n
 *
 * @code
 *        // The following calls will generate request headers in an HTTP GET request:
 *        // "User-Agent: My Own Browser 1.0\r\n"
 *        // "Connection: keep-alive\r\n"
 *        qapi_Net_HTTPc_Add_Header_Field(handle, "User-Agent", "My Own Browser 1.0");
 *        qapi_Net_HTTPc_Add_Header_Field(handle, "Connection", "keep-alive");
 *        qapi_Net_HTTPc_Request(handle, QAPI_NET_HTTP_CLIENT_GET_E, "index.html");
 * @endcode \n
 *
 * @return On success, 0 is returned. On error, QAPI_NET_STATUS_HTTPC_xxx is returned.
 */
qapi_Status_t qapi_Net_HTTPc_Add_Header_Field(qapi_Net_HTTPc_handle_t handle, const char *type, const char *value);

/**
 * @brief Clears the header field for an HTTP client session.
 *
 * @param[in] handle    HTTP client session handle.
 *
 * @return On success, 0 is returned. On error, QAPI_NET_STATUS_HTTPC_xxx is returned.
 */
qapi_Status_t qapi_Net_HTTPc_Clear_Header(qapi_Net_HTTPc_handle_t handle);

/**
 * @brief Sets SSL configuration parameters on a secure (i.e., HTTPS) session.
 *
 * @param[in] handle    HTTP client session handle.
 *
 * @param[in] ssl_Cfg   SSL connection parameters.
 *
 * @return On success, 0 is returned. On error, QAPI_NET_STATUS_HTTPC_xxx is returned.
 */
qapi_Status_t qapi_Net_HTTPc_Configure_SSL(qapi_Net_HTTPc_handle_t handle, qapi_Ssl_Config_t *ssl_Cfg);

/**
 * @brief Sets SSL certificate parameters on a secure (i.e., HTTPS) session.
 *
 * @param[in] handle    HTTP client session handle.
 *
 * @param[in] ssl_Cfg   SSL certificate parameters.
 *
 * @return On success, 0 is returned. On error, QAPI_NET_STATUS_HTTPC_xxx is returned.
 */
qapi_Status_t qapi_Net_HTTPc_Configure_Cert(qapi_Net_HTTPc_handle_t handle, qapi_Ssl_Cert_t *ssl_Cfg);

/**
 * @brief Enables/disables the addition of an HTTP head in a session callback.
 *
 * @details By default, the system returns the message body of response via user registered callback (see qapi_HTTPc_CB_t).
 *          Message headers are not returned. To enable the system to also return messages headers, this API should be called
 *          with 'enable'= 1.
 *
 * @param[in] handle   HTTP client session handle.
 *
 * @param[in] enable   1 -- enable; 0 -- disable.
 *
 * @return On success, 0 is returned. On error, QAPI_NET_STATUS_HTTPC_xxx is returned.
 */
qapi_Status_t qapi_Net_HTTPc_CB_Enable_Adding_Header(qapi_Net_HTTPc_handle_t handle, uint16_t enable);

/**
 * @brief Send raw data when an HTTP tunnel is established.
 *
 * @param[in] handle  HTTP client session handle.
 *
 * @param[in] buf   Pointer to data
 *
 * @param[in] length  Length of data
 *
 * @return On success, 0 is returned. On error, QAPI_NET_STATUS_HTTPC_xxx is returned.
 */
qapi_Status_t qapi_Net_HTTPc_Send_Data(qapi_Net_HTTPc_handle_t handle, const char *buf, uint32_t length);

 /**
  * @brief Send chunk data.
  *
  * @param[in] handle  HTTP client session handle.
  *
  * @param[in] cmd   HTTP request (see qapi_Net_HTTPc_Method_e)
  *
  * @param[in] URL   Pointer to the request URL, e.g.
  *                  "index.html" or "/cgi/mycgi.pl"
  *
  * @param[in] chunk   Pointer to chunk data
  *
  * @param[in] chunk_size  Length of chunk data
  *
  * @param[in] chunk_type  0x00 -- non chunk encoded without http header; 0x01 -- non chunk encoded with http header(first pakcet);
  *                        0x80 -- chunk encoded without http header; 0x81 -- chunk encoded with http header;
  *
  * @return
  * When all data is sent, 0 is returned; on error, non-zero is returned.
  */
qapi_Status_t qapi_Net_HTTPc_Send_Chunk(qapi_Net_HTTPc_handle_t handle, qapi_Net_HTTPc_Method_e cmd, const char *URL, const char *chunk, uint32_t chunk_size, uint8_t chunk_flag, int32_t total_size);

 /**
  * @brief Rlease Pre-allocate buffer.
  *
  * @return On success, 0 is returned. On error, QAPI_NET_STATUS_HTTPC_xxx is returned.
  */
 qapi_Status_t qapi_Net_HTTPc_Rlease_Pre_allocate_buffer(void);

 /** @} */ /* end_addtogroup qapi_networking_httpc */

#endif /* _QAPI_HTTPC_H_ */
