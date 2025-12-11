/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*                                                                                                                           */
/*       Firmware Upgrade */
/*                                                                                                                           */
/*****************************************************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qapi_types.h"
#include "qapi_status.h"
#include "qurt_internal.h"
#include "qurt_mutex.h"
#include "fw_upgrade.h"
#include "assert.h"
#include "nt_sys_monitoring.h"
#include "safeAPI.h"

/**********************************************************************************************************/
/* Preprocessor Definitions and Constants																  */
/**********************************************************************************************************/

#define TAKE_LOCK(__lock__) ((qurt_mutex_lock_timed(&(__lock__), QURT_TIME_WAIT_FOREVER)) == QURT_EOK)
#define RELEASE_LOCK(__lock__)          \
    do {                                \
        qurt_mutex_unlock(&(__lock__)); \
    } while (0)

#if defined(DEBUG_FW_UPGRADE_PRINTF)
#define FW_UPGRADE_D_PRINTF(args...) printf(args)
#else
#define FW_UPGRADE_D_PRINTF(args...)
#endif

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#define FW_UPGRADE_FLAG_AUTO_REBOOT             (1 << 0)
#define FW_UPGRADE_FLAG_DUPLICATE_ACTIVE_FS     (1 << 1)
#define FW_UPGRADE_FLAG_RANGE_HEADER            (1 << 2)

#define UNUSED(x) (void)(x)

/**********************************************************************************************************/
/* Globals																                                  */
/**********************************************************************************************************/
fw_upgrade_context_t *fw_upgrade_sess_cxt = NULL;
fw_upgrade_image_hdr_t *fw_upgrade_image_hdr = NULL; /* fw upgrade image header */
uint8_t fw_upgrade_mutex_init = 0;
qurt_mutex_t fw_upgrade_mutex;
/**********************************************************************************************************/
/* External Functions																                      */
/**********************************************************************************************************/
#ifdef CONFIG_QAT_OTA_DEMO
extern void qat_common_rst_timer_callback();
#endif
/**********************************************************************************************************/
/* Internal Functions																                      */
/**********************************************************************************************************/
/*
 * get fw upgrade session context
 */
static fw_upgrade_context_t *fw_upgrade_get_context(void)
{
    return fw_upgrade_sess_cxt;
}

/*
 * set fw upgrade session state
 */
static void fw_upgrade_set_state(fw_upgrade_state_t state)
{
    fw_upgrade_context_t *fw_upgrade_cxt;

    if (TAKE_LOCK(fw_upgrade_mutex)) {
        /* get fw upgrade session context */
        fw_upgrade_cxt = fw_upgrade_get_context();

        if (fw_upgrade_cxt == NULL) {
            // TODO: dump error info
            assert(0);
        }

        if (fw_upgrade_cxt != NULL) {
            fw_upgrade_cxt->fw_upgrade_state = state;
        }
        RELEASE_LOCK(fw_upgrade_mutex);
    }
}

/*
 * get fw upgrade session state
 */
static fw_upgrade_state_t fw_upgrade_get_state(void)
{
    fw_upgrade_state_t ret = FW_UPGRADE_STATE_NOT_START_E;
    fw_upgrade_context_t *fw_upgrade_cxt;

    if (fw_upgrade_mutex_init == 0)
        return ret;

    if (TAKE_LOCK(fw_upgrade_mutex)) {
        /* get fw upgrade session context */
        fw_upgrade_cxt = fw_upgrade_get_context();
        if (fw_upgrade_cxt == NULL) {
            ret = FW_UPGRADE_STATE_NOT_START_E;
        } else {
            ret = fw_upgrade_cxt->fw_upgrade_state;
        }
        RELEASE_LOCK(fw_upgrade_mutex);
    }
    return ret;
}

/*
 * get fw upgrade session active status
 */
static fw_upgrade_session_status_t fw_upgrade_get_session_status(void)
{
    fw_upgrade_session_status_t ret = FW_UPGRADE_SESSION_NOT_START_E;
    fw_upgrade_context_t *fw_upgrade_cxt;

    if (fw_upgrade_mutex_init == 0) {
        return ret;
    }

    if (TAKE_LOCK(fw_upgrade_mutex)) {
        /* get fw upgrade session context */
        fw_upgrade_cxt = fw_upgrade_get_context();
        if (fw_upgrade_cxt == NULL) {
            ret = FW_UPGRADE_SESSION_NOT_START_E;
        } else {
            ret = fw_upgrade_cxt->fw_upgrade_session_status;
        }
        RELEASE_LOCK(fw_upgrade_mutex);
    }

    return ret;
}

/*
 * set fw upgrade session active status
 */
static fw_upgrade_status_code_t fw_upgrade_set_session_status(fw_upgrade_session_status_t status)
{
    fw_upgrade_status_code_t ret = FW_UPGRADE_ERR_SESSION_NOT_START_E;
    fw_upgrade_context_t *fw_upgrade_cxt;

    if (fw_upgrade_mutex_init == 0) {
        return ret;
    }

    if (TAKE_LOCK(fw_upgrade_mutex)) {
        /* get fw upgrade session context */
        fw_upgrade_cxt = fw_upgrade_get_context();
        if (fw_upgrade_cxt == NULL) {
            ret = FW_UPGRADE_ERR_SESSION_NOT_START_E;
        } else {
            fw_upgrade_cxt->fw_upgrade_session_status = status;
            ret = FW_UPGRADE_OK_E;
        }
        RELEASE_LOCK(fw_upgrade_mutex);
    }
    return ret;
}

/*
 * set fw upgrade session error code
 */
static void fw_upgrade_set_error_code(int32_t err_code)
{
    fw_upgrade_context_t *fw_upgrade_cxt = fw_upgrade_get_context();

    if (fw_upgrade_cxt != NULL) {
        fw_upgrade_cxt->error_code = err_code;
    }
}

/*
 * get fw upgrade session error code
 */
static int32_t fw_upgrade_get_error_code(void)
{
    fw_upgrade_context_t *fw_upgrade_cxt = fw_upgrade_get_context();

    if (fw_upgrade_cxt != NULL) {
        return fw_upgrade_cxt->error_code;
    }
    return FW_UPGRADE_ERR_SESSION_NOT_START_E;
}

/*
 * call fw upgrade callback
 */
static void fw_upgrade_update_callback(int32_t state, int32_t status)
{
    fw_upgrade_context_t *fw_upgrade_cxt = fw_upgrade_get_context();

    if ((fw_upgrade_cxt != NULL) && (fw_upgrade_cxt->fw_upgrade_cb != NULL)) {
        fw_upgrade_cxt->fw_upgrade_cb(state, status);
    }
}

/*
 * call fw upgrade plugin's init callback
 */
static int32_t fw_upgrade_plugin_init(void)
{
    fw_upgrade_context_t *fw_upgrade_cxt;
    fw_upgrade_image_hdr_t *img_hdr;
    char *combined_url = NULL;
    uint32_t url_len = 0;
    int32_t ret;

    fw_upgrade_cxt = fw_upgrade_get_context();
    if (fw_upgrade_cxt == NULL) {
        return FW_UPGRADE_ERR_SESSION_NOT_START_E;
    }

    fw_upgrade_cxt->file_read_count = 0;

    combined_url = (char *)malloc(FW_UPGRADE_URL_TOTAL_LEN);
    if (!combined_url) {
        FW_UPGRADE_D_PRINTF("Out of memory error\r\n");
        return FW_UPGRADE_ERR_INSUFFICIENT_MEMORY_E;
    }
    memset(combined_url, '\0', FW_UPGRADE_URL_TOTAL_LEN);
    url_len = strlen(fw_upgrade_cxt->url);
    memscpy(combined_url, url_len, fw_upgrade_cxt->url, url_len);

    if (fw_upgrade_cxt->is_first != 0) {
        memscpy(&combined_url[url_len], strlen(fw_upgrade_cxt->cfg_file), fw_upgrade_cxt->cfg_file,
                strlen(fw_upgrade_cxt->cfg_file));
    } else {
        img_hdr = fw_upgrade_image_hdr;
        img_hdr += fw_upgrade_cxt->image_index;
        memscpy(&combined_url[url_len], strlen((char *)img_hdr->image_file), img_hdr->image_file,
                strlen((char *)img_hdr->image_file));
    }

    ret = (int32_t)fw_upgrade_cxt->plugin.fw_upgrade_plugin_init(
        fw_upgrade_cxt->interface_name, (const char *)combined_url, fw_upgrade_cxt->init_param);
    free(combined_url);
    return ret;
}

/*
 * call fw upgrade plugin's Recev_Data callback
 */
static int32_t fw_upgrade_plugin_recv_data(uint8_t *buffer, uint32_t buf_len, uint32_t *ret_size, void *init_param)
{
    fw_upgrade_context_t *fw_upgrade_cxt;
    int32_t ret;

    fw_upgrade_cxt = fw_upgrade_get_context();
    if (fw_upgrade_cxt == NULL)
        return FW_UPGRADE_ERR_SESSION_NOT_START_E;

    if (fw_upgrade_cxt->flags & FW_UPGRADE_FLAG_RANGE_HEADER) {
        ret = (int32_t)fw_upgrade_cxt->plugin.fw_upgrade_plugin_recv_data(buffer, buf_len, ret_size, &fw_upgrade_cxt->flags);
    } else{
        ret = (int32_t)fw_upgrade_cxt->plugin.fw_upgrade_plugin_recv_data(buffer, buf_len, ret_size, init_param);
    }

    return ret;
}

/*
 * call fw upgrade plugin's Fin callback
 */
static int32_t fw_upgrade_plugin_fin(void)
{
    fw_upgrade_context_t *fw_upgrade_cxt;
    int32_t ret;

    fw_upgrade_cxt = fw_upgrade_get_context();
    if (fw_upgrade_cxt == NULL)
        return FW_UPGRADE_ERR_SESSION_NOT_START_E;

    ret = (int32_t)fw_upgrade_cxt->plugin.fw_upgrade_plugin_fin();
    return ret;
}

/*
 * call fw upgrade plugin's abort callback
 */
static int32_t fw_upgrade_plugin_abort(void)
{
    fw_upgrade_context_t *fw_upgrade_cxt;
    int32_t ret;

    fw_upgrade_cxt = fw_upgrade_get_context();
    if (fw_upgrade_cxt == NULL)
        return FW_UPGRADE_ERR_SESSION_NOT_START_E;

    ret = (int32_t)fw_upgrade_cxt->plugin.fw_upgrade_plugin_abort();
    return ret;
}

