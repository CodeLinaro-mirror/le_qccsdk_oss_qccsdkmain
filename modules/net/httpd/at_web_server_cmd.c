/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Copyright (c) Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * NOT A CONTRIBUTION
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <time.h>
#include <sys/queue.h>
#include "esp_http_server.h"
#include "FreeRTOS.h"
#include "timers.h"
#include "event_groups.h"
#include "lwip/err.h"
#include "lwip/sockets.h"


#include "qapi_version.h"
#include "qat.h"

#ifdef CONFIG_HTTP_SERVER
#define AT_WEB_SERVER_CHECK(a, str, goto_tag, ...)                                              \
    do                                                                                 \
    {                                                                                  \
        if (!(a))                                                                      \
        {                                                                              \
            ESP_LOGE(TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            goto goto_tag;                                                             \
        }                                                                              \
    } while (0)

#define AT_WEB_VERSION                             "1.0"
#define AT_WEB_SCRATCH_BUFSIZE                     320
#define AT_WEB_PAGE_BUFSIZE                        1024
#define AT_WEB_MAX_SSID_SIZE                       32
#define AT_WEB_MAX_PWD_SIZE                        64
#define AT_WEB_MAX_WIFI_CFG_SIZE                   128    


#define AT_WEB_MOUNT_POINT                         "/lfs"
#define AT_LFS_WIFI_FILE                           "/lfs/wifi"
#define AT_OK          0  
#define AT_FAIL        -1 

/**
 * Maximum length of path prefix (not including zero terminator)
 */
#define AT_LFS_PATH_MAX 15

typedef struct {
    uint8_t ssid[AT_WEB_MAX_SSID_SIZE+1];
    uint8_t password[AT_WEB_MAX_PWD_SIZE+1];
} wifi_config_t;

typedef struct web_server_context {
    char base_path[AT_LFS_PATH_MAX + 1];
    char scratch[AT_WEB_SCRATCH_BUFSIZE];	
	char html_buf[AT_WEB_PAGE_BUFSIZE];
	wifi_config_t wifi_cfg;
} web_server_context_t;

static web_server_context_t *s_web_context = NULL;
static httpd_handle_t s_server = NULL;
static const char *TAG = "at web";
wifi_config_t *p_wifi_cfg = NULL;
char *p_html_buf = NULL;

int at_read_wifi_info(void);
static esp_err_t web_common_get_handler(httpd_req_t *req);
static esp_err_t at_web_version_get_handler(httpd_req_t *req);
static esp_err_t config_wifi_post_handler(httpd_req_t *req);

const char *html_page = 
	"<html lang='en'>"
	"<head>"
	    "<meta charset='UTF-8'>"
	    "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
	    "<title>Wi-Fi Configuration</title>"
	"</head>"
	"<body>"
	    "<h1>Wi-Fi Setup</h1>"
	    "<form action='/setwifi' method='POST'>"
	        "<label for='ssid'>SSID:</label><br>"
	        "<input type='text' id='ssid' name='ssid' value='%s' required>"
	        "<br><br>"
	        "<label for='password'>Password:</label><br>"
	        "<input type='test' id='password' name='password' value='%s' required>"
	        "<br><br>"
	        "<button type='submit'>Submit</button>"
	    "</form>"
	"</body>"
	"</html>";

int at_get_wifi_cfg(char *ssid, char *password)
{
    if ((NULL == ssid) 
		|| (NULL == password)
		|| (AT_OK != at_read_wifi_info()))
    {
        return AT_FAIL;
    }

    snprintf(ssid, sizeof(p_wifi_cfg->ssid), "%s", p_wifi_cfg->ssid);
    snprintf(password, sizeof(p_wifi_cfg->password), "%s", p_wifi_cfg->password);
	
    return AT_OK;
}

static int at_web_url_decode(char *src, int src_len, char *des, int des_len)
{
    int len = MIN(src_len, des_len);

    memset(des, 0, des_len);
    memcpy(des, src, len);

    return len;
}

/**
 * @brief Find a specific arg in a string of get- or post-data.
 * Line is the string of post/get-data, arg is the name of the value to find. The
 * zero-terminated result is written in buff, with at most buffLen bytes used. The
 * function returns the length of the result, or -1 if the value wasn't found. The
 * returned string will be urldecoded already.
 *
 */
static int at_web_find_arg(char *line, char *arg, char *buff, int buffLen)
{
    char *p,  *e;

    if (line == NULL) {
        return -1;
    }

    p = line;

    while (p != NULL && *p != '\n' && *p != '\r' && *p != 0) {
        if (strncmp(p, arg, strlen(arg)) == 0 && p[strlen(arg)] == '=') {
            p += strlen(arg) + 1; // move p to start of value
            e = (char*) strstr(p, "&");

            if (e == NULL) {
                e = p + strlen(p);
            }

            return at_web_url_decode(p, (e - p), buff, buffLen);
        }

        p = (char*) strstr(p, "&");

        if (p != NULL) {
            p += 1;
        }
    }

    ESP_LOGD(TAG, "Finding %s in %s: Not found :/\n", arg, line);
    return -1; // not found
}

static esp_err_t index_html_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    /* Add file upload form and script which on execution sends a POST request to /upload */
	memset(p_html_buf, 0, AT_WEB_PAGE_BUFSIZE);
	at_read_wifi_info();
	snprintf(p_html_buf, AT_WEB_PAGE_BUFSIZE, html_page, p_wifi_cfg->ssid, p_wifi_cfg->password);
    httpd_resp_send_chunk(req, (const char*) p_html_buf, strlen(p_html_buf));
    /* Respond with an empty chunk to signal HTTP response completion */
    return httpd_resp_send_chunk(req, NULL, 0);
}

