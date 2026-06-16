/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "fs.h"
#include "qat.h"
#include "qat_api.h"

/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 *-----------------------------------------------------------------------*/
#define FSTORE_MOUNT_POINT     "/lfs"
#define FSTORE_MAX_PATH        130
#define FSTORE_READ_CHUNK_SIZE 512
#define FSTORE_CHUNK_HEX_MAX   128  /* max bytes per hex chunk = 256 hex chars */

/*-------------------------------------------------------------------------
 * Type Declarations
 *-----------------------------------------------------------------------*/
static struct {
    struct fs_file_t file;
    size_t           total_len;
    size_t           received_len;
    bool             active;
} writefile_state;

/*-------------------------------------------------------------------------
 * Function Declarations
 *-----------------------------------------------------------------------*/
static QAT_Command_Status_t Extend_Command_WriteFile(uint32_t Op_Type, uint32_t Parameter_Count,
                                                     QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_ReadFile(uint32_t Op_Type, uint32_t Parameter_Count,
                                                    QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_DelFile(uint32_t Op_Type, uint32_t Parameter_Count,
                                                   QAT_Parameter_t *Parameter_List);

/*-------------------------------------------------------------------------
 * Command Table
 *-----------------------------------------------------------------------*/
static QAT_Command_t QAT_Fstore_Command_List[] = {
    {"+WRITEFILE", Extend_Command_WriteFile, QAT_OP_EXEC_W_PARAM | QAT_OP_EXEC},
    {"+READFILE",  Extend_Command_ReadFile,  QAT_OP_EXEC_W_PARAM | QAT_OP_EXEC},
    {"+DELFILE",   Extend_Command_DelFile,   QAT_OP_EXEC_W_PARAM | QAT_OP_EXEC},
};

/*-------------------------------------------------------------------------
 * Helper: normalize path to /lfs/<path>
 * Returns 0 on success, -1 if the result would exceed out_sz.
 *-----------------------------------------------------------------------*/
static int normalize_path(const char *input, char *out, size_t out_sz)
{
    bool has_prefix = strncmp(input, FSTORE_MOUNT_POINT, strlen(FSTORE_MOUNT_POINT)) == 0;

    if (has_prefix) {
        if (strlen(input) + 1 > out_sz) {
            return -1;
        }
        snprintf(out, out_sz, "%s", input);
    } else if (input[0] == '/') {
        /* Input starts with '/' but not the mount point: concatenate directly
         * to avoid double slash (e.g. /lfs + /foo/bar → /lfs/foo/bar). */
        if (strlen(FSTORE_MOUNT_POINT) + strlen(input) + 1 > out_sz) {
            return -1;
        }
        snprintf(out, out_sz, "%s%s", FSTORE_MOUNT_POINT, input);
    } else {
        /* Relative path: insert separator. */
        if (strlen(FSTORE_MOUNT_POINT) + 1 + strlen(input) + 1 > out_sz) {
            return -1;
        }
        snprintf(out, out_sz, "%s/%s", FSTORE_MOUNT_POINT, input);
    }
    return 0;
}

/*-------------------------------------------------------------------------
 * Helper: create all intermediate directories (mkdir -p semantics)
 *-----------------------------------------------------------------------*/
static void mkdir_parent(const char *path)
{
    char dir[FSTORE_MAX_PATH];
    /* Start scanning after the mount point prefix */
    const char *p = path + strlen(FSTORE_MOUNT_POINT);

    while (*p == '/') {
        p++;
    }

    while (*p) {
        const char *slash = strchr(p, '/');
        if (!slash) {
            break;  /* last component is the filename, stop */
        }
        size_t dir_len = (size_t)(slash - path);
        if (dir_len == 0 || dir_len >= sizeof(dir)) {
            break;
        }
        snprintf(dir, sizeof(dir), "%.*s", (int)dir_len, path);
        vfs_mkdir(dir);  /* ignore error — EEXIST is fine */
        p = slash + 1;
    }
}

/*-------------------------------------------------------------------------
 * Helper: decode one hex nibble; returns -1 on invalid character
 *-----------------------------------------------------------------------*/
static int hex_val(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/*-------------------------------------------------------------------------
 * AT+WRITEFILE=<path>,C,<size>   — create/truncate
 * AT+WRITEFILE=<path>,A,<hex>    — append hex-encoded chunk
 *-----------------------------------------------------------------------*/
static QAT_Command_Status_t Extend_Command_WriteFile(uint32_t Op_Type, uint32_t Parameter_Count,
                                                     QAT_Parameter_t *Parameter_List)
{
    char response[64];

    switch (Op_Type) {
        case QAT_OP_EXEC_W_PARAM: {
            if (Parameter_Count < 2 || !Parameter_List[0].String_Value ||
                !Parameter_List[1].String_Value) {
                QAT_Response_Str(QAT_RC_ERROR,
                    "+WRITEFILE: usage AT+WRITEFILE=<path>,C,<size> or AT+WRITEFILE=<path>,A,<hex>");
                return QAT_STATUS_SUCCESS_E;
            }

            if (!is_fs_mounted()) {
                QAT_Response_Str(QAT_RC_ERROR, "+WRITEFILE: filesystem not mounted");
                return QAT_STATUS_SUCCESS_E;
            }

            char path[FSTORE_MAX_PATH];
            if (normalize_path(Parameter_List[0].String_Value, path, sizeof(path)) < 0) {
                QAT_Response_Str(QAT_RC_ERROR, "+WRITEFILE: path too long");
                return QAT_STATUS_SUCCESS_E;
            }

            char mode = Parameter_List[1].String_Value[0];

            /* ---- C: create / truncate ---- */
            if (mode == 'C' || mode == 'c') {
                size_t expected = (Parameter_Count >= 3 && Parameter_List[2].Integer_Is_Valid)
                                  ? (size_t)Parameter_List[2].Integer_Value : 0;

                if (writefile_state.active) {
                    vfs_close(&writefile_state.file);
                    writefile_state.active = false;
                }
                mkdir_parent(path);
                fs_file_t_init(&writefile_state.file);
                int ret = vfs_open(&writefile_state.file, path, FS_O_CREATE | FS_O_WRITE);
                if (ret < 0) {
                    snprintf(response, sizeof(response), "+WRITEFILE: open failed (%d)", ret);
                    QAT_Response_Str(QAT_RC_ERROR, response);
                    return QAT_STATUS_SUCCESS_E;
                }
                vfs_truncate(&writefile_state.file, 0);
                writefile_state.total_len    = expected;
                writefile_state.received_len = 0;
                writefile_state.active       = true;
                QAT_Response_Str(QAT_RC_OK, NULL);

            /* ---- A: append hex-encoded chunk ---- */
            } else if (mode == 'A' || mode == 'a') {
                if (!writefile_state.active) {
                    QAT_Response_Str(QAT_RC_ERROR,
                        "+WRITEFILE: no file open, send C command first");
                    return QAT_STATUS_SUCCESS_E;
                }
                if (Parameter_Count < 3 || !Parameter_List[2].String_Value) {
                    QAT_Response_Str(QAT_RC_ERROR, "+WRITEFILE: hex data missing");
                    return QAT_STATUS_SUCCESS_E;
                }

                const char *hex = Parameter_List[2].String_Value;
                size_t hex_len = strlen(hex);

                if (hex_len == 0 || hex_len % 2 != 0) {
                    QAT_Response_Str(QAT_RC_ERROR,
                        "+WRITEFILE: hex data missing or odd length");
                    return QAT_STATUS_SUCCESS_E;
                }
                if (hex_len > FSTORE_CHUNK_HEX_MAX * 2) {
                    QAT_Response_Str(QAT_RC_ERROR, "+WRITEFILE: hex chunk too large");
                    return QAT_STATUS_SUCCESS_E;
                }

                uint8_t decode[FSTORE_CHUNK_HEX_MAX];
                size_t byte_count = hex_len / 2;

                for (size_t i = 0; i < byte_count; i++) {
                    int hi = hex_val(hex[i * 2]);
                    int lo = hex_val(hex[i * 2 + 1]);
                    if (hi < 0 || lo < 0) {
                        QAT_Response_Str(QAT_RC_ERROR, "+WRITEFILE: invalid hex character");
                        return QAT_STATUS_SUCCESS_E;
                    }
                    decode[i] = (uint8_t)((hi << 4) | lo);
                }

                if (writefile_state.total_len > 0 &&
                    writefile_state.received_len + byte_count > writefile_state.total_len) {
                    QAT_Response_Str(QAT_RC_ERROR, "+WRITEFILE: data exceeds declared length");
                    return QAT_STATUS_SUCCESS_E;
                }

                int32_t written = vfs_write(&writefile_state.file, decode, byte_count);
                if (written < 0) {
                    vfs_close(&writefile_state.file);
                    writefile_state.active = false;
                    snprintf(response, sizeof(response), "+WRITEFILE: write error (%d)",
                             (int)written);
                    QAT_Response_Str(QAT_RC_ERROR, response);
                    return QAT_STATUS_SUCCESS_E;
                }
                writefile_state.received_len += (size_t)written;

                if (writefile_state.total_len > 0 &&
                    writefile_state.received_len >= writefile_state.total_len) {
                    int close_ret = vfs_close(&writefile_state.file);
                    writefile_state.active = false;
                    if (close_ret < 0) {
                        snprintf(response, sizeof(response),
                                 "+WRITEFILE: close failed (%d)", close_ret);
                        QAT_Response_Str(QAT_RC_ERROR, response);
                    } else {
                        snprintf(response, sizeof(response), "+WRITEFILE: %u bytes written",
                                 (unsigned)writefile_state.received_len);
                        QAT_Response_Str(QAT_RC_OK, response);
                    }
                } else {
                    QAT_Response_Str(QAT_RC_OK, NULL);
                }

            } else {
                QAT_Response_Str(QAT_RC_ERROR, "+WRITEFILE: unknown mode, use C or A");
            }
            break;
        }

        default:
            if (writefile_state.active) {
                vfs_close(&writefile_state.file);
                writefile_state.active = false;
                QAT_Response_Str(QAT_RC_OK, "+WRITEFILE: transfer aborted");
            } else {
                QAT_Response_Str(QAT_RC_OK,
                    "+WRITEFILE=<path>,C,<size>  or  +WRITEFILE=<path>,A,<hex>");
            }
            break;
    }

    return QAT_STATUS_SUCCESS_E;
}

/*-------------------------------------------------------------------------
 * AT+READFILE=<path>
 *-----------------------------------------------------------------------*/
static QAT_Command_Status_t Extend_Command_ReadFile(uint32_t Op_Type, uint32_t Parameter_Count,
                                                    QAT_Parameter_t *Parameter_List)
{
    char response[64];

    if (Op_Type != QAT_OP_EXEC_W_PARAM) {
        QAT_Response_Str(QAT_RC_OK, "+READFILE=<path>");
        return QAT_STATUS_SUCCESS_E;
    }

    if (Parameter_Count < 1 || !Parameter_List[0].String_Value) {
        QAT_Response_Str(QAT_RC_ERROR, "+READFILE: usage AT+READFILE=<path>");
        return QAT_STATUS_SUCCESS_E;
    }

    if (!is_fs_mounted()) {
        QAT_Response_Str(QAT_RC_ERROR, "+READFILE: filesystem not mounted");
        return QAT_STATUS_SUCCESS_E;
    }

    char path[FSTORE_MAX_PATH];
    if (normalize_path(Parameter_List[0].String_Value, path, sizeof(path)) < 0) {
        QAT_Response_Str(QAT_RC_ERROR, "+READFILE: path too long");
        return QAT_STATUS_SUCCESS_E;
    }

    struct fs_dirent entry;
    if (vfs_stat(path, &entry) < 0) {
        QAT_Response_Str(QAT_RC_ERROR, "+READFILE: file not found");
        return QAT_STATUS_SUCCESS_E;
    }

    struct fs_file_t fp;
    fs_file_t_init(&fp);
    int ret = vfs_open(&fp, path, FS_O_READ);
    if (ret < 0) {
        snprintf(response, sizeof(response), "+READFILE: open failed (%d)", ret);
        QAT_Response_Str(QAT_RC_ERROR, response);
        return QAT_STATUS_SUCCESS_E;
    }

    uint8_t chunk[FSTORE_READ_CHUNK_SIZE];
    int32_t nread;
    while ((nread = vfs_read(&fp, chunk, sizeof(chunk))) > 0) {
        QAT_Response_Buffer(QAT_RC_QUIET, (char *)chunk, (uint32_t)nread);
    }

    vfs_close(&fp);

    if (nread < 0) {
        snprintf(response, sizeof(response), "+READFILE: read error (%d)", (int)nread);
        QAT_Response_Str(QAT_RC_ERROR, response);
    } else {
        QAT_Response_Str(QAT_RC_OK, NULL);
    }

    return QAT_STATUS_SUCCESS_E;
}

/*-------------------------------------------------------------------------
 * AT+DELFILE=<path>
 *-----------------------------------------------------------------------*/
static QAT_Command_Status_t Extend_Command_DelFile(uint32_t Op_Type, uint32_t Parameter_Count,
                                                   QAT_Parameter_t *Parameter_List)
{
    char response[64];

    if (Op_Type != QAT_OP_EXEC_W_PARAM) {
        QAT_Response_Str(QAT_RC_OK, "+DELFILE=<path>");
        return QAT_STATUS_SUCCESS_E;
    }

    if (Parameter_Count < 1 || !Parameter_List[0].String_Value) {
        QAT_Response_Str(QAT_RC_ERROR, "+DELFILE: usage AT+DELFILE=<path>");
        return QAT_STATUS_SUCCESS_E;
    }

    if (!is_fs_mounted()) {
        QAT_Response_Str(QAT_RC_ERROR, "+DELFILE: filesystem not mounted");
        return QAT_STATUS_SUCCESS_E;
    }

    char path[FSTORE_MAX_PATH];
    if (normalize_path(Parameter_List[0].String_Value, path, sizeof(path)) < 0) {
        QAT_Response_Str(QAT_RC_ERROR, "+DELFILE: path too long");
        return QAT_STATUS_SUCCESS_E;
    }

    int ret = vfs_unlink(path);
    if (ret < 0) {
        snprintf(response, sizeof(response), "+DELFILE: failed (%d)", ret);
        QAT_Response_Str(QAT_RC_ERROR, response);
    } else {
        QAT_Response_Str(QAT_RC_OK, NULL);
    }

    return QAT_STATUS_SUCCESS_E;
}

/*-------------------------------------------------------------------------
 * Public: Initialize_QAT_Fstore_Demo
 *-----------------------------------------------------------------------*/
void Initialize_QAT_Fstore_Demo(void)
{
    memset(&writefile_state, 0, sizeof(writefile_state));
    QAT_Register_Command_Group(QAT_Fstore_Command_List,
        sizeof(QAT_Fstore_Command_List) / sizeof(QAT_Command_t));
}