/*
 * call fw upgrade plugin's resume callback
 */
static int32_t fw_upgrade_plugin_resume(void)
{
    fw_upgrade_context_t *fw_upgrade_cxt;
    fw_upgrade_image_hdr_t *img_hdr;
    char *combined_url = NULL;
    uint32_t url_len = 0;
    int32_t ret;

    fw_upgrade_cxt = fw_upgrade_get_context();
    if (fw_upgrade_cxt == NULL) {
        return FW_UPGRADE_ERR_SESSION_NOT_START_E;
    }

    combined_url = (char *)malloc(FW_UPGRADE_URL_TOTAL_LEN);
    if (!combined_url) {
        FW_UPGRADE_D_PRINTF("Out of memory error\r\n");
        return FW_UPGRADE_ERR_INSUFFICIENT_MEMORY_E;
    }
    memset(combined_url, 0, FW_UPGRADE_URL_TOTAL_LEN);
    url_len = strlen(fw_upgrade_cxt->url);
    memscpy(combined_url, url_len, fw_upgrade_cxt->url, url_len);

    if (fw_upgrade_cxt->format != FW_UPGRADE_FORAMT_PARTIAL_UPGRADE) { /* all-in-one fw upgrade */
        memscpy(&combined_url[url_len], strlen(fw_upgrade_cxt->cfg_file), fw_upgrade_cxt->cfg_file,
                strlen(fw_upgrade_cxt->cfg_file));
    } else {
        if (fw_upgrade_cxt->is_first != 0) {
            memscpy(&combined_url[url_len], strlen(fw_upgrade_cxt->cfg_file), fw_upgrade_cxt->cfg_file,
                    strlen(fw_upgrade_cxt->cfg_file));
        } else {
            img_hdr = fw_upgrade_image_hdr;
            img_hdr += fw_upgrade_cxt->image_index;
            memscpy(&combined_url[url_len], strlen((char *)img_hdr->image_file), img_hdr->image_file,
                    strlen((char *)img_hdr->image_file));
        }
    }
    ret = (int32_t)fw_upgrade_cxt->plugin.fw_upgrade_plugin_resume(
        fw_upgrade_cxt->interface_name, (const char *)combined_url, fw_upgrade_cxt->file_read_count);
    free(combined_url);
    return ret;
}

/*
 * fw upgrade session fin
 */
static fw_upgrade_status_code_t fw_upgrade_session_fin(void)
{
    if (fw_upgrade_sess_cxt != NULL && fw_upgrade_sess_cxt->config_buf != NULL) {
        free(fw_upgrade_sess_cxt->config_buf);
        fw_upgrade_sess_cxt->config_buf = NULL;
    }

    // TODO: free AON MEM
    if (fw_upgrade_image_hdr != NULL) {
        free(fw_upgrade_image_hdr);
        fw_upgrade_image_hdr = NULL;
    }
    if (fw_upgrade_sess_cxt != NULL) {
        if (fw_upgrade_sess_cxt->partition_hdl != NULL) {
            ((fu_partition_client_t *)(fw_upgrade_sess_cxt->partition_hdl))->ref_count = 0;
            fw_upgrade_sess_cxt->partition_hdl = NULL;
        }
        if (fw_upgrade_sess_cxt->digest_ctx != NULL) {
            free((void *)(fw_upgrade_sess_cxt->digest_ctx));
            fw_upgrade_sess_cxt->digest_ctx = NULL;
        }
        free(fw_upgrade_sess_cxt);
        fw_upgrade_sess_cxt = NULL;
    }

    if (fw_upgrade_mutex_init != 0) {
        qurt_mutex_delete(&fw_upgrade_mutex);
        fw_upgrade_mutex_init = 0;
    }

    return FW_UPGRADE_OK_E;
}

/*
 * fw upgrade session init
 */
static fw_upgrade_status_code_t fw_upgrade_session_init(char *interface_name, fw_upgrade_plugin_t *plugin, char *url,
                                                        char *cfg_file, uint32_t flags,
                                                        fw_upgrade_cb_t fw_upgrade_callback, void *init_param)
{
    fw_upgrade_status_code_t ret = FW_UPGRADE_OK_E;
    uint32_t url_len = 0, filename_len = 0;
    uint32_t i;

    if (fw_upgrade_init() != FW_UPGRADE_OK_E) {
        ret = FW_UPGRADE_ERR_FLASH_INIT_TIMEOUT_E;
        goto session_init_end;
    }

    if (fw_upgrade_mutex_init == 0) {
        qurt_mutex_create(&fw_upgrade_mutex);
        fw_upgrade_mutex_init = 1;
    }

    // TODO: aon malloc
    fw_upgrade_sess_cxt = (fw_upgrade_context_t *)malloc(sizeof(fw_upgrade_context_t));
    if (fw_upgrade_sess_cxt == NULL) {
        ret = FW_UPGRADE_ERR_INSUFFICIENT_MEMORY_E;
        goto session_init_end;
    }

    /* Clear all data */
    memset(fw_upgrade_sess_cxt, '\0', sizeof(fw_upgrade_context_t));

    /* init the default setting */
    fw_upgrade_set_state(FW_UPGRADE_STATE_NOT_START_E);
    fw_upgrade_set_session_status(FW_UPGRADE_SESSION_RUNNING_E);
    fw_upgrade_set_error_code(FW_UPGRADE_OK_E);
    fw_upgrade_sess_cxt->partition_hdl = NULL;
    fw_upgrade_sess_cxt->flags = flags;
    fw_upgrade_sess_cxt->fw_upgrade_cb = (fw_upgrade_cb_t)fw_upgrade_callback;
    fw_upgrade_sess_cxt->init_param = init_param;
    memscpy((void *)&(fw_upgrade_sess_cxt->plugin), sizeof(fw_upgrade_plugin_t), (void *)plugin,
            sizeof(fw_upgrade_plugin_t));
    if ((fw_upgrade_sess_cxt->plugin.fw_upgrade_plugin_init == NULL) ||
        (fw_upgrade_sess_cxt->plugin.fw_upgrade_plugin_recv_data == NULL) ||
        (fw_upgrade_sess_cxt->plugin.fw_upgrade_plugin_abort == NULL) ||
        (fw_upgrade_sess_cxt->plugin.fw_upgrade_plugin_resume == NULL) ||
        (fw_upgrade_sess_cxt->plugin.fw_upgrade_plugin_fin == NULL)) {
        ret = FW_UPGRADE_ERR_PLUGIN_ENTRY_EMPTY_E;
        goto session_init_end;
    }

    /* save interface name */
    if (strlen(interface_name) >= FW_UPGRADE_INTERFACE_NAME_LEN) {
        ret = FW_UPGRADE_ERR_INTERFACE_NAME_TOO_LONG_E;
        goto session_init_end;
    }
    memset(fw_upgrade_sess_cxt->interface_name, 0, FW_UPGRADE_INTERFACE_NAME_LEN);
    memscpy(fw_upgrade_sess_cxt->interface_name, (strlen(interface_name) + 1), interface_name,
            (strlen(interface_name) + 1));

    url_len = strlen(url);
    filename_len = strlen(cfg_file);

    /* set url buffer to 0 */
    memset(fw_upgrade_sess_cxt->url, 0, FW_UPGRADE_URL_LEN);
    /* Add '/' to URL if it is not already added and save the URL. */
    if (url_len > 0) {
        if (url[url_len - 1] != '/') {
            if (url_len + 1 >= FW_UPGRADE_URL_LEN) {
                ret = FW_UPGRADE_ERR_URL_TOO_LONG_E;
                goto session_init_end;
            }
            memscpy(fw_upgrade_sess_cxt->url, url_len, url, url_len);
            fw_upgrade_sess_cxt->url[url_len] = '/';
        } else {
            if (url_len >= FW_UPGRADE_URL_LEN) {
                ret = FW_UPGRADE_ERR_URL_TOO_LONG_E;
                goto session_init_end;
            }
            memscpy(fw_upgrade_sess_cxt->url, url_len, url, url_len);
        }
    }

    /* Save the config file name */
    if (filename_len >= FW_UPGRADE_FILENAME_LEN) {
        ret = FW_UPGRADE_ERR_URL_TOO_LONG_E;
        goto session_init_end;
    }
    memset(fw_upgrade_sess_cxt->cfg_file, 0, FW_UPGRADE_FILENAME_LEN);
    memscpy(fw_upgrade_sess_cxt->cfg_file, strlen(cfg_file), cfg_file, strlen(cfg_file));

    fw_upgrade_sess_cxt->is_first = 1;
    fw_upgrade_sess_cxt->format = 1; /* 1: partial upgrade, 2: all-in-one */
    fw_upgrade_sess_cxt->buf_len = 0;
    fw_upgrade_sess_cxt->buf_offset = 0;
    fw_upgrade_sess_cxt->file_read_count = 0;

    fw_upgrade_sess_cxt->image_index = 0;
    fw_upgrade_sess_cxt->image_wrt_count = 0;
    fw_upgrade_sess_cxt->image_wrt_length = 0;
    fw_upgrade_sess_cxt->total_images = 0;
    fw_upgrade_sess_cxt->hidden_images_count = 0;
    fw_upgrade_sess_cxt->config_buf = NULL;

    for (i = 0; i < FW_UPGRADE_MAX_IMAGES_NUM; i++) {
        fw_upgrade_sess_cxt->download_flag[i] = 1;
    }

    /* allocate crypto resource */
    fw_upgrade_sess_cxt->digest_ctx = NULL;
    fw_upgrade_sess_cxt->digest_ctx = (mbedtls_sha256_context *)malloc(sizeof(mbedtls_sha256_context));
    if (fw_upgrade_sess_cxt->digest_ctx == NULL) {
        ret = FW_UPGRADE_ERR_INSUFFICIENT_MEMORY_E;
        goto session_init_end;
    }
    mbedtls_sha256_init(fw_upgrade_sess_cxt->digest_ctx);
    mbedtls_sha256_starts(fw_upgrade_sess_cxt->digest_ctx, 0);

    fw_upgrade_set_session_status(FW_UPGRADE_SESSION_RUNNING_E);
    return FW_UPGRADE_OK_E;

session_init_end:
    fw_upgrade_set_error_code(ret);
    fw_upgrade_session_fin();
    return ret;
}

/*
 * fw upgrade session suspend
 */