/* Send HTTP response with the contents of the requested file */
static esp_err_t web_common_get_handler(httpd_req_t *req)
{
    return index_html_get_handler(req);
    return AT_OK;
}

/* A help function to get post request data */
static esp_err_t recv_post_data(httpd_req_t *req, char *buf)
{
    int total_len = req->content_len;
    int cur_len = 0;
    int received = 0;

    if (total_len >= AT_WEB_SCRATCH_BUFSIZE) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_500(req);
        ESP_LOGE(TAG, "context too long");
        return AT_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_500(req);
            ESP_LOGE(TAG, "Failed to post control value");
            return AT_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0'; // now ,the post is str format, like ssid=yuxin&pwd=TestPWD&chl=1&ecn=0&maxconn=1&ssidhidden=0
    ESP_LOGI(TAG, "Post data is : %s\n", buf);
    return AT_OK;
}

static void at_web_response_ok(httpd_req_t *req)
{
    const char *temp_str = "{\"state\": 0}";
    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    httpd_resp_set_status(req, HTTPD_200);

    httpd_resp_send(req, temp_str, strlen(temp_str));
}

static void at_web_response_error(httpd_req_t *req, const char *status)
{
    const char *temp_str = "{\"state\": 1}";
    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    httpd_resp_set_status(req, status);

    httpd_resp_send(req, temp_str, strlen(temp_str));
}

static int at_get_wifi_info_from_str(char *buffer, wifi_config_t *config)
{
    char *ssid = NULL;
	char *pwd = NULL;
	char *end = NULL;	

	if ((NULL==buffer) || (NULL==config))
	{
	    return AT_FAIL;
	}

    printf("wifi info %s\r\n", buffer);

#if 1
    at_web_find_arg(buffer, "ssid", config->ssid, AT_WEB_MAX_SSID_SIZE);	
    at_web_find_arg(buffer, "password", config->password, AT_WEB_MAX_PWD_SIZE);	
#else	
	ssid = strstr(buffer, "ssid=");
	pwd = strstr(buffer, "password=");
    end = strstr(buffer, "&");
	//*end = '\0';
    if (ssid) {
        snprintf(config->ssid, end-ssid-5+1, "%s", ssid+5);
	}
	
	if (pwd) {
        snprintf(config->password, strlen(buffer)-(pwd-buffer)-9+1, "%s", pwd+9);
	}
#endif	
    printf("ssid %s password %s\r\n", config->ssid, config->password);
    return AT_OK;
}