static fw_upgrade_status_code_t fw_upgrade_session_prepare_suspend(void)
{
    if (fw_upgrade_sess_cxt) {
        if (fw_upgrade_sess_cxt->partition_hdl != NULL) {
            ((fu_partition_client_t *)(fw_upgrade_sess_cxt->partition_hdl))->ref_count = 0;
            fw_upgrade_sess_cxt->partition_hdl = NULL;
        }

        // free crypto
        if (fw_upgrade_sess_cxt->digest_ctx != NULL) {
            free((void *)(fw_upgrade_sess_cxt->digest_ctx));
            fw_upgrade_sess_cxt->digest_ctx = NULL;
        }
    }
    return FW_UPGRADE_OK_E;
}

/*
 * process fw upgrade conifg file
 *
 * Partial Upgrade Flow:
 *     receive whole config file
 *     check if fields are valid at config file header
 *     calc hash of config file and compare the result with hash field at config file
 *     save image entries
 *     check if fields are valid at each image entry
 *     calc hash at current image and determine if the image need to be downloaded.
 *     image will only be downloaded if the hash of the new image is different from the hash of the current image.
 *
 * All-in-one Upgrade Flow:
 *     receive imageset header
 *     check if fields are valid at config file header
 *     calc hash of config file and compare the result with hash field at header
 *     save image entries
 *     check if fields are valid at each image entry
 *
 * Firmware Upgrade Image HEADER format:
      uint32 sig
      uint32 ver
      uint32 format
      uint32 image_len
      uint8  num_images
      IMG_ENTRY
          ....
      IMG_ENTRY
      uint8 HASH[32]
*/
static fw_upgrade_status_code_t fw_upgrade_process_config_file(uint8_t *buf)
{
    fw_upgrade_status_code_t ret = FW_UPGRADE_OK_E;
    uint32_t i, len, offset, block_size, disk_size, nbytes;
    uint8_t hash[FW_UPGRADE_HASH_LEN], *hash_buf = NULL;
    uint8_t active_fwd;
    fw_upgrade_imageSet_hdr_part1_t *imgset_hdr;
    fw_upgrade_context_t *fw_upgrade_cxt;
    fw_upgrade_image_hdr_t *img_hdr;
    fu_part_hdl_t hdl;
    uint8_t totalImages;
    boolean sbl_image_exist = FALSE;
    boolean app_image_exist = FALSE;
    uint32_t sbl_disk_size = 0;

    fw_upgrade_cxt = fw_upgrade_get_context();
    if (fw_upgrade_cxt == NULL) {
        return FW_UPGRADE_ERR_SESSION_NOT_START_E;
    }

    // received first buffer for config file
    if (fw_upgrade_cxt->config_buf == NULL) {
        if (fw_upgrade_cxt->buf_len < sizeof(fw_upgrade_imageSet_hdr_part1_t)) {
            ret = FW_UPGRADE_ERR_INCORRECT_IMAGE_HDR_E;
            goto parse_img_hdr_end;
        }

        // get firmware upgrade image header part1
        imgset_hdr = (fw_upgrade_imageSet_hdr_part1_t *)buf;

        // check total images
        if (!(imgset_hdr->num_images > 0 && imgset_hdr->num_images <= FW_UPGRADE_MAX_IMAGES_NUM)) {
            FW_UPGRADE_D_PRINTF("num of firmware upgrade images are not correct\r\n");
            ret = FW_UPGRADE_ERR_INCORRECT_IMAGE_HDR_E;
            goto parse_img_hdr_end;
        }
        // check header
        if ((imgset_hdr->magic == 0) || (imgset_hdr->length == 0) || (imgset_hdr->format == 0)) {
            FW_UPGRADE_D_PRINTF("firmware upgrade image signature is not correct\r\n");
            ret = FW_UPGRADE_ERR_INCORRECT_IMAGE_HDR_E;
            goto parse_img_hdr_end;
        }

        // save fw_upgrade format -- partial upgrade or all in one
        fw_upgrade_cxt->format = imgset_hdr->format;

        len = imgset_hdr->length;
        if (len != (sizeof(fw_upgrade_imageSet_hdr_part1_t) + imgset_hdr->num_images * sizeof(fw_upgrade_image_hdr_t) +
                    FW_UPGRADE_HASH_LEN)) {
            FW_UPGRADE_D_PRINTF("hdr length is not correct\r\n");
            ret = FW_UPGRADE_ERR_INCORRECT_IMAGE_HDR_E;
            goto parse_img_hdr_end;
        }

        fw_upgrade_cxt->config_buf = (uint8_t *)malloc(len);
        if (fw_upgrade_cxt->config_buf == NULL) {
            ret = FW_UPGRADE_ERR_INSUFFICIENT_MEMORY_E;
            goto parse_img_hdr_end;
        }

        fw_upgrade_cxt->buf_offset = MIN(fw_upgrade_cxt->buf_len, len);
        memscpy(fw_upgrade_cxt->config_buf, fw_upgrade_cxt->buf_offset, buf, fw_upgrade_cxt->buf_offset);

        // don't receive whole config file at this packet yet
        if (fw_upgrade_cxt->buf_len < len) {
            fw_upgrade_cxt->image_wrt_count = fw_upgrade_cxt->buf_len;
            // continue receiving data
            fw_upgrade_set_state(FW_UPGRADE_STATE_RECEIVE_DATA_E);
            goto parse_img_hdr_end;
        }
    } else {
        imgset_hdr = (fw_upgrade_imageSet_hdr_part1_t *)fw_upgrade_cxt->config_buf;
        len = MIN(fw_upgrade_cxt->buf_len, imgset_hdr->length - fw_upgrade_cxt->image_wrt_count);
        memscpy(fw_upgrade_cxt->config_buf + fw_upgrade_cxt->image_wrt_count, len, buf, len);
        fw_upgrade_cxt->image_wrt_count += len;
        fw_upgrade_cxt->buf_offset = len;
        if (fw_upgrade_cxt->image_wrt_count < imgset_hdr->length) {
            // continue receiving data
            fw_upgrade_set_state(FW_UPGRADE_STATE_RECEIVE_DATA_E);
            goto parse_img_hdr_end;
        }
    }

    // get firmware upgrade image header part1
    imgset_hdr = (fw_upgrade_imageSet_hdr_part1_t *)fw_upgrade_cxt->config_buf;

    /* calc hash offset */
    offset = sizeof(fw_upgrade_imageSet_hdr_part1_t) + imgset_hdr->num_images * sizeof(fw_upgrade_image_hdr_t);

    // init crypto
    mbedtls_sha256_init(fw_upgrade_cxt->digest_ctx);
    mbedtls_sha256_starts(fw_upgrade_cxt->digest_ctx, 0);
    mbedtls_sha256_update(fw_upgrade_cxt->digest_ctx, (unsigned char *)imgset_hdr, offset);
    mbedtls_sha256_finish(fw_upgrade_cxt->digest_ctx, hash);

    // compare firmware upgrade image Header HASH
    if (memcmp(fw_upgrade_cxt->config_buf + offset, hash, FW_UPGRADE_HASH_LEN) != 0) {
        FW_UPGRADE_D_PRINTF("HASH is incorrect\r\n");
        ret = FW_UPGRADE_ERR_INCORRECT_IMAGE_CHECKSUM_E;
        goto parse_img_hdr_end;
    }

    // write magic number
    if (fw_upgrade_set_fwd_info(fw_upgrade_cxt->trial_fwd_idx, FW_UPGRADE_FWD_MAGIC_E, (uint8_t *)&imgset_hdr->magic) !=
        FW_UPGRADE_OK_E) {
        ret = FW_UPGRADE_ERR_FLASH_WRITE_FAIL_E;
        goto parse_img_hdr_end;
    }

    // write version
    if (fw_upgrade_set_fwd_info(fw_upgrade_cxt->trial_fwd_idx, FW_UPGRADE_FWD_VERSION_E,
                                (uint8_t *)&imgset_hdr->version) != FW_UPGRADE_OK_E) {
        ret = FW_UPGRADE_ERR_FLASH_WRITE_FAIL_E;
        goto parse_img_hdr_end;
    }

    // write num of images
    totalImages = imgset_hdr->num_images + fw_upgrade_cxt->hidden_images_count;
    if (fw_upgrade_set_fwd_info(fw_upgrade_cxt->trial_fwd_idx, FW_UPGRADE_FWD_TOTAL_IMAGE_E, (uint8_t *)&totalImages) !=
        FW_UPGRADE_OK_E) {
        ret = FW_UPGRADE_ERR_FLASH_WRITE_FAIL_E;
        goto parse_img_hdr_end;
    }

    // save total images
    fw_upgrade_cxt->total_images = imgset_hdr->num_images;

    /*Allocate buffer*/
    len = imgset_hdr->num_images * sizeof(fw_upgrade_image_hdr_t);

    // TODO: allocate aon mem
    fw_upgrade_image_hdr = malloc(len);
    if (fw_upgrade_image_hdr == NULL) {
        ret = FW_UPGRADE_ERR_INSUFFICIENT_MEMORY_E;
        goto parse_img_hdr_end;
    }

    /*save firmware upgrade image entries */
    memscpy((uint8_t *)(fw_upgrade_image_hdr), len,
            fw_upgrade_cxt->config_buf + sizeof(fw_upgrade_imageSet_hdr_part1_t), len);

    /* check image entries */
    for (i = 0, len = 0, disk_size = 0, img_hdr = fw_upgrade_image_hdr; i < imgset_hdr->num_images; i++) {
        // check image length
        if ((img_hdr->image_id == 0) || (img_hdr->magic == 0) || (img_hdr->hash_type == 0) ||
            (img_hdr->disk_size == 0) || (img_hdr->disk_size < img_hdr->image_length)) {
            FW_UPGRADE_D_PRINTF("firmware upgrade image length setting is not correct\r\n");
            ret = FW_UPGRADE_ERR_INCORRECT_IMAGE_HDR_E;
            goto parse_img_hdr_end;
        } else {
            if (img_hdr->image_id != SBL_IMG_ID) {
                /* adjust disk_size to align with block_size */
                fw_upgrade_get_mem_block_size(&block_size);
                if (img_hdr->disk_size % block_size != 0)
                    img_hdr->disk_size += block_size;
                img_hdr->disk_size = img_hdr->disk_size / block_size * block_size;
            }

            /* when calc the disk_size, exclude the first and second File system and SBL
               due to the File ssytem are pre-reserved already, SBL is located in specific
               place in RRAM.
            */
            if ((img_hdr->image_id != FS1_IMG_ID) && (img_hdr->image_id != FS2_IMG_ID) &&
                (img_hdr->image_id != SBL_IMG_ID)) {
                disk_size += img_hdr->disk_size;
                len += img_hdr->image_length;
            }
            if (img_hdr->image_id == SBL_IMG_ID) {
                sbl_image_exist = TRUE;
                sbl_disk_size = img_hdr->disk_size;
                if (img_hdr->image_length > img_hdr->disk_size) {
                    ret = FW_UPGRADE_ERR_INCORRECT_IMAGE_HDR_E;
                    goto parse_img_hdr_end;
                }
            }
            if (img_hdr->image_id == APP_IMG_ID) {
                app_image_exist = TRUE;
            }
        }

        // set pointer to next image header
        img_hdr++;
    }

    // SBL image must be upgraded together with APP image.
    if (sbl_image_exist && !(app_image_exist)) {
        ret = FW_UPGRADE_ERR_SBL_ONLY_NOT_SUPPORT_E;
        goto parse_img_hdr_end;
    }

    // check if memory has enough space to store images with disk_size
    if (disk_size > fw_upgrade_cxt->trial_mem_size) {
        ret = FW_UPGRADE_ERR_FLASH_NOT_ENOUGH_SPACE_E;
        goto parse_img_hdr_end;
    }

    if (len > disk_size) {
        ret = FW_UPGRADE_ERR_INCORRECT_IMAGE_HDR_E;
        goto parse_img_hdr_end;
    }

    if (fw_upgrade_select_sbl_trial_fde(&fw_upgrade_cxt->trial_sbl_idx, &fw_upgrade_cxt->trial_sbl_start,
                                        &fw_upgrade_cxt->trial_sbl_size) != FW_UPGRADE_OK_E) {
        ret = FW_UPGRADE_ERR_SBL_NOT_SUPPORT_UPGRADE_E;
        goto parse_img_hdr_end;
    }
    if (sbl_disk_size > fw_upgrade_cxt->trial_sbl_size) {
        ret = FW_UPGRADE_ERR_SBL_NOT_ENOUGH_SPACE_E;
        goto parse_img_hdr_end;
    }

    /* adjust file read count for all-in-one fw upgrade */
    fw_upgrade_cxt->file_read_count = imgset_hdr->length;

    /* free config buffer */
    if (fw_upgrade_cxt->config_buf != NULL) {
        free(fw_upgrade_cxt->config_buf);
        fw_upgrade_cxt->config_buf = NULL;
    }

    /* all-in-one fw upgrade case */
    if (fw_upgrade_cxt->format != FW_UPGRADE_FORAMT_PARTIAL_UPGRADE) {
        // imageset header is fully received and processed, move to next stage
        fw_upgrade_cxt->is_first = 0;
        if (fw_upgrade_cxt->buf_offset >= fw_upgrade_cxt->buf_len)
            fw_upgrade_set_state(FW_UPGRADE_STATE_RECEIVE_DATA_E);
        else
            fw_upgrade_set_state(FW_UPGRADE_STATE_PROCESS_IMAGE_E);
        goto parse_img_hdr_end;
    }

    /* this is for partial fw upgrade case */
    fw_upgrade_get_mem_block_size(&block_size);
    active_fwd = fw_upgrade_get_active_fwd(NULL, NULL);
    hash_buf = (uint8_t *)malloc(block_size);
    if (hash_buf == NULL) {
        ret = FW_UPGRADE_ERR_INSUFFICIENT_MEMORY_E;
        goto parse_img_hdr_end;
    }

    /* check images if need download */
    for (i = 0, img_hdr = fw_upgrade_image_hdr; i < fw_upgrade_cxt->total_images; i++, img_hdr++) {
        fw_upgrade_cxt->download_flag[i] = 1; /* default is to download */
        // TODO: SBL handle
        if ((img_hdr->image_id != FS1_IMG_ID) && (img_hdr->image_id != FS2_IMG_ID) &&
            (img_hdr->image_id != SBL_IMG_ID)) {
            if (fw_upgrade_find_partition(active_fwd, img_hdr->image_id, &hdl) != FW_UPGRADE_OK_E)
                continue;

            mbedtls_sha256_init(fw_upgrade_cxt->digest_ctx);
            mbedtls_sha256_starts(fw_upgrade_cxt->digest_ctx, 0);

            disk_size = ((fu_partition_client_t *)hdl)->img_size;

            for (offset = 0; offset < disk_size; offset += block_size) {
                fw_upgrade_read_partition(hdl, offset, (char *)hash_buf, block_size, &nbytes);
                mbedtls_sha256_update(fw_upgrade_cxt->digest_ctx, hash_buf, nbytes);
            }
            ((fu_partition_client_t *)hdl)->ref_count = 0;
            mbedtls_sha256_finish(fw_upgrade_cxt->digest_ctx, hash);
            // compare firmware upgrade image Header HASH
            if (memcmp(img_hdr->hash, hash, FW_UPGRADE_HASH_LEN) == 0) {
                fw_upgrade_cxt->download_flag[i] = 0;
            }
        }
    }

    if (hash_buf != NULL)
        free(hash_buf);

    // config file is fully received, move to next stage
    fw_upgrade_cxt->is_first = 0;
    fw_upgrade_set_state(FW_UPGRADE_STATE_DISCONNECT_SERVER_E);
    return ret;

parse_img_hdr_end:
    if ((ret != FW_UPGRADE_OK_E) && (fw_upgrade_cxt->config_buf != NULL)) {
        free(fw_upgrade_cxt->config_buf);
        fw_upgrade_cxt->config_buf = NULL;
    }
    if (hash_buf != NULL)
        free(hash_buf);
    return ret;
}

/*
 * verify image hash
 */
static fw_upgrade_status_code_t fw_upgrade_verify_image_hash(fw_upgrade_image_hdr_t *image_hdr)
{
    fw_upgrade_status_code_t ret = FW_UPGRADE_OK_E;
    fw_upgrade_context_t *fw_upgrade_cxt;
    uint8_t *hash_org, hash_result[FW_UPGRADE_HASH_LEN];
    uint8_t *hash_buf = NULL;
    uint32_t len;

    fw_upgrade_cxt = fw_upgrade_get_context();
    if (fw_upgrade_cxt == NULL) {
        return FW_UPGRADE_ERR_SESSION_NOT_START_E;
    }

    /* get org hash offset at image header */
    hash_org = (uint8_t *)image_hdr + sizeof(fw_upgrade_image_hdr_t) - FW_UPGRADE_HASH_LEN;

    if (fw_upgrade_cxt->format == FW_UPGRADE_FORAMT_PARTIAL_UPGRADE) {
        len = image_hdr->disk_size - image_hdr->image_length;
        if (len > 0) {
            hash_buf = (uint8_t *)malloc(len);
            if (hash_buf == NULL) {
                return FW_UPGRADE_ERR_INSUFFICIENT_MEMORY_E;
            }
            memset(hash_buf, 0xff, len);
            mbedtls_sha256_update(fw_upgrade_cxt->digest_ctx, hash_buf, len);
        }
    }

    /* get result */
    mbedtls_sha256_finish(fw_upgrade_cxt->digest_ctx, hash_result);

    if (hash_buf != NULL)
        free(hash_buf);

    /* compare fw upgrade image HASH */
    if (memcmp(hash_org, hash_result, FW_UPGRADE_HASH_LEN) != 0) {
        FW_UPGRADE_D_PRINTF("HASH is incorrect\r\n");
        ret = FW_UPGRADE_ERR_INCORRECT_IMAGE_CHECKSUM_E;
    }

    return ret;
}

/*
 * process firmware upgrade image
 */