int at_read_wifi_info(void)
{
    int file;

	if (NULL == p_wifi_cfg)
	{
	    return AT_FAIL;
	}
	
    file = open(AT_LFS_WIFI_FILE, O_RDONLY, 0);
    if (file == -1)
    {
        printf("Error opening file:%s.\n",AT_LFS_WIFI_FILE);
        return AT_FAIL;
    }
    printf("open %s for read\r\n", AT_LFS_WIFI_FILE);
    char *buf = (char*)malloc(AT_WEB_MAX_WIFI_CFG_SIZE);
    if(buf==NULL)
    {
        printf("ERROR: no enough memory\r\n");
        return AT_OK;
    }
    lseek(file, 0, SEEK_SET);

    int len_read = 0;
    len_read = read(file, buf, AT_WEB_MAX_WIFI_CFG_SIZE);
    if(len_read > 0)
    {
        buf[len_read] = '\0';
        printf("read %d %s\r\n", len_read, buf);
		at_get_wifi_info_from_str(buf, p_wifi_cfg);
    }
    else if(len_read == 0)
    {
        printf("Fail to read from: %s, %d\r\n", AT_LFS_WIFI_FILE, len_read);
    }
    else
    {
        printf("Fail to read from: %s, %d\r\n", AT_LFS_WIFI_FILE, len_read);
    }

    free(buf);
    close(file);
	
    return AT_OK;
}

static int at_write_wifi_info(char *info, int len)
{
    char buffer[AT_WEB_MAX_WIFI_CFG_SIZE] = {0};
	
    if ((NULL==info) || (0==len))
    {
        return AT_FAIL;
    }

	if(is_fs_mounted() == 0)
    {
        printf("FS is not mounted, please mount FS first.\r\n");
        return AT_FAIL;
    }

    int file = open(AT_LFS_WIFI_FILE, O_RDWR | O_CREAT, 0);
    if (file == -1)
    {
        printf("Error opening/creating file:%s.\n", AT_LFS_WIFI_FILE);
        return AT_FAIL;
    }

    snprintf(buffer, AT_WEB_MAX_WIFI_CFG_SIZE, "%s&%s", info, len);
	
    lseek(file, 0, SEEK_SET);
    write(file, buffer, strlen(buffer));    
    close(file);

	return 0;
}

static int AT_WebServer_Response(char *ssid, char *pwd)
{
	char buffer[AT_WEB_MAX_WIFI_CFG_SIZE]={0};
	QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;

    if ((NULL == ssid) || (NULL == pwd))
    {
        return QAT_RC_ERROR;
    }

	snprintf(buffer, AT_WEB_MAX_WIFI_CFG_SIZE, "+EVT:CWCFG:%s,%s", ssid, pwd);
	rc = QAT_Response_Str(QAT_RC_OK, buffer);
	return rc;
}

static esp_err_t config_wifi_post_handler(httpd_req_t *req)
{
    char *buf = ((web_server_context_t*)(req->user_ctx))->scratch;
    int str_len = 0;
    int32_t udp_port = -1;
    char temp_str[32] = {0};
    bool ssid_is_null = false;
    memset(buf, '\0', AT_WEB_SCRATCH_BUFSIZE * sizeof(char));
    // only wifi config not start or have success apply one connection,allow to apply new connect
    if (1) {
        if (recv_post_data(req, buf) != AT_OK) {
            at_web_response_error(req, HTTPD_500);
            ESP_LOGE(TAG, "recv post data error");
            goto error_handle;
        }

        if (at_get_wifi_info_from_str(buf, p_wifi_cfg) != AT_OK) {
            ESP_LOGE(TAG, "failed to parse wifi info, json str: %s", buf);
            goto error_handle;
        }
		at_write_wifi_info(buf, strlen(buf));
		AT_WebServer_Response(p_wifi_cfg->ssid, p_wifi_cfg->password);
        web_common_get_handler(req);
        return AT_OK;
    }
error_handle:
    at_web_response_error(req, HTTPD_400);
    return AT_FAIL;
}