static fw_upgrade_status_code_t fw_upgrade_process_receive_image(uint8_t *buffer)
{
    fw_upgrade_status_code_t ret = FW_UPGRADE_OK_E;
    fw_upgrade_context_t *fw_upgrade_cxt;
    fw_upgrade_image_hdr_t *img_hdr;
    uint32_t buf_len = 0, write_len, block_size;
    uint32_t startAddr;

    fw_upgrade_cxt = fw_upgrade_get_context();
    if (fw_upgrade_cxt == NULL)
        return FW_UPGRADE_ERR_SESSION_NOT_START_E;

    /* get block size */
    fw_upgrade_get_mem_block_size(&block_size);
    while (1) {
        /* for all-in-one fw upgrade case */
        if (fw_upgrade_cxt->format != FW_UPGRADE_FORAMT_PARTIAL_UPGRADE) {
            // all buffers have been processed
            if (fw_upgrade_cxt->buf_offset >= fw_upgrade_cxt->buf_len) {
                break;
            }

            // get available buf length
            buf_len = fw_upgrade_cxt->buf_len - fw_upgrade_cxt->buf_offset;
        }

        img_hdr = fw_upgrade_image_hdr;
        img_hdr += fw_upgrade_cxt->image_index;

        if (fw_upgrade_cxt->image_wrt_length == 0) {  // image entry not init
            fw_upgrade_cxt->image_wrt_count = 0;
            fw_upgrade_cxt->image_wrt_length = img_hdr->image_length;

            // create one image entry
            if (img_hdr->image_id == FS1_IMG_ID) {
                uint32_t disk_size, disk_start;
                if (fw_upgrade_find_partition(fw_upgrade_get_active_fwd(NULL, NULL), FS2_IMG_ID,
                                              &fw_upgrade_cxt->partition_hdl) != FW_UPGRADE_OK_E) {
                    ret = FW_UPGRADE_ERR_FLASH_IMAGE_NOT_FOUND_E;
                    break;
                }

                disk_size = ((fu_partition_client_t *)(fw_upgrade_cxt->partition_hdl))->img_size;
                disk_start = ((fu_partition_client_t *)(fw_upgrade_cxt->partition_hdl))->img_start;
                ((fu_partition_client_t *)(fw_upgrade_cxt->partition_hdl))->ref_count = 0;
                fw_upgrade_cxt->partition_hdl = NULL;

                // File system disk size is pre-set when first time download
                // Firmware Upgrade can't change size other than original size
                if (img_hdr->disk_size > disk_size) {
                    ret = FW_UPGRADE_ERR_INCORRECT_IMAGE_LENGTH_E;
                    break;
                }

                // create image for trial image's FS2
                if (fw_upgrade_create_partition(fw_upgrade_cxt->trial_fwd_idx, img_hdr->image_id, img_hdr->version,
                                                disk_start, disk_size,
                                                &fw_upgrade_cxt->partition_hdl) != FW_UPGRADE_OK_E) {
                    ret = FW_UPGRADE_ERR_FLASH_CREATE_PARTITION_E;
                    break;
                }
            } else {
                // TODO: special handling for SBL image
                if (img_hdr->image_id == SBL_IMG_ID) {
                    startAddr = fw_upgrade_cxt->trial_sbl_start;
                } else {
                    startAddr = fw_upgrade_cxt->trial_mem_start;
                }
                if (fw_upgrade_create_partition(fw_upgrade_cxt->trial_fwd_idx, img_hdr->image_id, img_hdr->version,
                                                startAddr, img_hdr->disk_size,
                                                &fw_upgrade_cxt->partition_hdl) != FW_UPGRADE_OK_E) {
                    ret = FW_UPGRADE_ERR_FLASH_CREATE_PARTITION_E;
                    break;
                }

                // erase first block
                if (fw_upgrade_erase_partition(fw_upgrade_cxt->partition_hdl, 0, block_size) != FW_UPGRADE_OK_E) {
                    ret = FW_UPGRADE_ERR_FLASH_ERASE_PARTITION_E;
                    break;
                }
            }

            // process the case of image length is 0x0 when using all-in-one fw upgrade
            if ((fw_upgrade_cxt->format != FW_UPGRADE_FORAMT_PARTIAL_UPGRADE) && (img_hdr->image_length == 0)) {
                // free partition handle
                ((fu_partition_client_t *)(fw_upgrade_cxt->partition_hdl))->ref_count = 0;
                fw_upgrade_cxt->partition_hdl = NULL;

                // FS1_IMG has its own start address and size
                if (img_hdr->image_id != FS1_IMG_ID && img_hdr->image_id != SBL_IMG_ID) {
                    // adjust memory start address for next entry
                    fw_upgrade_cxt->trial_mem_start += img_hdr->disk_size;
                }

                // still have data at buffer and move to next image entry
                fw_upgrade_cxt->image_index++;
                fw_upgrade_cxt->image_wrt_length = 0;

                // check if we have received all images
                if (fw_upgrade_cxt->image_index >= fw_upgrade_cxt->total_images) {
                    fw_upgrade_set_state(FW_UPGRADE_STATE_DUPLICATE_FS_E);
                    break;
                }

                continue;
            }

            // reset crypto engine
            mbedtls_sha256_init(fw_upgrade_cxt->digest_ctx);
            mbedtls_sha256_starts(fw_upgrade_cxt->digest_ctx, 0);
        }

        // set write_flash_len
        if (fw_upgrade_cxt->format == FW_UPGRADE_FORAMT_PARTIAL_UPGRADE) {
            write_len = fw_upgrade_cxt->buf_len;
            fw_upgrade_cxt->buf_offset = 0;
            if (write_len > (fw_upgrade_cxt->image_wrt_length - fw_upgrade_cxt->image_wrt_count)) {
                ret = FW_UPGRADE_ERR_INCORRECT_IMAGE_LENGTH_E;
                break;
            }
        } else {
            write_len = MIN(buf_len, (fw_upgrade_cxt->image_wrt_length - fw_upgrade_cxt->image_wrt_count));
        }

        // update firmware upgrade image HASH
        mbedtls_sha256_update(fw_upgrade_cxt->digest_ctx, &buffer[fw_upgrade_cxt->buf_offset], write_len);

        // check flash block if need erase first
        {
            uint32_t first_block, last_block;

            if ((fw_upgrade_cxt->image_wrt_count / block_size) !=
                ((fw_upgrade_cxt->image_wrt_count + write_len - 1) / block_size)) {
                first_block = fw_upgrade_cxt->image_wrt_count / block_size + 1;

                last_block = (fw_upgrade_cxt->image_wrt_count + write_len) / block_size;
                if (((fw_upgrade_cxt->image_wrt_count + write_len) % block_size) != 0)
                    last_block++;
                // erase blocks
                if (fw_upgrade_erase_partition(fw_upgrade_cxt->partition_hdl, first_block * block_size,
                                               (last_block - first_block) * block_size) != FW_UPGRADE_OK_E) {
                    ret = FW_UPGRADE_ERR_FLASH_ERASE_PARTITION_E;
                    break;
                }
            }
        }

        // write flash
        if (fw_upgrade_write_partition(fw_upgrade_cxt->partition_hdl, fw_upgrade_cxt->image_wrt_count,
                                       (char *)&buffer[fw_upgrade_cxt->buf_offset], write_len) != FW_UPGRADE_OK_E) {
            ret = FW_UPGRADE_ERR_FLASH_WRITE_PARTITION_E;
            break;
        }

        // update record
        fw_upgrade_cxt->buf_offset += write_len;
        fw_upgrade_cxt->file_read_count += write_len;
        fw_upgrade_cxt->image_wrt_count += write_len;

        // flash one image, move to next one
        if (fw_upgrade_cxt->image_wrt_count >= fw_upgrade_cxt->image_wrt_length) {
            // verify image HASH
            if ((ret = fw_upgrade_verify_image_hash(img_hdr)) != FW_UPGRADE_OK_E) {
                break;
            }

            // free partition handle
            ((fu_partition_client_t *)(fw_upgrade_cxt->partition_hdl))->ref_count = 0;
            fw_upgrade_cxt->partition_hdl = NULL;

            // FS1_IMG has its own start address and size
            if (img_hdr->image_id != FS1_IMG_ID && img_hdr->image_id != SBL_IMG_ID) {
                // adjust memory start address for next entry
                fw_upgrade_cxt->trial_mem_start += img_hdr->disk_size;
            }

            // still have data at buffer and move to next image entry
            fw_upgrade_cxt->image_index++;
            fw_upgrade_cxt->image_wrt_length = 0;

            if (fw_upgrade_cxt->format == FW_UPGRADE_FORAMT_PARTIAL_UPGRADE) {
                // move to next state
                fw_upgrade_set_state(FW_UPGRADE_STATE_DISCONNECT_SERVER_E);
            } else {
                // check if we have received all images
                if (fw_upgrade_cxt->image_index >= fw_upgrade_cxt->total_images) {
                    fw_upgrade_set_state(FW_UPGRADE_STATE_DUPLICATE_FS_E);
                    break;
                }
            }
        }

        // check if need erase block for next round
        if (((fw_upgrade_cxt->image_wrt_count % block_size) == 0) && (fw_upgrade_cxt->image_wrt_length > 0)) {
            // erase block
            if (fw_upgrade_erase_partition(fw_upgrade_cxt->partition_hdl, fw_upgrade_cxt->image_wrt_count,
                                           block_size) != FW_UPGRADE_OK_E) {
                ret = FW_UPGRADE_ERR_FLASH_ERASE_PARTITION_E;
                break;
            }
        }

        if (fw_upgrade_cxt->format == FW_UPGRADE_FORAMT_PARTIAL_UPGRADE) {
            /* it is done for this round */
            break;
        }
    }

    return ret;
}

/*
 * process duplicate file system
 */
static fw_upgrade_status_code_t fw_upgrade_process_duplicate_fs(uint32_t flags)
{
    uint8_t *buf = NULL;
    uint32_t size;
    uint32_t offset;
    uint32_t disk_size;
    uint32_t nbytes;
    fu_part_hdl_t hdl1 = NULL;
    fu_part_hdl_t hdl2 = NULL;
    fw_upgrade_status_code_t ret = FW_UPGRADE_OK_E;

    UNUSED(flags);

    fw_upgrade_get_mem_block_size(&size);
    buf = (uint8_t *)malloc(size);
    if (buf == NULL) {
        ret = FW_UPGRADE_ERR_INSUFFICIENT_MEMORY_E;
        goto dup_fs_end;
    }

    // copy FS1 to FS2
    if ((fw_upgrade_find_partition(fw_upgrade_get_active_fwd(NULL, NULL), FS1_IMG_ID, &hdl1) != FW_UPGRADE_OK_E) ||
        (fw_upgrade_find_partition(fw_upgrade_get_active_fwd(NULL, NULL), FS2_IMG_ID, &hdl2) != FW_UPGRADE_OK_E)) {
        ret = FW_UPGRADE_ERR_FLASH_IMAGE_NOT_FOUND_E;
        goto dup_fs_end;
    }

    disk_size = ((fu_partition_client_t *)hdl1)->img_size;

    for (offset = 0; offset < disk_size; offset += size) {
        if (fw_upgrade_read_partition(hdl1, offset, (char *)buf, size, &nbytes) != FW_UPGRADE_OK_E) {
            ret = FW_UPGRADE_ERR_FLASH_READ_FAIL_E;
            break;
        }
        // erase one block
        if (fw_upgrade_erase_partition(hdl2, offset, size) != FW_UPGRADE_OK_E) {
            ret = FW_UPGRADE_ERR_FLASH_ERASE_PARTITION_E;
            break;
        }
        // write flash
        if (fw_upgrade_write_partition(hdl2, offset, (char *)buf, size) != FW_UPGRADE_OK_E) {
            ret = FW_UPGRADE_ERR_FLASH_WRITE_PARTITION_E;
            break;
        }
    }

dup_fs_end:
    if (buf) {
        free(buf);
    }
    if (hdl1) {
        ((fu_partition_client_t *)hdl1)->ref_count = 0;
        hdl1 = NULL;
    }
    if (hdl2) {
        ((fu_partition_client_t *)hdl2)->ref_count = 0;
        hdl2 = NULL;
    }

    return ret;
}

/*
 * process duplicate images from current to trial if need
 */