static esp_err_t at_web_version_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    char *temp_json_str = ((web_server_context_t*)(req->user_ctx))->scratch;
    snprintf(temp_json_str, strlen("\"message\":\"%s\"}"), "{\"version\":\"%s\"}", AT_WEB_VERSION);
    ESP_LOGD(TAG, "ready to send version: %s\n", temp_json_str);

    httpd_resp_send(req, temp_json_str, (temp_json_str == NULL) ? 0 : strlen(temp_json_str));

    return AT_OK;
}

static esp_err_t start_web_server(const char *base_path, uint16_t server_port)
{
    AT_WEB_SERVER_CHECK(base_path, "wrong base path", err);
    s_web_context = calloc(1, sizeof(web_server_context_t));
    AT_WEB_SERVER_CHECK(s_web_context, "No memory for rest context", err);
	p_wifi_cfg = &(s_web_context->wifi_cfg);
	p_html_buf = s_web_context->html_buf;
    strlcpy(s_web_context->base_path, base_path, sizeof(s_web_context->base_path));

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 8;
    config.max_open_sockets = 7; // It cannot be less than 7.
    config.server_port = server_port;

    ESP_LOGD(TAG, "Starting HTTP Server");
    AT_WEB_SERVER_CHECK(httpd_start(&s_server, &config) == AT_OK, "Start server failed", err_start);

	httpd_uri_t httpd_uri_array[] = {
		{"/", HTTP_GET, web_common_get_handler, s_web_context}, 	 
		{"/index.html", HTTP_GET, web_common_get_handler, s_web_context},
		{"/getversion", HTTP_GET, at_web_version_get_handler, s_web_context},
		{"/setwifi", HTTP_POST, config_wifi_post_handler, s_web_context},
	};

    for (int i = 0; i < sizeof(httpd_uri_array) / sizeof(httpd_uri_t); i++) {
        if (httpd_register_uri_handler(s_server, &httpd_uri_array[i]) != AT_OK) {
            ESP_LOGE(TAG, "httpd register uri_array[%d] fail", i);
        }
    }

    return AT_OK;
err_start:
    free(s_web_context);
	s_web_context = NULL;
	p_wifi_cfg = NULL;
	p_html_buf = NULL;
err:
    return AT_FAIL;
}

static int stop_web_server(void)
{
    AT_WEB_SERVER_CHECK(httpd_stop(s_server) == AT_OK, "Stop server failed", err);
    free(s_web_context);
    s_web_context = NULL;	
	p_wifi_cfg = NULL;
	p_html_buf = NULL;
    s_server = NULL;
    ESP_LOGI(TAG, "Stop HTTP Server");

    return AT_OK;
err:
    return AT_FAIL;
}


int at_web_start(uint16_t server_port)
{
    esp_err_t err;

    if (s_server == NULL) {
        /*AT web can use fatfs to storge html or use embeded file to storge html.If use fatfs, we should enable AT FS Command support*/
#ifdef CONFIG_FILE_SYSTEM
        err = init_fs();
        if (err != AT_OK) {
            return err;
        }
#endif
        err = start_web_server(AT_WEB_MOUNT_POINT, server_port);
        if (err != AT_OK) {
            return err;
        }
    }

    return AT_OK;
}

int at_web_stop(void)
{
    esp_err_t err;

    if (s_server != NULL) {
        if ((err = stop_web_server()) != ESP_OK) {
            return err;
        }
    }

    return ESP_OK;
}

#endif