static fw_upgrade_status_code_t fw_upgrade_process_duplicate_images(void)
{
    fw_upgrade_status_code_t ret = FW_UPGRADE_OK_E;
    uint32_t i, offset, size, nbytes;
    uint8_t *buf = NULL;
    uint8_t active_fwd;
    fw_upgrade_context_t *fw_upgrade_cxt;
    fw_upgrade_image_hdr_t *img_hdr;
    fu_part_hdl_t hdl = NULL;

    fw_upgrade_cxt = fw_upgrade_get_context();
    if (fw_upgrade_cxt == NULL) {
        return FW_UPGRADE_ERR_SESSION_NOT_START_E;
    }

    fw_upgrade_get_mem_block_size(&size);
    active_fwd = fw_upgrade_get_active_fwd(NULL, NULL);
    buf = (uint8_t *)malloc(size);
    if (buf == NULL) {
        ret = FW_UPGRADE_ERR_INSUFFICIENT_MEMORY_E;
        goto dup_img_end;
    }

    // check images if need download
    for (i = 0, img_hdr = fw_upgrade_image_hdr; i < fw_upgrade_cxt->total_images; i++, img_hdr++) {
        if ((fw_upgrade_cxt->download_flag[i] == 0) && (img_hdr->image_id != FS1_IMG_ID) &&
            (img_hdr->image_id != FS2_IMG_ID) && (img_hdr->image_id != SBL_IMG_ID)) {
            if (fw_upgrade_find_partition(active_fwd, img_hdr->image_id, &hdl) != FW_UPGRADE_OK_E) {
                ret = FW_UPGRADE_ERR_INCORRECT_IMAGE_HDR_E;
                break;
            }

            if (fw_upgrade_create_partition(fw_upgrade_cxt->trial_fwd_idx, img_hdr->image_id, img_hdr->version,
                                            fw_upgrade_cxt->trial_mem_start, img_hdr->disk_size,
                                            &fw_upgrade_cxt->partition_hdl) != FW_UPGRADE_OK_E) {
                ret = FW_UPGRADE_ERR_FLASH_CREATE_PARTITION_E;
                break;
            }

            for (offset = 0; offset < img_hdr->disk_size; offset += size) {
                fw_upgrade_read_partition(hdl, offset, (char *)buf, size, &nbytes);
                // erase one block
                if (fw_upgrade_erase_partition(fw_upgrade_cxt->partition_hdl, offset, size) != FW_UPGRADE_OK_E) {
                    ret = FW_UPGRADE_ERR_FLASH_ERASE_PARTITION_E;
                    break;
                }
                // write flash
                if (fw_upgrade_write_partition(fw_upgrade_cxt->partition_hdl, offset, (char *)buf, size) !=
                    FW_UPGRADE_OK_E) {
                    ret = FW_UPGRADE_ERR_FLASH_WRITE_PARTITION_E;
                    break;
                }
            }
            ((fu_partition_client_t *)hdl)->ref_count = 0;
            hdl = NULL;
            ((fu_partition_client_t *)(fw_upgrade_cxt->partition_hdl))->ref_count = 0;
            fw_upgrade_cxt->partition_hdl = NULL;

            // adjust memory start address for next entry
            fw_upgrade_cxt->trial_mem_start += img_hdr->disk_size;
        }
    }

dup_img_end:
    if (fw_upgrade_cxt->partition_hdl) {
        ((fu_partition_client_t *)(fw_upgrade_cxt->partition_hdl))->ref_count = 0;
        fw_upgrade_cxt->partition_hdl = NULL;
    }
    if (hdl != NULL) {
        ((fu_partition_client_t *)hdl)->ref_count = 0;
    }
    if (buf != NULL) {
        free(buf);
    }
    return ret;
}

/*
 * process fw upgrade session
 */
static int32_t fw_upgrade_session_process(void)
{
    int32_t ret = FW_UPGRADE_OK_E;
    fw_upgrade_context_t *fw_upgrade_cxt;
    uint8_t *buffer = NULL;
    uint8_t run = 1;
    uint32_t received;

    /* TODO: check battery level if need */

    /* get fw upgrade context */
    fw_upgrade_cxt = fw_upgrade_get_context();
    if (fw_upgrade_cxt == NULL) {
        return FW_UPGRADE_ERR_SESSION_NOT_START_E;
    }

    /*Allocate buffer*/
    if ((buffer = malloc(FW_UPGRADE_BUF_SIZE)) == NULL) {
        FW_UPGRADE_D_PRINTF("Out of memory error\r\n");
        return FW_UPGRADE_ERR_INSUFFICIENT_MEMORY_E;
    }

    while ((run == 1) && (fw_upgrade_get_session_status() == FW_UPGRADE_SESSION_RUNNING_E)) {
        switch (fw_upgrade_get_state()) {
            case FW_UPGRADE_STATE_NOT_START_E:
                fw_upgrade_update_callback(fw_upgrade_get_state(), fw_upgrade_get_error_code());
                fw_upgrade_set_state(FW_UPGRADE_STATE_GET_TRIAL_INFO_E);
                break;

            case FW_UPGRADE_STATE_GET_TRIAL_INFO_E:
                fw_upgrade_update_callback(fw_upgrade_get_state(), fw_upgrade_get_error_code());
                // locate available partition for trial FWD
                if (fw_upgrade_select_trial_fwd(&(fw_upgrade_cxt->trial_fwd_idx), &(fw_upgrade_cxt->trial_mem_start),
                                                &(fw_upgrade_cxt->trial_mem_size)) != FW_UPGRADE_OK_E) {
                    ret = FW_UPGRADE_ERR_FLASH_NOT_SUPPORT_FW_UPGRADE_E;
                    run = 0;
                    break;
                }

                if (fw_upgrade_get_active_fwd(NULL, NULL) == fw_upgrade_cxt->trial_fwd_idx) {
                    // if current running FWD is trial FWD, just reject the request
                    ret = FW_UPGRADE_ERR_TRIAL_IS_RUNNING_E;
                    run = 0;
                    break;
                }

                fw_upgrade_set_state(FW_UPGRADE_STATE_ERASE_FWD_E);
                break;

            case FW_UPGRADE_STATE_ERASE_FWD_E:
                fw_upgrade_update_callback(fw_upgrade_get_state(), fw_upgrade_get_error_code());
                // erase trial FWD
                if (fw_upgrade_erase_fwd(fw_upgrade_cxt->trial_fwd_idx) != FW_UPGRADE_OK_E) {
                    ret = FW_UPGRADE_ERR_FLASH_ERASE_FAIL_E;
                    run = 0;
                    break;
                }
#ifdef CONFIG_FW_UPGRADE_NO_FS
                fw_upgrade_set_state(FW_UPGRADE_STATE_PREPARE_FS_E);
#else
                fw_upgrade_set_state(FW_UPGRADE_STATE_ERASE_SECOND_FS_E);
#endif
                break;

            case FW_UPGRADE_STATE_ERASE_SECOND_FS_E: {
                uint32_t disk_size;

                fw_upgrade_update_callback(fw_upgrade_get_state(), fw_upgrade_get_error_code());

                if (fw_upgrade_find_partition(fw_upgrade_get_active_fwd(NULL, NULL), FS2_IMG_ID,
                                              &fw_upgrade_cxt->partition_hdl) != FW_UPGRADE_OK_E) {
                    ret = FW_UPGRADE_ERR_FLASH_IMAGE_NOT_FOUND_E;
                    run = 0;
                    break;
                }

                disk_size = ((fu_partition_client_t *)(fw_upgrade_cxt->partition_hdl))->img_size;

                // erase flash where to store the second FS
                if (fw_upgrade_erase_partition(fw_upgrade_cxt->partition_hdl, 0, disk_size) != FW_UPGRADE_OK_E) {
                    ret = FW_UPGRADE_ERR_FLASH_ERASE_PARTITION_E;
                    run = 0;
                    break;
                }
                ((fu_partition_client_t *)(fw_upgrade_cxt->partition_hdl))->ref_count = 0;
                fw_upgrade_cxt->partition_hdl = NULL;

                fw_upgrade_set_state(FW_UPGRADE_STATE_PREPARE_FS_E);
                break;
            }

            case FW_UPGRADE_STATE_PREPARE_FS_E: {
                fw_upgrade_update_callback(fw_upgrade_get_state(), fw_upgrade_get_error_code());
#ifndef CONFIG_FW_UPGRADE_NO_FS
                uint32_t disk_size = 0, disk_start = 0, version = 0;
                if (fw_upgrade_find_partition(fw_upgrade_get_active_fwd(NULL, NULL), FS1_IMG_ID,
                                              &fw_upgrade_cxt->partition_hdl) != FW_UPGRADE_OK_E) {
                    ret = FW_UPGRADE_ERR_FLASH_IMAGE_NOT_FOUND_E;
                    run = 0;
                    break;
                }

                disk_size = ((fu_partition_client_t *)(fw_upgrade_cxt->partition_hdl))->img_size;
                disk_start = ((fu_partition_client_t *)(fw_upgrade_cxt->partition_hdl))->img_start;
                version = ((fu_partition_client_t *)(fw_upgrade_cxt->partition_hdl))->img_version;
                ((fu_partition_client_t *)(fw_upgrade_cxt->partition_hdl))->ref_count = 0;
                fw_upgrade_cxt->partition_hdl = NULL;

                // create image for trial image's FS2
                if (fw_upgrade_create_partition(fw_upgrade_cxt->trial_fwd_idx, FS2_IMG_ID, version, disk_start,
                                                disk_size, &fw_upgrade_cxt->partition_hdl) != FW_UPGRADE_OK_E) {
                    ret = FW_UPGRADE_ERR_FLASH_CREATE_PARTITION_E;
                    run = 0;
                    break;
                }
                fw_upgrade_cxt->hidden_images_count++;
                ((fu_partition_client_t *)(fw_upgrade_cxt->partition_hdl))->ref_count = 0;
                fw_upgrade_cxt->partition_hdl = NULL;

#endif

                if (fw_upgrade_find_partition(fw_upgrade_get_active_fwd(NULL, NULL), RAMDUMP_IMG_ID,
                                              &fw_upgrade_cxt->partition_hdl) == FW_UPGRADE_OK_E) {
                    uint32_t rd_disk_size = 0, rd_disk_start = 0, rd_version = 0;
                    uint32_t ramdump_magic_number = 0;
                    uint32_t nbytes = 0;

                    rd_disk_size = ((fu_partition_client_t *)(fw_upgrade_cxt->partition_hdl))->img_size;
                    rd_disk_start = ((fu_partition_client_t *)(fw_upgrade_cxt->partition_hdl))->img_start;
                    rd_version = ((fu_partition_client_t *)(fw_upgrade_cxt->partition_hdl))->img_version;

                    if ((fw_upgrade_read_partition(fw_upgrade_cxt->partition_hdl, FLASH_OFFSET_MAGIC_NUM_ADDRESS,
                                                   (char *)&ramdump_magic_number, sizeof(ramdump_magic_number),
                                                   &nbytes) != FW_UPGRADE_OK_E) ||
                        (nbytes != sizeof(ramdump_magic_number))) {
                        ret = FW_UPGRADE_ERR_FLASH_ERASE_PARTITION_E;
                        run = 0;
                        break;
                    }

                    if (ramdump_magic_number != FLASH_ERASED_VALUE) {
                        if (fw_upgrade_erase_partition(fw_upgrade_cxt->partition_hdl, 0, rd_disk_size) !=
                            FW_UPGRADE_OK_E) {
                            ret = FW_UPGRADE_ERR_FLASH_ERASE_PARTITION_E;
                            run = 0;
                            break;
                        }
                    }

                    ((fu_partition_client_t *)(fw_upgrade_cxt->partition_hdl))->ref_count = 0;
                    fw_upgrade_cxt->partition_hdl = NULL;
                    if (fw_upgrade_create_partition(fw_upgrade_cxt->trial_fwd_idx, RAMDUMP_IMG_ID, rd_version,
                                                    rd_disk_start, rd_disk_size,
                                                    &fw_upgrade_cxt->partition_hdl) != FW_UPGRADE_OK_E) {
                        ret = FW_UPGRADE_ERR_FLASH_CREATE_PARTITION_E;
                        run = 0;
                        break;
                    }
                    fw_upgrade_cxt->hidden_images_count++;
                    ((fu_partition_client_t *)(fw_upgrade_cxt->partition_hdl))->ref_count = 0;
                    fw_upgrade_cxt->partition_hdl = NULL;
                }

                if (fw_upgrade_find_partition(fw_upgrade_get_active_fwd(NULL, NULL), USERDATA_IMG_ID,
                                              &fw_upgrade_cxt->partition_hdl) == FW_UPGRADE_OK_E) {
                    uint32_t usr_disk_size = 0, usr_disk_start = 0, usr_version = 0;

                    usr_disk_size = ((fu_partition_client_t *)(fw_upgrade_cxt->partition_hdl))->img_size;
                    usr_disk_start = ((fu_partition_client_t *)(fw_upgrade_cxt->partition_hdl))->img_start;
                    usr_version = ((fu_partition_client_t *)(fw_upgrade_cxt->partition_hdl))->img_version;
                    ((fu_partition_client_t *)(fw_upgrade_cxt->partition_hdl))->ref_count = 0;
                    fw_upgrade_cxt->partition_hdl = NULL;

                    if (fw_upgrade_create_partition(fw_upgrade_cxt->trial_fwd_idx, USERDATA_IMG_ID, usr_version,
                                                    usr_disk_start, usr_disk_size,
                                                    &fw_upgrade_cxt->partition_hdl) != FW_UPGRADE_OK_E) {
                        ret = FW_UPGRADE_ERR_FLASH_CREATE_PARTITION_E;
                        run = 0;
                        break;
                    }
                    fw_upgrade_cxt->hidden_images_count++;
                    ((fu_partition_client_t *)(fw_upgrade_cxt->partition_hdl))->ref_count = 0;
                    fw_upgrade_cxt->partition_hdl = NULL;
                }

                fw_upgrade_set_state(FW_UPGRADE_STATE_PREPARE_CONNECT_E);
                break;
            }

            case FW_UPGRADE_STATE_PREPARE_CONNECT_E:
                /* check if received config file already */
                if (fw_upgrade_cxt->is_first == 0) {
                    fw_upgrade_image_hdr_t *img_hdr;
                    uint32_t disk_size, disk_start;

                    for (; fw_upgrade_cxt->image_index < fw_upgrade_cxt->total_images; fw_upgrade_cxt->image_index++) {
                        if (fw_upgrade_cxt->download_flag[fw_upgrade_cxt->image_index] != 0) {
                            break;
                        }
                    }

                    // this is special case for FS1 IMG and remote file size is 0
                    img_hdr = fw_upgrade_image_hdr;
                    img_hdr += fw_upgrade_cxt->image_index;

                    if ((img_hdr->image_id == FS1_IMG_ID) && (img_hdr->image_length == 0)) {
                        if (fw_upgrade_find_partition(fw_upgrade_get_active_fwd(NULL, NULL), FS2_IMG_ID,
                                                      &fw_upgrade_cxt->partition_hdl) != FW_UPGRADE_OK_E) {
                            ret = FW_UPGRADE_ERR_FLASH_IMAGE_NOT_FOUND_E;
                            run = 0;
                            break;
                        }

                        disk_size = ((fu_partition_client_t *)(fw_upgrade_cxt->partition_hdl))->img_size;
                        disk_start = ((fu_partition_client_t *)(fw_upgrade_cxt->partition_hdl))->img_start;
                        ((fu_partition_client_t *)(fw_upgrade_cxt->partition_hdl))->ref_count = 0;
                        fw_upgrade_cxt->partition_hdl = NULL;

                        // File system disk size is pre-set when first time download
                        // Firmware Upgrade can't change size other than original size
                        if (img_hdr->disk_size > disk_size) {
                            ret = FW_UPGRADE_ERR_INCORRECT_IMAGE_LENGTH_E;
                            run = 0;
                            break;
                        }

                        // create image for trial image's FS2
                        if (fw_upgrade_create_partition(fw_upgrade_cxt->trial_fwd_idx, img_hdr->image_id,
                                                        img_hdr->version, disk_start, disk_size,
                                                        &fw_upgrade_cxt->partition_hdl) != FW_UPGRADE_OK_E) {
                            ret = FW_UPGRADE_ERR_FLASH_CREATE_PARTITION_E;
                            run = 0;
                            break;
                        }
                        ((fu_partition_client_t *)(fw_upgrade_cxt->partition_hdl))->ref_count = 0;
                        fw_upgrade_cxt->partition_hdl = NULL;
                        fw_upgrade_cxt->image_index++;
                        fw_upgrade_set_state(FW_UPGRADE_STATE_PREPARE_CONNECT_E);
                        break;
                    }

                    if (fw_upgrade_cxt->image_index >= fw_upgrade_cxt->total_images) {
                        fw_upgrade_set_state(FW_UPGRADE_STATE_DUPLICATE_IMAGES_E);
                    } else {
                        fw_upgrade_set_state(FW_UPGRADE_STATE_CONNECT_SERVER_E);
                    }
                } else {
                    fw_upgrade_set_state(FW_UPGRADE_STATE_CONNECT_SERVER_E);
                }
                break;

            case FW_UPGRADE_STATE_CONNECT_SERVER_E:
                fw_upgrade_update_callback(fw_upgrade_get_state(), fw_upgrade_get_error_code());
                // init FW Upgrade Plugin
                if ((ret = fw_upgrade_plugin_init()) != FW_UPGRADE_OK_E) {
                    run = 0;
                    break;
                }
                fw_upgrade_set_state(FW_UPGRADE_STATE_RECEIVE_DATA_E);
                break;

            case FW_UPGRADE_STATE_RESUME_SERVICE_E: {
                uint32_t offset, len, nbytes, total;
                fw_upgrade_image_hdr_t *img_hdr;

                fw_upgrade_update_callback(fw_upgrade_get_state(), fw_upgrade_get_error_code());

                /* get fw upgrade session context */
                fw_upgrade_cxt = fw_upgrade_get_context();
                if (fw_upgrade_cxt == NULL) {
                    ret = FW_UPGRADE_ERR_SESSION_NOT_START_E;
                    run = 0;
                    break;
                }

                img_hdr = fw_upgrade_image_hdr;
                img_hdr += fw_upgrade_cxt->image_index;

                // open partition handle
                if (fw_upgrade_find_partition(fw_upgrade_cxt->trial_fwd_idx, img_hdr->image_id,
                                              &fw_upgrade_cxt->partition_hdl) != FW_UPGRADE_OK_E) {
                    ret = FW_UPGRADE_ERR_FLASH_IMAGE_NOT_FOUND_E;
                    run = 0;
                    break;
                }

                fw_upgrade_sess_cxt->digest_ctx = (mbedtls_sha256_context *)malloc(sizeof(mbedtls_sha256_context));
                if (fw_upgrade_sess_cxt->digest_ctx == NULL) {
                    ret = FW_UPGRADE_ERR_INSUFFICIENT_MEMORY_E;
                    run = 0;
                    break;
                }
                mbedtls_sha256_init(fw_upgrade_sess_cxt->digest_ctx);
                mbedtls_sha256_starts(fw_upgrade_sess_cxt->digest_ctx, 0);

                offset = 0;
                len = 0;
                total = 0;

                // calculate fw upgrade image HASH
                while (total < fw_upgrade_cxt->image_wrt_count) {
                    if ((fw_upgrade_cxt->image_wrt_count - total) > FW_UPGRADE_BUF_SIZE) {
                        len = FW_UPGRADE_BUF_SIZE;
                    } else {
                        len = fw_upgrade_cxt->image_wrt_count - total;
                    }
                    if (fw_upgrade_read_partition(fw_upgrade_cxt->partition_hdl, offset, (char *)buffer, len,
                                                  &nbytes) != FW_UPGRADE_OK_E) {
                        ret = FW_UPGRADE_ERR_FLASH_READ_FAIL_E;
                        run = 0;
                        break;
                    }

                    mbedtls_sha256_update(fw_upgrade_cxt->digest_ctx, buffer, nbytes);
                    offset += nbytes;
                    total += nbytes;
                }

                // erase image if need

                fw_upgrade_set_state(FW_UPGRADE_STATE_RESUME_SERVER_E);
                break;
            }
            case FW_UPGRADE_STATE_RESUME_SERVER_E:
                fw_upgrade_update_callback(fw_upgrade_get_state(), fw_upgrade_get_error_code());
                // call FW Upgrade Plugin resume
                if ((ret = fw_upgrade_plugin_resume()) != FW_UPGRADE_OK_E) {
                    run = 0;
                    break;
                }
                fw_upgrade_set_state(FW_UPGRADE_STATE_RECEIVE_DATA_E);
                break;

            case FW_UPGRADE_STATE_RECEIVE_DATA_E:
                fw_upgrade_update_callback(fw_upgrade_get_state(), fw_upgrade_get_error_code());
                /* Receiving data from FTP server.*/
                if (fw_upgrade_cxt->flags & FW_UPGRADE_FLAG_RANGE_HEADER) {           
                    ret = fw_upgrade_plugin_recv_data((uint8_t *)buffer, FW_UPGRADE_BUF_SIZE, &received, &fw_upgrade_cxt->flags);
                } else {
                    ret = fw_upgrade_plugin_recv_data((uint8_t *)buffer, FW_UPGRADE_BUF_SIZE, &received, fw_upgrade_cxt->init_param);
                }

                if ((ret == FW_UPGRADE_OK_E) && (received > 0)) {
                    /* handle data */
                    fw_upgrade_cxt->buf_len = received;
                    fw_upgrade_cxt->buf_offset = 0;

                    if (fw_upgrade_cxt->is_first == 1) {
                        fw_upgrade_set_state(FW_UPGRADE_STATE_PROCESS_CONFIG_FILE_E);
                    } else {
                        fw_upgrade_set_state(FW_UPGRADE_STATE_PROCESS_IMAGE_E);
                    }

                } else if ((ret == FW_UPGRADE_OK_E) && (received == 0)) {
                    // no more data
                    run = 0;
                } else if (ret != FW_UPGRADE_OK_E) {
                    // can't get data
                    run = 0;
                }
                break;

            case FW_UPGRADE_STATE_PROCESS_CONFIG_FILE_E:
                fw_upgrade_update_callback(fw_upgrade_get_state(), fw_upgrade_get_error_code());
                /* parse fw upgrade image Header */
                if ((ret = fw_upgrade_process_config_file(buffer)) != FW_UPGRADE_OK_E) {
                    fw_upgrade_plugin_abort();
                    run = 0;
                }
                break;

            case FW_UPGRADE_STATE_PROCESS_IMAGE_E:
                fw_upgrade_update_callback(fw_upgrade_get_state(), fw_upgrade_get_error_code());
                if ((ret = fw_upgrade_process_receive_image(buffer)) != FW_UPGRADE_OK_E) {
                    fw_upgrade_plugin_abort();
                    run = 0;
                } else if (fw_upgrade_get_state() == FW_UPGRADE_STATE_PROCESS_IMAGE_E) {
                    // continue receiving data
                    fw_upgrade_set_state(FW_UPGRADE_STATE_RECEIVE_DATA_E);
                }
                break;

            case FW_UPGRADE_STATE_DISCONNECT_SERVER_E:
                fw_upgrade_update_callback(fw_upgrade_get_state(), fw_upgrade_get_error_code());
                fw_upgrade_plugin_fin();
                fw_upgrade_set_state(FW_UPGRADE_STATE_PREPARE_CONNECT_E);
                break;

            case FW_UPGRADE_STATE_DUPLICATE_IMAGES_E:
                fw_upgrade_update_callback(fw_upgrade_get_state(), fw_upgrade_get_error_code());
                // duplicate images from current to trial if need
                if ((ret = fw_upgrade_process_duplicate_images()) != FW_UPGRADE_OK_E) {
                    run = 0;
                    break;
                }
                fw_upgrade_set_state(FW_UPGRADE_STATE_DUPLICATE_FS_E);
                break;

            case FW_UPGRADE_STATE_DUPLICATE_FS_E:
                fw_upgrade_update_callback(fw_upgrade_get_state(), fw_upgrade_get_error_code());
                // check duplicate FS flag
                if (fw_upgrade_cxt->flags & FW_UPGRADE_FLAG_DUPLICATE_ACTIVE_FS) {
                    // copy files from FS1 to FS2
                    if ((ret = fw_upgrade_process_duplicate_fs(fw_upgrade_cxt->flags)) != FW_UPGRADE_OK_E) {
                        run = 0;
                        break;
                    }
                }

                fw_upgrade_set_state(FW_UPGRADE_STATE_FINISH_E);
                break;
            case FW_UPGRADE_STATE_FINISH_E:
                fw_upgrade_update_callback(fw_upgrade_get_state(), fw_upgrade_get_error_code());
                run = 0;
                break;
            default:
                break;
        }  // switch(...

    }  // while(...

    /* free bufer */
    if (buffer) {
        free(buffer);
    }

    // set error code
    if (fw_upgrade_get_session_status() == FW_UPGRADE_SESSION_CANCEL_E) {
        fw_upgrade_set_error_code(FW_UPGRADE_ERR_SESSION_CANCELLED_E);
    } else {
        fw_upgrade_set_error_code(ret);
    }
    return (ret);
}

/*
 * finalize fw upgrade session
 */
static int32_t fw_upgrade_session_finalize(int32_t ret)
{
    fw_upgrade_context_t *fw_upgrade_cxt;
    uint8_t status;
    fu_part_hdl_t hdl;
    fw_upgrade_status_code_t resp;
#ifdef CONFIG_QAT_OTA_DEMO
    TimerHandle_t rst_timer_handle;
#endif
    /* get fw upgrade session context */
    fw_upgrade_cxt = fw_upgrade_get_context();
    if (fw_upgrade_cxt == NULL) {
        return FW_UPGRADE_ERR_SESSION_NOT_START_E;
    }

    // call plugin fin
    fw_upgrade_plugin_fin();

    // check if this session is cancelled or set to suspend
    if (fw_upgrade_get_session_status() == FW_UPGRADE_SESSION_CANCEL_E) {
        ret = FW_UPGRADE_ERR_SESSION_CANCELLED_E;
    } else if ((ret == FW_UPGRADE_OK_E) && (fw_upgrade_get_state() != FW_UPGRADE_STATE_FINISH_E)) {
        ret = FW_UPGRADE_ERR_INCOMPLETE_E;
    }

    // set final error code
    fw_upgrade_set_error_code(ret);
    // update state and err code
    fw_upgrade_update_callback(fw_upgrade_get_state(), fw_upgrade_get_error_code());

    /* everything is good */
    if ((ret == FW_UPGRADE_OK_E) && (fw_upgrade_get_state() == FW_UPGRADE_STATE_FINISH_E)) {
        status = FW_UPGRADE_FWD_STATUS_VALID;
        fw_upgrade_set_fwd_info(fw_upgrade_cxt->trial_fwd_idx, FW_UPGRADE_FWD_STATUS_E, (uint8_t *)&status);

        /* set SBL Trial FDE */
        resp = fw_upgrade_find_partition(fw_upgrade_cxt->trial_fwd_idx, SBL_IMG_ID, &hdl);
        if (resp == FW_UPGRADE_OK_E) {
            fw_upgrade_set_sbl_trial_fde(fw_upgrade_cxt->trial_sbl_idx, ((fu_partition_client_t *)hdl)->img_version);
        }

        // check AUTO_REBOOT flag
        if (fw_upgrade_cxt->flags & FW_UPGRADE_FLAG_AUTO_REBOOT) {
            fw_upgrade_session_fin();
#ifdef CONFIG_QAT_OTA_DEMO
            // when using QAT demo, need some times to send response to SPI host
            rst_timer_handle = nt_qurt_timer_create("rst_timer", 100, TRUE, NULL, qat_common_rst_timer_callback);
            qurt_timer_start(rst_timer_handle, 0);
#else
            // reboot system here
            nt_system_sw_reset();
#endif
        }
    }

    if (fw_upgrade_get_session_status() == FW_UPGRADE_SESSION_SUSPEND_E) {
        fw_upgrade_session_prepare_suspend();
        ret = FW_UPGRADE_ERR_SESSION_SUSPEND_E;
    } else {
        fw_upgrade_session_fin();
    }
    return ret;
}

/**********************************************************************************************************/
/* 																                                          */
/**********************************************************************************************************/

/*
 * start firmware upgrade session
 */
int32_t fw_upgrade(char *interface_name, fw_upgrade_plugin_t *plugin, char *url, char *cfg_file, uint32_t flags,
                   fw_upgrade_cb_t cb, void *init_param)
{
    int32_t ret = FW_UPGRADE_OK_E;

    if (fw_upgrade_get_context() != NULL) {
        return FW_UPGRADE_ERR_SESSION_IN_PROGRESS_E;
    }

    /* session init */
    if ((ret = fw_upgrade_session_init(interface_name, plugin, url, cfg_file, flags, cb, init_param)) !=
        FW_UPGRADE_OK_E) {
        /* fail to session init, just return here with error code */
        return ret;
    }

    /* fw upgrade session process */
    ret = fw_upgrade_session_process();
    return fw_upgrade_session_finalize(ret);
}

/*
 * resume firmware upgrade session
 */
int32_t fw_upgrade_session_resume(void)
{
    int32_t ret = FW_UPGRADE_OK_E;
    uint32_t state;

    /* get fw upgrade session context */
    if (fw_upgrade_get_context() == NULL) {
        return FW_UPGRADE_ERR_SESSION_NOT_START_E;
    }

    if (fw_upgrade_mutex_init == 0) {
        qurt_mutex_create(&fw_upgrade_mutex);
        fw_upgrade_mutex_init = 1;
    }

    if (fw_upgrade_get_session_status() != FW_UPGRADE_SESSION_SUSPEND_E) {
        return FW_UPGRADE_ERR_SESSION_IN_PROGRESS_E;
    }

    /* reset session status */
    fw_upgrade_set_session_status(FW_UPGRADE_SESSION_RUNNING_E);

    /* init FW_Upgrade */
    if (fw_upgrade_init() != FW_UPGRADE_OK_E) {
        fw_upgrade_set_session_status(FW_UPGRADE_SESSION_NOT_START_E);
        ret = FW_UPGRADE_ERR_FLASH_INIT_TIMEOUT_E;
        goto session_resume_end;
    }

    // restore State
    state = (uint32_t)fw_upgrade_get_state();
    if (state >= FW_UPGRADE_STATE_RESUME_SERVICE_E) {
        fw_upgrade_set_state(FW_UPGRADE_STATE_RESUME_SERVICE_E);
    }

    // reset error code
    fw_upgrade_set_error_code(FW_UPGRADE_OK_E);
    ret = fw_upgrade_session_process();

session_resume_end:
    return fw_upgrade_session_finalize(ret);
}

/*
 * cancel firmware upgrade session
 */
fw_upgrade_status_code_t fw_upgrade_session_cancel(void)
{
    if (fw_upgrade_get_context() == NULL) {
        return FW_UPGRADE_ERR_SESSION_NOT_START_E;
    }
    if (fw_upgrade_get_session_status() == FW_UPGRADE_SESSION_RUNNING_E) {
        return fw_upgrade_set_session_status(FW_UPGRADE_SESSION_CANCEL_E);
    }
    return fw_upgrade_session_fin();
}

/*
 * suspend firmware upgrade session
 */
fw_upgrade_status_code_t fw_upgrade_session_suspend(void)
{
    if ((fw_upgrade_get_context() == NULL) || (fw_upgrade_get_session_status() != FW_UPGRADE_SESSION_RUNNING_E))
        return FW_UPGRADE_ERR_SESSION_NOT_START_E;
    return fw_upgrade_set_session_status(FW_UPGRADE_SESSION_SUSPEND_E);
}

/*
 * done firmware upgrade session
 */
fw_upgrade_status_code_t fw_upgrade_session_done(uint32_t result)
{
    fw_upgrade_status_code_t ret = FW_UPGRADE_ERROR_E;
    uint8_t trial, current;
    uint32_t rank;

    if (fw_upgrade_get_trial_active_fwd_index(&trial, &current, &rank) == FW_UPGRADE_OK_E) {
        /* process trial image based on result */
        if (result) {
            /* accept trial image here */
            if (fw_upgrade_accept_trial_fwd() == FW_UPGRADE_OK_E)
                ret = FW_UPGRADE_OK_E;
        } else {
            /*reject trial image */
            if (fw_upgrade_reject_trial_fwd() == FW_UPGRADE_OK_E)
                ret = FW_UPGRADE_OK_E;
        }
    }

    return ret;
}
