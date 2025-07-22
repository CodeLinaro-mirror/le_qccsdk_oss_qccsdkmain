/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _NT_LOGGER_API_H
#define _NT_LOGGER_API_H
#ifdef NT_FN_LOGGER
/* -- Includes -- */
#include <stdint.h> /*for unsigned int data types */
#include "nt_logger.h"
#include "nt_common.h" /*for Moduleid's fetching*/
#include "autoconf.h"
#include "iot_wifi.h"
#include "wifi_fw_logger.h"
extern uint8_t set_host_loglvl[NT_MAX_MODULE_ID];
/*!< Log level and module id declared through cli*/
extern uint8_t set_loglvl[NT_MAX_MODULE_ID];
/*! @info is low priority. */
#define NT_LOG_LVL_INFO 0
/*! @warning condition priority. */
#define NT_LOG_LVL_WARN 1
/*! @error condition priority. */
#define NT_LOG_LVL_ERR 2
/*! @critical condition priority. */
#define NT_LOG_LVL_CRIT 3

/**
 * for log level is with in the range.
 */
#define LOG_LEVEL_CHECK(loglvl)               \
    if (loglvl >= NT_LOG_LVL_MAX_LOG_LEVEL) { \
        return NT_EFAIL;                      \
    }
/**
 * check the buffer size overflow by block size.
 */
#define BUFFER_SIZE_CHECK(block_size)        \
    if (block_size > NT_LOG_MAX_BUFF_SIZE) { \
        return NT_EFAIL;                     \
    }
#define NUMARGS(...) (sizeof((uint32_t[]){0, ##__VA_ARGS__}) / sizeof(int) - 1)

#ifndef SUPPORT_FERMION_LOGGER
#if (NT_FN_FUNCTION_LINE_NUM_FLAG == 0)

#define NT_LOG_PRINT(MODNAME, LVL, msg, ...) \
    nt_log_printf(NT_STATUS_MOD_##MODNAME, NT_LOG_LVL_##LVL, msg, NUMARGS(__VA_ARGS__), ##__VA_ARGS__)
#else
#define NT_LOG_PRINT(MODNAME, LVL, msg, ...)                                                                        \
    nt_log_printf(NT_STATUS_MOD_##MODNAME, NT_LOG_LVL_##LVL, (char *)__func__, __LINE__, msg, NUMARGS(__VA_ARGS__), \
                  ##__VA_ARGS__)

#endif  // NT_FN_FUNCTION_LINE_NUM_FLAG ==  0

#if (NT_FN_FUNCTION_LINE_NUM_FLAG == 0)
#define NT_LOG_ARRAY_PRINT(MODNAME, LVL, str, ptr, len) \
    nt_log_array_printf(NT_STATUS_MOD_##MODNAME, NT_LOG_LVL_##LVL, str, ptr, len)
#else
#define NT_LOG_ARRAY_PRINT(MODNAME, LVL, str, ptr, len) \
    nt_log_array_printf(NT_STATUS_MOD_##MODNAME, NT_LOG_LVL_##LVL, (char *)__func__, __LINE__, str, ptr, len)
#endif

#else  // SUPPORT_FERMION_LOGGER
#define NT_LOG_PRINT(MODNAME, LVL, msg, ...)                                      \
    do {                                                                          \
        NT_LOG_PRINT_FORMAT_CHK(msg, NUMARGS(__VA_ARGS__))                        \
        if (NT_LOG_LVL_##LVL >= set_loglvl[NT_STATUS_MOD_##MODNAME]) {            \
            DEBUG_LOG_STRING(log_fmt, msg);                                       \
            log_to_buffer_variadic(log_fmt, NUMARGS(__VA_ARGS__), ##__VA_ARGS__); \
        }                                                                         \
    } while (0)

#define NT_LOG_ARRAY_PRINT(MODNAME, LVL, msg, ptr, len)                \
    do {                                                               \
        if (NT_LOG_LVL_##LVL >= set_loglvl[NT_STATUS_MOD_##MODNAME]) { \
            DEBUG_LOG_STRING(log_fmt, msg                              \
                             ""                                        \
                             " %s");                                   \
            log_arr_to_buffer(log_fmt, ptr, len);                      \
        }                                                              \
    } while (0)

#define NT_LOG_PRINT_MINIMAL(MODNAME, LVL, msg, ...)                             \
    do {                                                                         \
        if (NT_LOG_LVL_##LVL >= set_loglvl[NT_STATUS_MOD_##MODNAME]) {           \
            DEBUG_LOG_STRING(log_fmt, msg);                                      \
            log_to_buffer_minimal(log_fmt, NUMARGS(__VA_ARGS__), ##__VA_ARGS__); \
        }                                                                        \
    } while (0)
#endif  // SUPPORT_FERMION_LOGGER

/**
 * @FUNCTION    :  nt_log_printf
 * @Description :  Check the  log level and module id,pass the data to buffer.
 *  Check the log level is in with the range or not. pass the variable arguments list.
 * @Param       :  log level(0 - 3):  like info,warn,err,critical.
 *                 moduleid        : (8 - bit) module id
 *                 fmt             : message
                   num             : number of variable arguments.
                   ...             : variable arguments list.
 * @return      :  PASS/FAILURE/nt_err_loglvl/nt_err_buff_size.
 */

uint8_t nt_log_printf(uint8_t mod_id, uint8_t loglvl,
#if (NT_FN_FUNCTION_LINE_NUM_FLAG == 1)
                      char *func_name,
                      /*@ for line number*/
                      uint16_t ln,
#endif
                      const char *fmt, uint8_t num, ...);

uint8_t nt_log_array_printf(uint8_t mod_id, uint8_t loglvl,
#if (NT_FN_FUNCTION_LINE_NUM_FLAG == 1)
                            char *func_name,
                            /*@ for line number*/
                            uint16_t ln,
#endif
                            const char *pmsg, const uint8_t *ptr, const uint16_t len);

/**
 * below mentioned macros we are using to create the logs. Goto the code and check the which module we are create the
 * logs. create logs for related module. And we are adding new module logs. Can you please first create a macro for
 * module id (NT_STATUS_MOD_ ...). and update the NT_MAX_MODULE_ID. This two macros available in nt_common.h.
 */
#if (NT_FN_FUNCTION_LINE_NUM_FLAG == 0)
#define NT_LOG_FILE_LOC
#else
#define NT_LOG_FILE_LOC (char *)__func__, __LINE__,
#endif

#if NT_FN_FIXED_ARG_LOGGER

/**
 * @Function    :  nt_log_sub_mod_info
 * @Description :  Check if sub module is enabled for logging
 */
uint8_t nt_log_sub_mod_info(uint8_t mod_id, uint8_t loglvl, uint16_t sub_mod_id, const char *msg, uint32_t p1,
                            uint32_t p2, uint32_t p3);

/**
 * @FUNCTION    :  nt_log_write
 * @Description :  check the buffer size and log level and module id,pass the data to buffer.
 * @Param       :  log level(0 - 3):  like info,warn,err,critical.
 *                 moduleid        : (8 - bit) module id
                   p1,p2,p3        : user calling parameters.
                   fn              : for function name.
                   ln              : for line number.
 * @return      :  Success/FAILURE/nt_err_loglvl/nt_err_buff_size.
 */
uint8_t nt_log_write(uint8_t mod_id, uint8_t loglvl,
#if (NT_FN_FUNCTION_LINE_NUM_FLAG == 1)
                     char *fn, uint16_t ln,
#endif
                     const char *msg, uint32_t p1, uint32_t p2, uint32_t p3);

// NT_LOG_MOD_LVL macros should not contain any format specifiers. Apprnding them manually for the decoder
#define LOGGER_SPECIFIERS " %d %d %d"

#ifndef SUPPORT_FERMION_LOGGER
#define NT_LOG_MOD_LVL(MODNAME, LVL, msg, p1, p2, p3) \
    nt_log_write(NT_STATUS_MOD_##MODNAME, NT_LOG_LVL_##LVL, NT_LOG_FILE_LOC msg, p1, p2, p3)

#else  // SUPPORT_FERMION_LOGGER

#define NT_LOG_MOD_LVL(MODNAME, LVL, msg, p1, p2, p3)                  \
    do {                                                               \
        NT_LOG_MOD_LVL_FORMAT_CHK(msg);                                \
        if (NT_LOG_LVL_##LVL >= set_loglvl[NT_STATUS_MOD_##MODNAME]) { \
            DEBUG_LOG_STRING(log_fmt, msg "" LOGGER_SPECIFIERS);       \
            log_to_buffer_fixed(log_fmt, p1, p2, p3);                  \
        }                                                              \
    } while (0)
#endif  // SUPPORT_FERMION_LOGGER

#define NT_LOG_DYNAMIC_STRING(MODNAME, LVL, msg, ptr)                  \
    do {                                                               \
        if (NT_LOG_LVL_##LVL >= set_loglvl[NT_STATUS_MOD_##MODNAME]) { \
            DEBUG_LOG_STRING(log_fmt, msg                              \
                             ""                                        \
                             " %s");                                   \
            log_dynamic_string_to_buffer(log_fmt, ptr);                \
        }                                                              \
    } while (0)

#define NT_LOG_HAL_INFO(msg, p1, p2, p3) NT_LOG_MOD_LVL(HAL, INFO, msg, p1, p2, p3)

#define NT_LOG_HAL_WARN(msg, p1, p2, p3) NT_LOG_MOD_LVL(HAL, WARN, msg, p1, p2, p3)

#define NT_LOG_HAL_ERR(msg, p1, p2, p3) NT_LOG_MOD_LVL(HAL, ERR, msg, p1, p2, p3)

#define NT_LOG_HAL_CRIT(msg, p1, p2, p3) NT_LOG_MOD_LVL(HAL, CRIT, msg, p1, p2, p3)

#define NT_LOG_SME_INFO(msg, p1, p2, p3) NT_LOG_MOD_LVL(SME, INFO, msg, p1, p2, p3)

#define NT_LOG_SME_WARN(msg, p1, p2, p3) NT_LOG_MOD_LVL(SME, WARN, msg, p1, p2, p3)

#define NT_LOG_SME_ERR(msg, p1, p2, p3) NT_LOG_MOD_LVL(SME, ERR, msg, p1, p2, p3)

#define NT_LOG_SME_CRIT(msg, p1, p2, p3) NT_LOG_MOD_LVL(SME, CRIT, msg, p1, p2, p3)

#define NT_LOG_MLM_INFO(msg, p1, p2, p3) NT_LOG_MOD_LVL(MLM, INFO, msg, p1, p2, p3)

#define NT_LOG_MLM_WARN(msg, p1, p2, p3) NT_LOG_MOD_LVL(MLM, WARN, msg, p1, p2, p3)

#define NT_LOG_MLM_ERR(msg, p1, p2, p3) NT_LOG_MOD_LVL(MLM, ERR, msg, p1, p2, p3)

#define NT_LOG_MLM_CRIT(msg, p1, p2, p3) NT_LOG_MOD_LVL(MLM, CRIT, msg, p1, p2, p3)

#define NT_LOG_DPM_INFO(msg, p1, p2, p3) NT_LOG_MOD_LVL(DPM, INFO, msg, p1, p2, p3)

#define NT_LOG_DPM_WARN(msg, p1, p2, p3) NT_LOG_MOD_LVL(DPM, WARN, msg, p1, p2, p3)

#define NT_LOG_DPM_ERR(msg, p1, p2, p3) NT_LOG_MOD_LVL(DPM, ERR, msg, p1, p2, p3)

#define NT_LOG_DPM_CRIT(msg, p1, p2, p3) NT_LOG_MOD_LVL(DPM, CRIT, msg, p1, p2, p3)

#define NT_LOG_WFM_INFO(msg, p1, p2, p3) NT_LOG_MOD_LVL(WFM, INFO, msg, p1, p2, p3)

#define NT_LOG_WFM_WARN(msg, p1, p2, p3) NT_LOG_MOD_LVL(WFM, WARN, msg, p1, p2, p3)

#define NT_LOG_WFM_ERR(msg, p1, p2, p3) NT_LOG_MOD_LVL(WFM, ERR, msg, p1, p2, p3)

#define NT_LOG_WFM_CRIT(msg, p1, p2, p3) NT_LOG_MOD_LVL(WFM, CRIT, msg, p1, p2, p3)

#define NT_LOG_WMI_INFO(msg, p1, p2, p3) NT_LOG_MOD_LVL(WMI, INFO, msg, p1, p2, p3)

#define NT_LOG_WMI_WARN(msg, p1, p2, p3) NT_LOG_MOD_LVL(WMI, WARN, msg, p1, p2, p3)

#define NT_LOG_WMI_ERR(msg, p1, p2, p3) NT_LOG_MOD_LVL(WMI, ERR, msg, p1, p2, p3)

#define NT_LOG_WMI_CRIT(msg, p1, p2, p3) NT_LOG_MOD_LVL(WMI, CRIT, msg, p1, p2, p3)

#define NT_LOG_HALPHY_INFO(msg, p1, p2, p3) NT_LOG_MOD_LVL(HALPHY, INFO, msg, p1, p2, p3)

#define NT_LOG_HALPHY_WARN(msg, p1, p2, p3) NT_LOG_MOD_LVL(HALPHY, WARN, msg, p1, p2, p3)

#define NT_LOG_HALPHY_ERR(msg, p1, p2, p3) NT_LOG_MOD_LVL(HALPHY, ERR, msg, p1, p2, p3)

#define NT_LOG_HALPHY_CRIT(msg, p1, p2, p3) NT_LOG_MOD_LVL(HALPHY, CRIT, msg, p1, p2, p3)

#define NT_LOG_CLI_INFO(msg, p1, p2, p3) NT_LOG_MOD_LVL(CLI, INFO, msg, p1, p2, p3)

#define NT_LOG_CLI_WARN(msg, p1, p2, p3) NT_LOG_MOD_LVL(CLI, WARN, msg, p1, p2, p3)

#define NT_LOG_CLI_ERR(msg, p1, p2, p3) NT_LOG_MOD_LVL(CLI, ERR, msg, p1, p2, p3)

#define NT_LOG_CLI_CRIT(msg, p1, p2, p3) NT_LOG_MOD_LVL(CLI, CRIT, msg, p1, p2, p3)

#define NT_LOG_SYSTEM_INFO(msg, p1, p2, p3) NT_LOG_MOD_LVL(SYSTEM, INFO, msg, p1, p2, p3)

#define NT_LOG_SYSTEM_WARN(msg, p1, p2, p3) NT_LOG_MOD_LVL(SYSTEM, WARN, msg, p1, p2, p3)

#define NT_LOG_SYSTEM_ERR(msg, p1, p2, p3) NT_LOG_MOD_LVL(SYSTEM, ERR, msg, p1, p2, p3)

#define NT_LOG_SYSTEM_CRIT(msg, p1, p2, p3) NT_LOG_MOD_LVL(SYSTEM, CRIT, msg, p1, p2, p3)

#define NT_LOG_WIFI_APP_INFO(msg, p1, p2, p3) NT_LOG_MOD_LVL(WIFI_APP, INFO, msg, p1, p2, p3)

#define NT_LOG_WIFI_APP_WARN(msg, p1, p2, p3) NT_LOG_MOD_LVL(WIFI_APP, WARN, msg, p1, p2, p3)

#define NT_LOG_WIFI_APP_ERR(msg, p1, p2, p3) NT_LOG_MOD_LVL(WIFI_APP, ERR, msg, p1, p2, p3)

#define NT_LOG_WIFI_APP_CRIT(msg, p1, p2, p3) NT_LOG_MOD_LVL(WIFI_APP, CRIT, msg, p1, p2, p3)

#define NT_LOG_COMMON_INFO(msg, p1, p2, p3) NT_LOG_MOD_LVL(COMMON, INFO, msg, p1, p2, p3)

#define NT_LOG_COMMON_WARN(msg, p1, p2, p3) NT_LOG_MOD_LVL(COMMON, WARN, msg, p1, p2, p3)

#define NT_LOG_COMMON_ERR(msg, p1, p2, p3) NT_LOG_MOD_LVL(COMMON, ERR, msg, p1, p2, p3)

#define NT_LOG_COMMON_CRIT(msg, p1, p2, p3) NT_LOG_MOD_LVL(COMMON, CRIT, msg, p1, p2, p3)

#define NT_LOG_SECURITY_INFO(msg, p1, p2, p3) NT_LOG_MOD_LVL(SECURITY, INFO, msg, p1, p2, p3)

#define NT_LOG_SECURITY_WARN(msg, p1, p2, p3) NT_LOG_MOD_LVL(SECURITY, WARN, msg, p1, p2, p3)

#define NT_LOG_SECURITY_ERR(msg, p1, p2, p3) NT_LOG_MOD_LVL(SECURITY, ERR, msg, p1, p2, p3)

#define NT_LOG_SECURITY_CRIT(msg, p1, p2, p3) NT_LOG_MOD_LVL(SECURITY, CRIT, msg, p1, p2, p3)

#define NT_LOG_HAL_API_INFO(msg, p1, p2, p3) NT_LOG_MOD_LVL(HAL_API, INFO, msg, p1, p2, p3)

#define NT_LOG_HAL_API_WARN(msg, p1, p2, p3) NT_LOG_MOD_LVL(HAL_API, WARN, msg, p1, p2, p3)

#define NT_LOG_HAL_API_ERR(msg, p1, p2, p3) NT_LOG_MOD_LVL(HAL_API, ERR, msg, p1, p2, p3)

#define NT_LOG_HAL_API_CRIT(msg, p1, p2, p3) NT_LOG_MOD_LVL(HAL_API, CRIT, msg, p1, p2, p3)

#define NT_LOG_COMMISIONING_APP_INFO(msg, p1, p2, p3) NT_LOG_MOD_LVL(COMMISIONING_APP, INFO, msg, p1, p2, p3)

#define NT_LOG_COMMISIONING_APP_WARN(msg, p1, p2, p3) NT_LOG_MOD_LVL(COMMISIONING_APP, WARN, msg, p1, p2, p3)

#define NT_LOG_COMMISIONING_APP_ERR(msg, p1, p2, p3) NT_LOG_MOD_LVL(COMMISIONING_APP, ERR, msg, p1, p2, p3)

#define NT_LOG_COMMISIONING_APP_CRIT(msg, p1, p2, p3) NT_LOG_MOD_LVL(COMMISIONING_APP, CRIT, msg, p1, p2, p3)

#define NT_LOG_ONBOARDING_APP_INFO(msg, p1, p2, p3) NT_LOG_MOD_LVL(ONBOARDING_APP, INFO, msg, p1, p2, p3)

#define NT_LOG_ONBOARDING_APP_WARN(msg, p1, p2, p3) NT_LOG_MOD_LVL(ONBOARDING_APP, WARN, msg, p1, p2, p3)

#define NT_LOG_ONBOARDING_APP_ERR(msg, p1, p2, p3) NT_LOG_MOD_LVL(ONBOARDING_APP, ERR, msg, p1, p2, p3)

#define NT_LOG_ONBOARDING_APP_CRIT(msg, p1, p2, p3) NT_LOG_MOD_LVL(ONBOARDING_APP, CRIT, msg, p1, p2, p3)

#define NT_LOG_WPS_INFO(msg, p1, p2, p3) NT_LOG_MOD_LVL(WPS, INFO, msg, p1, p2, p3)

#define NT_LOG_WPS_WARN(msg, p1, p2, p3) NT_LOG_MOD_LVL(WPS, WARN, msg, p1, p2, p3)

#define NT_LOG_WPS_ERR(msg, p1, p2, p3) NT_LOG_MOD_LVL(WPS, ERR, msg, p1, p2, p3)

#define NT_LOG_WPS_CRIT(msg, p1, p2, p3) NT_LOG_MOD_LVL(WPS, CRIT, msg, p1, p2, p3)

#define NT_LOG_AWS_INFO(msg, p1, p2, p3) NT_LOG_MOD_LVL(AWS, INFO, msg, p1, p2, p3)

#define NT_LOG_AWS_WARN(msg, p1, p2, p3) NT_LOG_MOD_LVL(AWS, WARN, msg, p1, p2, p3)

#define NT_LOG_AWS_ERR(msg, p1, p2, p3) NT_LOG_MOD_LVL(AWS, ERR, msg, p1, p2, p3)

#define NT_LOG_AWS_CRIT(msg, p1, p2, p3) NT_LOG_MOD_LVL(AWS, CRIT, msg, p1, p2, p3)

#define NT_LOG_PDC_INFO(msg, p1, p2, p3) NT_LOG_MOD_LVL(PDC, INFO, msg, p1, p2, p3)

#define NT_LOG_PDC_WARN(msg, p1, p2, p3) NT_LOG_MOD_LVL(PDC, WARN, msg, p1, p2, p3)

#define NT_LOG_PDC_ERR(msg, p1, p2, p3) NT_LOG_MOD_LVL(PDC, ERR, msg, p1, p2, p3)

#define NT_LOG_PDC_CRIT(msg, p1, p2, p3) NT_LOG_MOD_LVL(PDC, CRIT, msg, p1, p2, p3)

#define NT_LOG_COEX_MOD_INFO(coex_mod_id, MODNAME, LVL, msg, p1, p2, p3)                          \
    if ((NT_STATUS_MOD_##MODNAME == NT_STATUS_MOD_COEX && NT_LOG_LVL_##LVL == NT_LOG_LVL_INFO) && \
        ((gpBtCoexUtilDev && (gpBtCoexUtilDev->dbg_cntrl & (1 << coex_mod_id))))) {               \
        NT_LOG_MOD_LVL(COEX, INFO, msg, p1, p2, p3);                                              \
    }

#define NT_LOG_COEX_INFO(coex_mod_id, msg, p1, p2, p3) NT_LOG_COEX_MOD_INFO(coex_mod_id, COEX, INFO, msg, p1, p2, p3)

#define NT_LOG_COEX_WARN(msg, p1, p2, p3) NT_LOG_MOD_LVL(COEX, WARN, msg, p1, p2, p3)

#define NT_LOG_COEX_ERR(msg, p1, p2, p3) NT_LOG_MOD_LVL(COEX, ERR, msg, p1, p2, p3)

#define NT_LOG_COEX_CRIT(msg, p1, p2, p3) NT_LOG_MOD_LVL(COEX, CRIT, msg, p1, p2, p3)

#define NT_LOG_RCLI_INFO(msg, p1, p2, p3) NT_LOG_MOD_LVL(RCLI, INFO, msg, p1, p2, p3)

#define NT_LOG_RCLI_WARN(msg, p1, p2, p3) NT_LOG_MOD_LVL(RCLI, WARN, msg, p1, p2, p3)

#define NT_LOG_RCLI_ERR(msg, p1, p2, p3) NT_LOG_MOD_LVL(RCLI, ERR, msg, p1, p2, p3)

#define NT_LOG_RCLI_CRIT(msg, p1, p2, p3) NT_LOG_MOD_LVL(RCLI, CRIT, msg, p1, p2, p3)

#define NT_LOG_HOSTED_APP_INFO(msg, p1, p2, p3) NT_LOG_MOD_LVL(HOSTED_APP, INFO, msg, p1, p2, p3)

#define NT_LOG_HOSTED_APP_WARN(msg, p1, p2, p3) NT_LOG_MOD_LVL(HOSTED_APP, WARN, msg, p1, p2, p3)

#define NT_LOG_HOSTED_APP_ERR(msg, p1, p2, p3) NT_LOG_MOD_LVL(HOSTED_APP, ERR, msg, p1, p2, p3)

#define NT_LOG_HOSTED_APP_CRIT(msg, p1, p2, p3) NT_LOG_MOD_LVL(HOSTED_APP, CRIT, msg, p1, p2, p3)

#define NT_LOG_TLS_INFO(msg, p1, p2, p3) NT_LOG_MOD_LVL(TLS, INFO, msg, p1, p2, p3)

#define NT_LOG_TLS_WARN(msg, p1, p2, p3) NT_LOG_MOD_LVL(TLS, WARN, msg, p1, p2, p3)

#define NT_LOG_TLS_ERR(msg, p1, p2, p3) NT_LOG_MOD_LVL(TLS, ERR, msg, p1, p2, p3)

#define NT_LOG_TLS_CRIT(msg, p1, p2, p3) NT_LOG_MOD_LVL(TLS, CRIT, msg, p1, p2, p3)

#define NT_LOG_DEV_CFG_INFO(msg, p1, p2, p3) NT_LOG_MOD_LVL(DEV_CFG, INFO, msg, p1, p2, p3)

#define NT_LOG_DEV_CFG_WARN(msg, p1, p2, p3) NT_LOG_MOD_LVL(DEV_CFG, WARN, msg, p1, p2, p3)

#define NT_LOG_DEV_CFG_ERR(msg, p1, p2, p3) NT_LOG_MOD_LVL(DEV_CFG, ERR, msg, p1, p2, p3)

#define NT_LOG_DEV_CFG_CRIT(msg, p1, p2, p3) NT_LOG_MOD_LVL(DEV_CFG, CRIT, msg, p1, p2, p3)

#define NT_LOG_RA_INFO(msg, p1, p2, p3) NT_LOG_MOD_LVL(RA, INFO, msg, p1, p2, p3)

#define NT_LOG_RA_WARN(msg, p1, p2, p3) NT_LOG_MOD_LVL(RA, WARN, msg, p1, p2, p3)

#define NT_LOG_RA_ERR(msg, p1, p2, p3) NT_LOG_MOD_LVL(RA, ERR, msg, p1, p2, p3)

#define NT_LOG_RA_CRIT(msg, p1, p2, p3) NT_LOG_MOD_LVL(RA, CRIT, msg, p1, p2, p3)

#define NT_LOG_SOCPM_INFO(msg, p1, p2, p3) NT_LOG_MOD_LVL(SOCPM, INFO, msg, p1, p2, p3)

#define NT_LOG_SOCPM_WARN(msg, p1, p2, p3) NT_LOG_MOD_LVL(SOCPM, WARN, msg, p1, p2, p3)

#define NT_LOG_SOCPM_ERR(msg, p1, p2, p3) NT_LOG_MOD_LVL(SOCPM, ERR, msg, p1, p2, p3)

#define NT_LOG_SOCPM_CRIT(msg, p1, p2, p3) NT_LOG_MOD_LVL(SOCPM, CRIT, msg, p1, p2, p3)

#define NT_LOG_WPM_INFO(msg, p1, p2, p3) NT_LOG_MOD_LVL(WPM, INFO, msg, p1, p2, p3)

#define NT_LOG_WPM_WARN(msg, p1, p2, p3) NT_LOG_MOD_LVL(WPM, WARN, msg, p1, p2, p3)

#define NT_LOG_WPM_ERR(msg, p1, p2, p3) NT_LOG_MOD_LVL(WPM, ERR, msg, p1, p2, p3)

#define NT_LOG_WPM_CRIT(msg, p1, p2, p3) NT_LOG_MOD_LVL(WPM, CRIT, msg, p1, p2, p3)

#define NT_LOG_ANI_INFO(msg, p1, p2, p3) NT_LOG_MOD_LVL(ANI, INFO, msg, p1, p2, p3)

#define NT_LOG_ANI_WARN(msg, p1, p2, p3) NT_LOG_MOD_LVL(ANI, WARN, msg, p1, p2, p3)

#define NT_LOG_ANI_ERR(msg, p1, p2, p3) NT_LOG_MOD_LVL(ANI, ERR, msg, p1, p2, p3)

#define NT_LOG_ANI_CRIT(msg, p1, p2, p3) NT_LOG_MOD_LVL(ANI, CRIT, msg, p1, p2, p3)

#define NT_LOG_FTM_INFO(msg, p1, p2, p3) NT_LOG_MOD_LVL(FTM, INFO, msg, p1, p2, p3)

#define NT_LOG_FTM_WARN(msg, p1, p2, p3) NT_LOG_MOD_LVL(FTM, WARN, msg, p1, p2, p3)

#define NT_LOG_FTM_ERR(msg, p1, p2, p3) NT_LOG_MOD_LVL(FTM, ERR, msg, p1, p2, p3)

#define NT_LOG_FTM_CRIT(msg, p1, p2, p3) NT_LOG_MOD_LVL(FTM, CRIT, msg, p1, p2, p3)

#else

/**
 * @Function    :  nt_log_sub_mod_info
 * @Description :  Check if sub module is enabled for logging
 */
uint8_t nt_log_sub_mod_info(uint8_t mod_id, uint8_t loglvl, uint16_t sub_mod_id, const char *msg, ...);

/**
 * @FUNCTION    :  nt_log_write
 * @Description :  check the buffer size and log level and module id,pass the data to buffer.
 * @Param       :  log level(0 - 3):  like info,warn,err,critical.
 *                 moduleid        : (8 - bit) module id
                   fn              : for function name.
                   ln              : for line number.
                   msg             :message
 * @return      :  Success/FAILURE/nt_err_loglvl/nt_err_buff_size.
 */

uint8_t nt_log_write(uint8_t mod_id, uint8_t loglvl,
#if (NT_FN_FUNCTION_LINE_NUM_FLAG == 1)
                     char *fn, uint16_t ln,
#endif
                     const char *msg, ...);

#define NT_LOG_MOD_LVL(MODNAME, LVL, msg, ...) \
    nt_log_write(NT_STATUS_MOD_##MODNAME, NT_LOG_LVL_##LVL, NT_LOG_FILE_LOC msg, ##__VA_ARGS__)

#define NT_LOG_HAL_INFO(msg, ...) NT_LOG_MOD_LVL(HAL, INFO, msg, ##__VA_ARGS__)

#define NT_LOG_HAL_WARN(msg, ...) NT_LOG_MOD_LVL(HAL, WARN, msg, ##__VA_ARGS__)

#define NT_LOG_HAL_ERR(msg, ...) NT_LOG_MOD_LVL(HAL, ERR, msg, ##__VA_ARGS__)

#define NT_LOG_HAL_CRIT(msg, ...) NT_LOG_MOD_LVL(HAL, CRIT, msg, ##__VA_ARGS__)

#define NT_LOG_SME_INFO(msg, ...) NT_LOG_MOD_LVL(SME, INFO, msg, ##__VA_ARGS__)

#define NT_LOG_SME_WARN(msg, ...) NT_LOG_MOD_LVL(SME, WARN, msg, ##__VA_ARGS__)

#define NT_LOG_SME_ERR(msg, ...) NT_LOG_MOD_LVL(SME, ERR, msg, ##__VA_ARGS__)

#define NT_LOG_SME_CRIT(msg, ...) NT_LOG_MOD_LVL(SME, CRIT, msg, ##__VA_ARGS__)

#define NT_LOG_MLM_INFO(msg, ...) NT_LOG_MOD_LVL(MLM, INFO, msg, ##__VA_ARGS__)

#define NT_LOG_MLM_WARN(msg, ...) NT_LOG_MOD_LVL(MLM, WARN, msg, ##__VA_ARGS__)

#define NT_LOG_MLM_ERR(msg, ...) NT_LOG_MOD_LVL(MLM, ERR, msg, ##__VA_ARGS__)

#define NT_LOG_MLM_CRIT(msg, ...) NT_LOG_MOD_LVL(MLM, CRIT, msg, ##__VA_ARGS__)

#define NT_LOG_DPM_INFO(msg, ...) NT_LOG_MOD_LVL(DPM, INFO, msg, ##__VA_ARGS__)

#define NT_LOG_DPM_WARN(msg, ...) NT_LOG_MOD_LVL(DPM, WARN, msg, ##__VA_ARGS__)

#define NT_LOG_DPM_ERR(msg, ...) NT_LOG_MOD_LVL(DPM, ERR, msg, ##__VA_ARGS__)

#define NT_LOG_DPM_CRIT(msg, ...) NT_LOG_MOD_LVL(DPM, CRIT, msg, ##__VA_ARGS__)

#define NT_LOG_WFM_INFO(msg, ...) NT_LOG_MOD_LVL(WFM, INFO, msg, ##__VA_ARGS__)

#define NT_LOG_WFM_WARN(msg, ...) NT_LOG_MOD_LVL(WFM, WARN, msg, ##__VA_ARGS__)

#define NT_LOG_WFM_ERR(msg, ...) NT_LOG_MOD_LVL(WFM, ERR, msg, ##__VA_ARGS__)

#define NT_LOG_WFM_CRIT(msg, ...) NT_LOG_MOD_LVL(WFM, CRIT, msg, ##__VA_ARGS__)

#define NT_LOG_WMI_INFO(msg, ...) NT_LOG_MOD_LVL(WMI, INFO, msg, ##__VA_ARGS__)

#define NT_LOG_WMI_WARN(msg, ...) NT_LOG_MOD_LVL(WMI, WARN, msg, ##__VA_ARGS__)

#define NT_LOG_WMI_ERR(msg, ...) NT_LOG_MOD_LVL(WMI, ERR, msg, ##__VA_ARGS__)

#define NT_LOG_WMI_CRIT(msg, ...) NT_LOG_MOD_LVL(WMI, CRIT, msg, ##__VA_ARGS__)

#define NT_LOG_HALPHY_INFO(msg, ...) NT_LOG_MOD_LVL(HALPHY, INFO, msg, ##__VA_ARGS__)

#define NT_LOG_HALPHY_WARN(msg, ...) NT_LOG_MOD_LVL(HALPHY, WARN, msg, ##__VA_ARGS__)

#define NT_LOG_HALPHY_ERR(msg, ...) NT_LOG_MOD_LVL(HALPHY, ERR, msg, ##__VA_ARGS__)

#define NT_LOG_HALPHY_CRIT(msg, ...) NT_LOG_MOD_LVL(HALPHY, CRIT, msg, ##__VA_ARGS__)

#define NT_LOG_CLI_INFO(msg, ...) NT_LOG_MOD_LVL(CLI, INFO, msg, ##__VA_ARGS__)

#define NT_LOG_CLI_WARN(msg, ...) NT_LOG_MOD_LVL(CLI, WARN, msg, ##__VA_ARGS__)

#define NT_LOG_CLI_ERR(msg, ...) NT_LOG_MOD_LVL(CLI, ERR, msg, ##__VA_ARGS__)

#define NT_LOG_CLI_CRIT(msg, ...) NT_LOG_MOD_LVL(CLI, CRIT, msg, ##__VA_ARGS__)

#define NT_LOG_SYSTEM_INFO(msg, ...) NT_LOG_MOD_LVL(SYSTEM, INFO, msg, ##__VA_ARGS__)

#define NT_LOG_SYSTEM_WARN(msg, ...) NT_LOG_MOD_LVL(SYSTEM, WARN, msg, ##__VA_ARGS__)

#define NT_LOG_SYSTEM_ERR(msg, ...) NT_LOG_MOD_LVL(SYSTEM, ERR, msg, ##__VA_ARGS__)

#define NT_LOG_SYSTEM_CRIT(msg, ...) NT_LOG_MOD_LVL(SYSTEM, CRIT, msg, ##__VA_ARGS__)

#define NT_LOG_WIFI_APP_INFO(msg, ...) NT_LOG_MOD_LVL(WIFI_APP, INFO, msg, ##__VA_ARGS__)

#define NT_LOG_WIFI_APP_WARN(msg, ...) NT_LOG_MOD_LVL(WIFI_APP, WARN, msg, ##__VA_ARGS__)

#define NT_LOG_WIFI_APP_ERR(msg, ...) NT_LOG_MOD_LVL(WIFI_APP, ERR, msg, ##__VA_ARGS__)

#define NT_LOG_WIFI_APP_CRIT(msg, ...) NT_LOG_MOD_LVL(WIFI_APP, CRIT, msg, ##__VA_ARGS__)

#define NT_LOG_COMMON_INFO(msg, ...) NT_LOG_MOD_LVL(COMMON, INFO, msg, ##__VA_ARGS__)

#define NT_LOG_COMMON_WARN(msg, ...) NT_LOG_MOD_LVL(COMMON, WARN, msg, ##__VA_ARGS__)

#define NT_LOG_COMMON_ERR(msg, ...) NT_LOG_MOD_LVL(COMMON, ERR, msg, ##__VA_ARGS__)

#define NT_LOG_COMMON_CRIT(msg, ...) NT_LOG_MOD_LVL(COMMON, CRIT, msg, ##__VA_ARGS__)

#define NT_LOG_SECURITY_INFO(msg, ...) NT_LOG_MOD_LVL(SECURITY, INFO, msg, ##__VA_ARGS__)

#define NT_LOG_SECURITY_WARN(msg, ...) NT_LOG_MOD_LVL(SECURITY, WARN, msg, ##__VA_ARGS__)

#define NT_LOG_SECURITY_ERR(msg, ...) NT_LOG_MOD_LVL(SECURITY, ERR, msg, ##__VA_ARGS__)

#define NT_LOG_SECURITY_CRIT(msg, ...) NT_LOG_MOD_LVL(SECURITY, CRIT, msg, ##__VA_ARGS__)

#define NT_LOG_HAL_API_INFO(msg, ...) NT_LOG_MOD_LVL(HAL_API, INFO, msg, ##__VA_ARGS__)

#define NT_LOG_HAL_API_WARN(msg, ...) NT_LOG_MOD_LVL(HAL_API, WARN, msg, ##__VA_ARGS__)

#define NT_LOG_HAL_API_ERR(msg, ...) NT_LOG_MOD_LVL(HAL_API, ERR, msg, ##__VA_ARGS__)

#define NT_LOG_HAL_API_CRIT(msg, ...) NT_LOG_MOD_LVL(HAL_API, CRIT, msg, ##__VA_ARGS__)

#define NT_LOG_COMMISIONING_APP_INFO(msg, ...) NT_LOG_MOD_LVL(COMMISIONING_APP, INFO, msg, ##__VA_ARGS__)

#define NT_LOG_COMMISIONING_APP_WARN(msg, ...) NT_LOG_MOD_LVL(COMMISIONING_APP, WARN, msg, ##__VA_ARGS__)

#define NT_LOG_COMMISIONING_APP_ERR(msg, ...) NT_LOG_MOD_LVL(COMMISIONING_APP, ERR, msg, ##__VA_ARGS__)

#define NT_LOG_COMMISIONING_APP_CRIT(msg, ...) NT_LOG_MOD_LVL(COMMISIONING_APP, CRIT, msg, ##__VA_ARGS__)

#define NT_LOG_ONBOARDING_APP_INFO(msg, ...) NT_LOG_MOD_LVL(ONBOARDING_APP, INFO, msg, ##__VA_ARGS__)

#define NT_LOG_ONBOARDING_APP_WARN(msg, ...) NT_LOG_MOD_LVL(ONBOARDING_APP, WARN, msg, ##__VA_ARGS__)

#define NT_LOG_ONBOARDING_APP_ERR(msg, ...) NT_LOG_MOD_LVL(ONBOARDING_APP, ERR, msg, ##__VA_ARGS__)

#define NT_LOG_ONBOARDING_APP_CRIT(msg, ...) NT_LOG_MOD_LVL(ONBOARDING_APP, CRIT, msg, ##__VA_ARGS__)

#define NT_LOG_WPS_INFO(msg, ...) NT_LOG_MOD_LVL(WPS, INFO, msg, ##__VA_ARGS__)

#define NT_LOG_WPS_WARN(msg, ...) NT_LOG_MOD_LVL(WPS, WARN, msg, ##__VA_ARGS__)

#define NT_LOG_WPS_ERR(msg, ...) NT_LOG_MOD_LVL(WPS, ERR, msg, ##__VA_ARGS__)

#define NT_LOG_WPS_CRIT(msg, ...) NT_LOG_MOD_LVL(WPS, CRIT, msg, ##__VA_ARGS__)

#define NT_LOG_AWS_INFO(msg, ...) NT_LOG_MOD_LVL(AWS, INFO, msg, ##__VA_ARGS__)

#define NT_LOG_AWS_WARN(msg, ...) NT_LOG_MOD_LVL(AWS, WARN, msg, ##__VA_ARGS__)

#define NT_LOG_AWS_ERR(msg, ...) NT_LOG_MOD_LVL(AWS, ERR, msg, ##__VA_ARGS__)

#define NT_LOG_AWS_CRIT(msg, ...) NT_LOG_MOD_LVL(AWS, CRIT, msg, ##__VA_ARGS__)

#define NT_LOG_COEX_MOD_INFO(coex_mod_id, MODNAME, LVL, msg, ...)                  \
    if ((mod_id == NT_STATUS_MOD_COEX && loglvl == NT_LOG_LVL_INFO) &&             \
        ((gpBtCoexUtilDev && (gpBtCoexUtilDev->dbg_cntrl & (1 << sub_mod_id))))) { \
        NT_LOG_MOD_LVL(COEX, INFO, NT_LOG_FILE_LOC, msg, ...);                     \
    }

#define NT_LOG_COEX_INFO(msg, ...) NT_LOG_COEX_MOD_INFO(coex_mod_id, COEX, INFO, msg, ##__VA_ARGS__)

#define NT_LOG_COEX_WARN(msg, ...) NT_LOG_MOD_LVL(COEX, WARN, msg, ##__VA_ARGS__)

#define NT_LOG_COEX_ERR(msg, ...) NT_LOG_MOD_LVL(COEX, ERR, msg, ##__VA_ARGS__)

#define NT_LOG_COEX_CRIT(msg, ...) NT_LOG_MOD_LVL(COEX, CRIT, msg, ##__VA_ARGS__)

#define NT_LOG_RCLI_INFO(msg, ...) NT_LOG_MOD_LVL(RCLI, INFO, msg, ##__VA_ARGS__)

#define NT_LOG_RCLI_WARN(msg, ...) NT_LOG_MOD_LVL(RCLI, WARN, msg, ##__VA_ARGS__)

#define NT_LOG_RCLI_ERR(msg, ...) NT_LOG_MOD_LVL(RCLI, ERR, msg, ##__VA_ARGS__)

#define NT_LOG_RCLI_CRIT(msg, ...) NT_LOG_MOD_LVL(RCLI, CRIT, msg, ##__VA_ARGS__)

#define NT_LOG_HOSTED_APP_INFO(msg, ...) NT_LOG_MOD_LVL(HOSTED_APP, INFO, msg, ##__VA_ARGS__)

#define NT_LOG_HOSTED_APP_WARN(msg, ...) NT_LOG_MOD_LVL(HOSTED_APP, WARN, msg, ##__VA_ARGS__)

#define NT_LOG_HOSTED_APP_ERR(msg, ...) NT_LOG_MOD_LVL(HOSTED_APP, ERR, msg, ##__VA_ARGS__)

#define NT_LOG_HOSTED_APP_CRIT(msg, ...) NT_LOG_MOD_LVL(HOSTED_APP, CRIT, msg, ##__VA_ARGS__)

#define NT_LOG_TLS_INFO(msg, ...) NT_LOG_MOD_LVL(TLS, INFO, msg, ##__VA_ARGS__)

#define NT_LOG_TLS_WARN(msg, ...) NT_LOG_MOD_LVL(TLS, WARN, msg, ##__VA_ARGS__)

#define NT_LOG_TLS_ERR(msg, ...) NT_LOG_MOD_LVL(TLS, ERR, msg, ##__VA_ARGS__)

#define NT_LOG_TLS_CRIT(msg, ...) NT_LOG_MOD_LVL(TLS, CRIT, msg, ##__VA_ARGS__)

#define NT_LOG_DEV_CFG_INFO(msg, ...) NT_LOG_MOD_LVL(DEV_CFG, INFO, msg, ##__VA_ARGS__)

#define NT_LOG_DEV_CFG_WARN(msg, ...) NT_LOG_MOD_LVL(DEV_CFG, WARN, msg, ##__VA_ARGS__)

#define NT_LOG_DEV_CFG_ERR(msg, ...) NT_LOG_MOD_LVL(DEV_CFG, ERR, msg, ##__VA_ARGS__)

#define NT_LOG_DEV_CFG_CRIT(msg, ...) NT_LOG_MOD_LVL(DEV_CFG, CRIT, msg, ##__VA_ARGS__)

#define NT_LOG_RA_INFO(msg, ...) NT_LOG_MOD_LVL(RA, INFO, msg, ##__VA_ARGS__)

#define NT_LOG_RA_WARN(msg, ...) NT_LOG_MOD_LVL(RA, WARN, msg, ##__VA_ARGS__)

#define NT_LOG_RA_ERR(msg, ...) NT_LOG_MOD_LVL(RA, ERR, msg, ##__VA_ARGS__)

#define NT_LOG_RA_CRIT(msg, ...) NT_LOG_MOD_LVL(RA, CRIT, msg, ##__VA_ARGS__)

#define NT_LOG_SOCPM_INFO(msg, ...) NT_LOG_MOD_LVL(SOCPM, INFO, msg, ##__VA_ARGS__)

#define NT_LOG_SOCPM_WARN(msg, ...) NT_LOG_MOD_LVL(SOCPM, WARN, msg, ##__VA_ARGS__)

#define NT_LOG_SOCPM_ERR(msg, ...) NT_LOG_MOD_LVL(SOCPM, ERR, msg, ##__VA_ARGS__)

#define NT_LOG_SOCPM_CRIT(msg, ...) NT_LOG_MOD_LVL(SOCPM, CRIT, msg, ##__VA_ARGS__)

#define NT_LOG_WPM_INFO(msg, ...) NT_LOG_MOD_LVL(WPM, INFO, msg, ##__VA_ARGS__)

#define NT_LOG_WPM_WARN(msg, ...) NT_LOG_MOD_LVL(WPM, WARN, msg, ##__VA_ARGS__)

#define NT_LOG_WPM_ERR(msg, ...) NT_LOG_MOD_LVL(WPM, ERR, msg, ##__VA_ARGS__)

#define NT_LOG_WPM_CRIT(msg, ...) NT_LOG_MOD_LVL(WPM, CRIT, msg, ##__VA_ARGS__)

#define NT_LOG_ANI_INFO(msg, ...) NT_LOG_MOD_LVL(ANI, INFO, msg, ##__VA_ARGS__)

#define NT_LOG_ANI_WARN(msg, ...) NT_LOG_MOD_LVL(ANI, WARN, msg, ##__VA_ARGS__)

#define NT_LOG_ANI_ERR(msg, ...) NT_LOG_MOD_LVL(ANI, ERR, msg, ##__VA_ARGS__)

#define NT_LOG_ANI_CRIT(msg, ...) NT_LOG_MOD_LVL(ANI, CRIT, msg, ##__VA_ARGS__)

#endif  // NT_FN_FIXED_ARG_LOGGER

#ifdef SUPPORT_FERMION_LOGGER
#define NT_LOG_DYN_STR_HAL_INFO(msg, ptr) NT_LOG_DYNAMIC_STRING(HAL, INFO, msg, ptr)

#define NT_LOG_DYN_STR_HAL_WARN(msg, ptr) NT_LOG_DYNAMIC_STRING(HAL, WARN, msg, ptr)

#define NT_LOG_DYN_STR_HAL_ERR(msg, ptr) NT_LOG_DYNAMIC_STRING(HAL, ERR, msg, ptr)

#define NT_LOG_DYN_STR_HAL_CRIT(msg, ptr) NT_LOG_DYNAMIC_STRING(HAL, CRIT, msg, ptr)

#define NT_LOG_DYN_STR_SME_INFO(msg, ptr) NT_LOG_DYNAMIC_STRING(SME, INFO, msg, ptr)

#define NT_LOG_DYN_STR_SME_WARN(msg, ptr) NT_LOG_DYNAMIC_STRING(SME, WARN, msg, ptr)

#define NT_LOG_DYN_STR_SME_ERR(msg, ptr) NT_LOG_DYNAMIC_STRING(SME, ERR, msg, ptr)

#define NT_LOG_DYN_STR_SME_CRIT(msg, ptr) NT_LOG_DYNAMIC_STRING(SME, CRIT, msg, ptr)

#define NT_LOG_DYN_STR_MLM_INFO(msg, ptr) NT_LOG_DYNAMIC_STRING(MLM, INFO, msg, ptr)

#define NT_LOG_DYN_STR_MLM_WARN(msg, ptr) NT_LOG_DYNAMIC_STRING(MLM, WARN, msg, ptr)

#define NT_LOG_DYN_STR_MLM_ERR(msg, ptr) NT_LOG_DYNAMIC_STRING(MLM, ERR, msg, ptr)

#define NT_LOG_DYN_STR_MLM_CRIT(msg, ptr) NT_LOG_DYNAMIC_STRING(MLM, CRIT, msg, ptr)

#define NT_LOG_DYN_STR_DPM_INFO(msg, ptr) NT_LOG_DYNAMIC_STRING(DPM, INFO, msg, ptr)

#define NT_LOG_DYN_STR_DPM_WARN(msg, ptr) NT_LOG_DYNAMIC_STRING(DPM, WARN, msg, ptr)

#define NT_LOG_DYN_STR_DPM_ERR(msg, ptr) NT_LOG_DYNAMIC_STRING(DPM, ERR, msg, ptr)

#define NT_LOG_DYN_STR_DPM_CRIT(msg, ptr) NT_LOG_DYNAMIC_STRING(DPM, CRIT, msg, ptr)

#define NT_LOG_DYN_STR_WFM_INFO(msg, ptr) NT_LOG_DYNAMIC_STRING(WFM, INFO, msg, ptr)

#define NT_LOG_DYN_STR_WFM_WARN(msg, ptr) NT_LOG_DYNAMIC_STRING(WFM, WARN, msg, ptr)

#define NT_LOG_DYN_STR_WFM_ERR(msg, ptr) NT_LOG_DYNAMIC_STRING(WFM, ERR, msg, ptr)

#define NT_LOG_DYN_STR_WFM_CRIT(msg, ptr) NT_LOG_DYNAMIC_STRING(WFM, CRIT, msg, ptr)

#define NT_LOG_DYN_STR_WMI_INFO(msg, ptr) NT_LOG_DYNAMIC_STRING(WMI, INFO, msg, ptr)

#define NT_LOG_DYN_STR_WMI_WARN(msg, ptr) NT_LOG_DYNAMIC_STRING(WMI, WARN, msg, ptr)

#define NT_LOG_DYN_STR_WMI_ERR(msg, ptr) NT_LOG_DYNAMIC_STRING(WMI, ERR, msg, ptr)

#define NT_LOG_DYN_STR_WMI_CRIT(msg, ptr) NT_LOG_DYNAMIC_STRING(WMI, CRIT, msg, ptr)

#define NT_LOG_DYN_STR_HALPHY_INFO(msg, ptr) NT_LOG_DYNAMIC_STRING(HALPHY, INFO, msg, ptr)

#define NT_LOG_DYN_STR_HALPHY_WARN(msg, ptr) NT_LOG_DYNAMIC_STRING(HALPHY, WARN, msg, ptr)

#define NT_LOG_DYN_STR_HALPHY_ERR(msg, ptr) NT_LOG_DYNAMIC_STRING(HALPHY, ERR, msg, ptr)

#define NT_LOG_DYN_STR_HALPHY_CRIT(msg, ptr) NT_LOG_DYNAMIC_STRING(HALPHY, CRIT, msg, ptr)

#define NT_LOG_DYN_STR_ANI_INFO(msg, ptr) NT_LOG_DYNAMIC_STRING(ANI, INFO, msg, ptr)

#define NT_LOG_DYN_STR_ANI_WARN(msg, ptr) NT_LOG_DYNAMIC_STRING(ANI, WARN, msg, ptr)

#define NT_LOG_DYN_STR_ANI_ERR(msg, ptr) NT_LOG_DYNAMIC_STRING(ANI, ERR, msg, ptr)

#define NT_LOG_DYN_STR_ANI_CRIT(msg, ptr) NT_LOG_DYNAMIC_STRING(ANI, CRIT, msg, ptr)

#define NT_LOG_DYN_STR_CLI_INFO(msg, ptr) NT_LOG_DYNAMIC_STRING(CLI, INFO, msg, ptr)

#define NT_LOG_DYN_STR_CLI_WARN(msg, ptr) NT_LOG_DYNAMIC_STRING(CLI, WARN, msg, ptr)

#define NT_LOG_DYN_STR_CLI_ERR(msg, ptr) NT_LOG_DYNAMIC_STRING(CLI, ERR, msg, ptr)

#define NT_LOG_DYN_STR_CLI_CRIT(msg, ptr) NT_LOG_DYNAMIC_STRING(CLI, CRIT, msg, ptr)

#define NT_LOG_DYN_STR_SYSTEM_INFO(msg, ptr) NT_LOG_DYNAMIC_STRING(SYSTEM, INFO, msg, ptr)

#define NT_LOG_DYN_STR_SYSTEM_WARN(msg, ptr) NT_LOG_DYNAMIC_STRING(SYSTEM, WARN, msg, ptr)

#define NT_LOG_DYN_STR_SYSTEM_ERR(msg, ptr) NT_LOG_DYNAMIC_STRING(SYSTEM, ERR, msg, ptr)

#define NT_LOG_DYN_STR_SYSTEM_CRIT(msg, ptr) NT_LOG_DYNAMIC_STRING(SYSTEM, CRIT, msg, ptr)

#define NT_LOG_DYN_STR_WIFI_APP_INFO(msg, ptr) NT_LOG_DYNAMIC_STRING(WIFI_APP, INFO, msg, ptr)

#define NT_LOG_DYN_STR_WIFI_APP_WARN(msg, ptr) NT_LOG_DYNAMIC_STRING(WIFI_APP, WARN, msg, ptr)

#define NT_LOG_DYN_STR_WIFI_APP_ERR(msg, ptr) NT_LOG_DYNAMIC_STRING(WIFI_APP, ERR, msg, ptr)

#define NT_LOG_DYN_STR_WIFI_APP_CRIT(msg, ptr) NT_LOG_DYNAMIC_STRING(WIFI_APP, CRIT, msg, ptr)

#define NT_LOG_DYN_STR_COMMON_INFO(msg, ptr) NT_LOG_DYNAMIC_STRING(COMMON, INFO, msg, ptr)

#define NT_LOG_DYN_STR_COMMON_WARN(msg, ptr) NT_LOG_DYNAMIC_STRING(COMMON, WARN, msg, ptr)

#define NT_LOG_DYN_STR_COMMON_ERR(msg, ptr) NT_LOG_DYNAMIC_STRING(COMMON, ERR, msg, ptr)

#define NT_LOG_DYN_STR_COMMON_CRIT(msg, ptr) NT_LOG_DYNAMIC_STRING(COMMON, CRIT, msg, ptr)

#define NT_LOG_DYN_STR_SECURITY_INFO(msg, ptr) NT_LOG_DYNAMIC_STRING(SECURITY, INFO, msg, ptr)

#define NT_LOG_DYN_STR_SECURITY_WARN(msg, ptr) NT_LOG_DYNAMIC_STRING(SECURITY, WARN, msg, ptr)

#define NT_LOG_DYN_STR_SECURITY_ERR(msg, ptr) NT_LOG_DYNAMIC_STRING(SECURITY, ERR, msg, ptr)

#define NT_LOG_DYN_STR_SECURITY_CRIT(msg, ptr) NT_LOG_DYNAMIC_STRING(SECURITY, CRIT, msg, ptr)

#define NT_LOG_DYN_STR_HAL_API_INFO(msg, ptr) NT_LOG_DYNAMIC_STRING(HAL_API, INFO, msg, ptr)

#define NT_LOG_DYN_STR_HAL_API_WARN(msg, ptr) NT_LOG_DYNAMIC_STRING(HAL_API, WARN, msg, ptr)

#define NT_LOG_DYN_STR_HAL_API_ERR(msg, ptr) NT_LOG_DYNAMIC_STRING(HAL_API, ERR, msg, ptr)

#define NT_LOG_DYN_STR_HAL_API_CRIT(msg, ptr) NT_LOG_DYNAMIC_STRING(HAL_API, CRIT, msg, ptr)

#define NT_LOG_DYN_STR_COMMISIONING_APP_INFO(msg, ptr) NT_LOG_DYNAMIC_STRING(COMMISIONING_APP, INFO, msg, ptr)

#define NT_LOG_DYN_STR_COMMISIONING_APP_WARN(msg, ptr) NT_LOG_DYNAMIC_STRING(COMMISIONING_APP, WARN, msg, ptr)

#define NT_LOG_DYN_STR_COMMISIONING_APP_ERR(msg, ptr) NT_LOG_DYNAMIC_STRING(COMMISIONING_APP, ERR, msg, ptr)

#define NT_LOG_DYN_STR_COMMISIONING_APP_CRIT(msg, ptr) NT_LOG_DYNAMIC_STRING(COMMISIONING_APP, CRIT, msg, ptr)

#define NT_LOG_DYN_STR_ONBOARDING_APP_INFO(msg, ptr) NT_LOG_DYNAMIC_STRING(ONBOARDING_APP, INFO, msg, ptr)

#define NT_LOG_DYN_STR_ONBOARDING_APP_WARN(msg, ptr) NT_LOG_DYNAMIC_STRING(ONBOARDING_APP, WARN, msg, ptr)

#define NT_LOG_DYN_STR_ONBOARDING_APP_ERR(msg, ptr) NT_LOG_DYNAMIC_STRING(ONBOARDING_APP, ERR, msg, ptr)

#define NT_LOG_DYN_STR_ONBOARDING_APP_CRIT(msg, ptr) NT_LOG_DYNAMIC_STRING(ONBOARDING_APP, CRIT, msg, ptr)

#define NT_LOG_DYN_STR_WPS_INFO(msg, ptr) NT_LOG_DYNAMIC_STRING(WPS, INFO, msg, ptr)

#define NT_LOG_DYN_STR_WPS_WARN(msg, ptr) NT_LOG_DYNAMIC_STRING(WPS, WARN, msg, ptr)

#define NT_LOG_DYN_STR_WPS_ERR(msg, ptr) NT_LOG_DYNAMIC_STRING(WPS, ERR, msg, ptr)

#define NT_LOG_DYN_STR_WPS_CRIT(msg, ptr) NT_LOG_DYNAMIC_STRING(WPS, CRIT, msg, ptr)

#define NT_LOG_DYN_STR_AWS_INFO(msg, ptr) NT_LOG_DYNAMIC_STRING(AWS, INFO, msg, ptr)

#define NT_LOG_DYN_STR_AWS_WARN(msg, ptr) NT_LOG_DYNAMIC_STRING(AWS, WARN, msg, ptr)

#define NT_LOG_DYN_STR_AWS_ERR(msg, ptr) NT_LOG_DYNAMIC_STRING(AWS, ERR, msg, ptr)

#define NT_LOG_DYN_STR_AWS_CRIT(msg, ptr) NT_LOG_DYNAMIC_STRING(AWS, CRIT, msg, ptr)

#define NT_LOG_DYN_STR_COEX_INFO(msg, ptr) NT_LOG_DYNAMIC_STRING(COEX, INFO, msg, ptr)

#define NT_LOG_DYN_STR_COEX_WARN(msg, ptr) NT_LOG_DYNAMIC_STRING(COEX, WARN, msg, ptr)

#define NT_LOG_DYN_STR_COEX_ERR(msg, ptr) NT_LOG_DYNAMIC_STRING(COEX, ERR, msg, ptr)

#define NT_LOG_DYN_STR_COEX_CRIT(msg, ptr) NT_LOG_DYNAMIC_STRING(COEX, CRIT, msg, ptr)

#define NT_LOG_DYN_STR_RCLI_INFO(msg, ...) NT_LOG_DYNAMIC_STRING(RCLI, INFO, msg, ##__VA_ARGS__)

#define NT_LOG_DYN_STR_RCLI_WARN(msg, ...) NT_LOG_DYNAMIC_STRING(RCLI, WARN, msg, ##__VA_ARGS__)

#define NT_LOG_DYN_STR_RCLI_ERR(msg, ...) NT_LOG_DYNAMIC_STRING(RCLI, ERR, msg, ##__VA_ARGS__)

#define NT_LOG_DYN_STR_RCLI_CRIT(msg, ...) NT_LOG_DYNAMIC_STRING(RCLI, CRIT, msg, ##__VA_ARGS__)

#define NT_LOG_DYN_STR_HOSTED_APP_INFO(msg, ...) NT_LOG_DYNAMIC_STRING(HOSTED_APP, INFO, msg, ##__VA_ARGS__)

#define NT_LOG_DYN_STR_HOSTED_APP_WARN(msg, ...) NT_LOG_DYNAMIC_STRING(HOSTED_APP, WARN, msg, ##__VA_ARGS__)

#define NT_LOG_DYN_STR_HOSTED_APP_ERR(msg, ...) NT_LOG_DYNAMIC_STRING(HOSTED_APP, ERR, msg, ##__VA_ARGS__)

#define NT_LOG_DYN_STR_HOSTED_APP_CRIT(msg, ...) NT_LOG_DYNAMIC_STRING(HOSTED_APP, CRIT, msg, ##__VA_ARGS__)

#define NT_LOG_DYN_STR_TLS_INFO(msg, ...) NT_LOG_DYNAMIC_STRING(TLS, INFO, msg, ##__VA_ARGS__)

#define NT_LOG_DYN_STR_TLS_WARN(msg, ...) NT_LOG_DYNAMIC_STRING(TLS, WARN, msg, ##__VA_ARGS__)

#define NT_LOG_DYN_STR_TLS_ERR(msg, ...) NT_LOG_DYNAMIC_STRING(TLS, ERR, msg, ##__VA_ARGS__)

#define NT_LOG_DYN_STR_TLS_CRIT(msg, ...) NT_LOG_DYNAMIC_STRING(TLS, CRIT, msg, ##__VA_ARGS__)

#define NT_LOG_DYN_STR_DEV_CFG_INFO(msg, ...) NT_LOG_DYNAMIC_STRING(DEV_CFG, INFO, msg, ##__VA_ARGS__)

#define NT_LOG_DYN_STR_DEV_CFG_WARN(msg, ...) NT_LOG_DYNAMIC_STRING(DEV_CFG, WARN, msg, ##__VA_ARGS__)

#define NT_LOG_DYN_STR_DEV_CFG_ERR(msg, ...) NT_LOG_DYNAMIC_STRING(DEV_CFG, ERR, msg, ##__VA_ARGS__)

#define NT_LOG_DYN_STR_DEV_CFG_CRIT(msg, ...) NT_LOG_DYNAMIC_STRING(DEV_CFG, CRIT, msg, ##__VA_ARGS__)

#define NT_LOG_DYN_STR_SOCPM_INFO(msg, ...) NT_LOG_DYNAMIC_STRING(SOCPM, INFO, msg, ##__VA_ARGS__)

#define NT_LOG_DYN_STR_SOCPM_WARN(msg, ...) NT_LOG_DYNAMIC_STRING(SOCPM, WARN, msg, ##__VA_ARGS__)

#define NT_LOG_DYN_STR_SOCPM_ERR(msg, ...) NT_LOG_DYNAMIC_STRING(SOCPM, ERR, msg, ##__VA_ARGS__)

#define NT_LOG_DYN_STR_SOCPM_CRIT(msg, ...) NT_LOG_DYNAMIC_STRING(SOCPM, CRIT, msg, ##__VA_ARGS__)

#define NT_LOG_DYN_STR_WPM_INFO(msg, ...) NT_LOG_DYNAMIC_STRING(WPM, INFO, msg, ##__VA_ARGS__)

#define NT_LOG_DYN_STR_WPM_WARN(msg, ...) NT_LOG_DYNAMIC_STRING(WPM, WARN, msg, ##__VA_ARGS__)

#define NT_LOG_DYN_STR_WPM_ERR(msg, ...) NT_LOG_DYNAMIC_STRING(WPM, ERR, msg, ##__VA_ARGS__)

#define NT_LOG_DYN_STR_WPM_CRIT(msg, ...) NT_LOG_DYNAMIC_STRING(WPM, CRIT, msg, ##__VA_ARGS__)

#else
#define NT_LOG_DYN_STR_HAL_INFO(msg, ptr) NT_LOG_MOD_LVL(HAL, INFO, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_HAL_WARN(msg, ptr) NT_LOG_MOD_LVL(HAL, WARN, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_HAL_ERR(msg, ptr) NT_LOG_MOD_LVL(HAL, ERR, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_HAL_CRIT(msg, ptr) NT_LOG_MOD_LVL(HAL, CRIT, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_SME_INFO(msg, ptr) NT_LOG_MOD_LVL(SME, INFO, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_SME_WARN(msg, ptr) NT_LOG_MOD_LVL(SME, WARN, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_SME_ERR(msg, ptr) NT_LOG_MOD_LVL(SME, ERR, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_SME_CRIT(msg, ptr) NT_LOG_MOD_LVL(SME, CRIT, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_MLM_INFO(msg, ptr) NT_LOG_MOD_LVL(MLM, INFO, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_MLM_WARN(msg, ptr) NT_LOG_MOD_LVL(MLM, WARN, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_MLM_ERR(msg, ptr) NT_LOG_MOD_LVL(MLM, ERR, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_MLM_CRIT(msg, ptr) NT_LOG_MOD_LVL(MLM, CRIT, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_DPM_INFO(msg, ptr) NT_LOG_MOD_LVL(DPM, INFO, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_DPM_WARN(msg, ptr) NT_LOG_MOD_LVL(DPM, WARN, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_DPM_ERR(msg, ptr) NT_LOG_MOD_LVL(DPM, ERR, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_DPM_CRIT(msg, ptr) NT_LOG_MOD_LVL(DPM, CRIT, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_WFM_INFO(msg, ptr) NT_LOG_MOD_LVL(WFM, INFO, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_WFM_WARN(msg, ptr) NT_LOG_MOD_LVL(WFM, WARN, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_WFM_ERR(msg, ptr) NT_LOG_MOD_LVL(WFM, ERR, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_WFM_CRIT(msg, ptr) NT_LOG_MOD_LVL(WFM, CRIT, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_WMI_INFO(msg, ptr) NT_LOG_MOD_LVL(WMI, INFO, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_WMI_WARN(msg, ptr) NT_LOG_MOD_LVL(WMI, WARN, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_WMI_ERR(msg, ptr) NT_LOG_MOD_LVL(WMI, ERR, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_WMI_CRIT(msg, ptr) NT_LOG_MOD_LVL(WMI, CRIT, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_HALPHY_INFO(msg, ptr) NT_LOG_MOD_LVL(HALPHY, INFO, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_HALPHY_WARN(msg, ptr) NT_LOG_MOD_LVL(HALPHY, WARN, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_HALPHY_ERR(msg, ptr) NT_LOG_MOD_LVL(HALPHY, ERR, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_HALPHY_CRIT(msg, ptr) NT_LOG_MOD_LVL(HALPHY, CRIT, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_ANI_INFO(msg, ptr) NT_LOG_MOD_LVL(ANI, INFO, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_ANI_WARN(msg, ptr) NT_LOG_MOD_LVL(ANI, WARN, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_ANI_ERR(msg, ptr) NT_LOG_MOD_LVL(ANI, ERR, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_ANI_CRIT(msg, ptr) NT_LOG_MOD_LVL(ANI, CRIT, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_CLI_INFO(msg, ptr) NT_LOG_MOD_LVL(CLI, INFO, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_CLI_WARN(msg, ptr) NT_LOG_MOD_LVL(CLI, WARN, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_CLI_ERR(msg, ptr) NT_LOG_MOD_LVL(CLI, ERR, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_CLI_CRIT(msg, ptr) NT_LOG_MOD_LVL(CLI, CRIT, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_SYSTEM_INFO(msg, ptr) NT_LOG_MOD_LVL(SYSTEM, INFO, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_SYSTEM_WARN(msg, ptr) NT_LOG_MOD_LVL(SYSTEM, WARN, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_SYSTEM_ERR(msg, ptr) NT_LOG_MOD_LVL(SYSTEM, ERR, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_SYSTEM_CRIT(msg, ptr) NT_LOG_MOD_LVL(SYSTEM, CRIT, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_WIFI_APP_INFO(msg, ptr) NT_LOG_MOD_LVL(WIFI_APP, INFO, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_WIFI_APP_WARN(msg, ptr) NT_LOG_MOD_LVL(WIFI_APP, WARN, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_WIFI_APP_ERR(msg, ptr) NT_LOG_MOD_LVL(WIFI_APP, ERR, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_WIFI_APP_CRIT(msg, ptr) NT_LOG_MOD_LVL(WIFI_APP, CRIT, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_COMMON_INFO(msg, ptr) NT_LOG_MOD_LVL(COMMON, INFO, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_COMMON_WARN(msg, ptr) NT_LOG_MOD_LVL(COMMON, WARN, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_COMMON_ERR(msg, ptr) NT_LOG_MOD_LVL(COMMON, ERR, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_COMMON_CRIT(msg, ptr) NT_LOG_MOD_LVL(COMMON, CRIT, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_SECURITY_INFO(msg, ptr) NT_LOG_MOD_LVL(SECURITY, INFO, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_SECURITY_WARN(msg, ptr) NT_LOG_MOD_LVL(SECURITY, WARN, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_SECURITY_ERR(msg, ptr) NT_LOG_MOD_LVL(SECURITY, ERR, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_SECURITY_CRIT(msg, ptr) NT_LOG_MOD_LVL(SECURITY, CRIT, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_HAL_API_INFO(msg, ptr) NT_LOG_MOD_LVL(HAL_API, INFO, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_HAL_API_WARN(msg, ptr) NT_LOG_MOD_LVL(HAL_API, WARN, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_HAL_API_ERR(msg, ptr) NT_LOG_MOD_LVL(HAL_API, ERR, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_HAL_API_CRIT(msg, ptr) NT_LOG_MOD_LVL(HAL_API, CRIT, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_COMMISIONING_APP_INFO(msg, ptr) NT_LOG_MOD_LVL(COMMISIONING_APP, INFO, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_COMMISIONING_APP_WARN(msg, ptr) NT_LOG_MOD_LVL(COMMISIONING_APP, WARN, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_COMMISIONING_APP_ERR(msg, ptr) NT_LOG_MOD_LVL(COMMISIONING_APP, ERR, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_COMMISIONING_APP_CRIT(msg, ptr) NT_LOG_MOD_LVL(COMMISIONING_APP, CRIT, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_ONBOARDING_APP_INFO(msg, ptr) NT_LOG_MOD_LVL(ONBOARDING_APP, INFO, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_ONBOARDING_APP_WARN(msg, ptr) NT_LOG_MOD_LVL(ONBOARDING_APP, WARN, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_ONBOARDING_APP_ERR(msg, ptr) NT_LOG_MOD_LVL(ONBOARDING_APP, ERR, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_ONBOARDING_APP_CRIT(msg, ptr) NT_LOG_MOD_LVL(ONBOARDING_APP, CRIT, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_WPS_INFO(msg, ptr) NT_LOG_MOD_LVL(WPS, INFO, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_WPS_WARN(msg, ptr) NT_LOG_MOD_LVL(WPS, WARN, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_WPS_ERR(msg, ptr) NT_LOG_MOD_LVL(WPS, ERR, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_WPS_CRIT(msg, ptr) NT_LOG_MOD_LVL(WPS, CRIT, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_AWS_INFO(msg, ptr) NT_LOG_MOD_LVL(AWS, INFO, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_AWS_WARN(msg, ptr) NT_LOG_MOD_LVL(AWS, WARN, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_AWS_ERR(msg, ptr) NT_LOG_MOD_LVL(AWS, ERR, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_AWS_CRIT(msg, ptr) NT_LOG_MOD_LVL(AWS, CRIT, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_COEX_INFO(msg, ptr) NT_LOG_MOD_LVL(COEX, INFO, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_COEX_WARN(msg, ptr) NT_LOG_MOD_LVL(COEX, WARN, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_COEX_ERR(msg, ptr) NT_LOG_MOD_LVL(COEX, ERR, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_COEX_CRIT(msg, ptr) NT_LOG_MOD_LVL(COEX, CRIT, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_RCLI_INFO(msg, ptr) NT_LOG_MOD_LVL(RCLI, INFO, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_RCLI_WARN(msg, ptr) NT_LOG_MOD_LVL(RCLI, WARN, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_RCLI_ERR(msg, ptr) NT_LOG_MOD_LVL(RCLI, ERR, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_RCLI_CRIT(msg, ptr) NT_LOG_MOD_LVL(RCLI, CRIT, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_HOSTED_APP_INFO(msg, ptr) NT_LOG_MOD_LVL(HOSTED_APP, INFO, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_HOSTED_APP_WARN(msg, ptr) NT_LOG_MOD_LVL(HOSTED_APP, WARN, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_HOSTED_APP_ERR(msg, ptr) NT_LOG_MOD_LVL(HOSTED_APP, ERR, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_HOSTED_APP_CRIT(msg, ptr) NT_LOG_MOD_LVL(HOSTED_APP, CRIT, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_TLS_INFO(msg, ptr) NT_LOG_MOD_LVL(TLS, INFO, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_TLS_WARN(msg, ptr) NT_LOG_MOD_LVL(TLS, WARN, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_TLS_ERR(msg, ptr) NT_LOG_MOD_LVL(TLS, ERR, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_TLS_CRIT(msg, ptr) NT_LOG_MOD_LVL(TLS, CRIT, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_DEV_CFG_INFO(msg, ptr) NT_LOG_MOD_LVL(DEV_CFG, INFO, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_DEV_CFG_WARN(msg, ptr) NT_LOG_MOD_LVL(DEV_CFG, WARN, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_DEV_CFG_ERR(msg, ptr) NT_LOG_MOD_LVL(DEV_CFG, ERR, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_DEV_CFG_CRIT(msg, ptr) NT_LOG_MOD_LVL(DEV_CFG, CRIT, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_SOCPM_INFO(msg, ptr) NT_LOG_MOD_LVL(SOCPM, INFO, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_SOCPM_WARN(msg, ptr) NT_LOG_MOD_LVL(SOCPM, WARN, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_SOCPM_ERR(msg, ptr) NT_LOG_MOD_LVL(SOCPM, ERR, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_SOCPM_CRIT(msg, ptr) NT_LOG_MOD_LVL(SOCPM, CRIT, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_WPM_INFO(msg, ptr) NT_LOG_MOD_LVL(WPM, INFO, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_WPM_WARN(msg, ptr) NT_LOG_MOD_LVL(WPM, WARN, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_WPM_ERR(msg, ptr) NT_LOG_MOD_LVL(WPM, ERR, ptr, 0, 0, 0)

#define NT_LOG_DYN_STR_WPM_CRIT(msg, ptr) NT_LOG_MOD_LVL(WPM, CRIT, ptr, 0, 0, 0)
#endif  // SUPPORT_FERMION_LOGGER

/**
 * @FUNCTION    : nt_log_cfg
 * @description : Initialize All the Buffers and Calling all the cfg functions.
 *  Initialize the block size, from the user or default value.
 *  create semaphore.
 * @Param      : Pointer variable size.changing of buffers into blocks.
 * @return     : NONE
 */
void nt_log_cfg(log_ringdesc_t *size);

/**
 *   @FUNCTION    :    nt_log_deinit
 *   @Description :    de-initialize the initialized memory
 *   @Param       :    NONE
 *   @return      :    Success
 */
uint8_t nt_log_deinit(void);
/**
 *   @FUNCTION    : nt_log_read
 *   @Description : Read Data from all the buffers
 *   @Param       : module_id : cli-command, set the module id, based on filtering the data.
 *   @return      : NONE
 */
void nt_log_read(uint8_t moduleId);
/**
 *  @FUNCTION    : nt_log_lvl_set
 *  @Description : It indicates the, which log level is set. the log level and module id set through cli command.
 *  @Param       : log level(0 - 3)
 *   			   Moduleid - moduleid set through cli
 *  @return      : success/FAILURE
 */
uint8_t nt_log_lvl_set(uint8_t moduleid, int8_t loglvl);
/**
 *  @FUNCTION    : nt_log_lvl_get
 *  @Description : get the which log level is enabled.
 *  @Param       : NONE
 *  @return      : Success/FAILURE
 */
uint8_t nt_log_lvl_get(void);
/**
 *  @FUNCTION    : nt_log_lvl_clr
 *  @Description : set the all log level is 0.
 *  @Param       : NONE
 *  @return      : Success/FAILURE
 */
uint8_t nt_log_lvl_clear(void);

/**
 * @FUNCTION    :  nt_log_data_pass_to_buff
 * @Description :  Initialize all the structure members.And clear the respective block, Write data to buffer.
                   Based on module id and log level, We are fill the buffer. Each buffer we are splitting
                   to blocks. Each block consist of time stamp and module id and log level and task name function
                   name and function line number and message and p1,p2,p3 parameters.
 * @Param       :  log level(0 - 3): like info,warn,err,critical.
 *                 moduleid        : 8 - bit module id
 *                 message         : String parameter pointer
                   p1,p2,p3        : User calling parameters..
                   fn              : for function name
                   ln              : for line number.
                   log_buffer      : Buffer,like info,warn,err,critical.
                   calc_msg_len    : It indicates message length.
                   xStartTime      : Timer stamp
 * @return     :   success/fail.
 */
uint8_t nt_log_data_pass_to_buff(log_ringdesc_t *log_buffer, uint8_t mod_id, uint8_t loglvl,
#if (NT_FN_FUNCTION_LINE_NUM_FLAG == 1)
                                 char *fn, uint16_t ln,
#endif
                                 const char *msg, uint32_t p1, uint32_t p2, uint32_t p3, uint8_t calc_msg_len,
                                 uint32_t xStartTime);

/**
 *   @FUNCTION    : nt_calc_for_num_blocks
 *   @Description : check the buffer is fill or not. buffer is fill, restart head position.
 *   @Param       : log_buffer : buffer
 *   @return      : Index
 */

uint32_t nt_calc_for_num_blocks(log_ringdesc_t *log_buffer);

/**
 *   @FUNCTION    : nt_log_lvl_read
 *   @Description : Through CLI command, we will get the respective log related log information from respective
 * moduleid.
 *   @Param       : moduleid.
 *   				log level.
 *   @return      : Null
 */
void nt_log_lvl_read(uint8_t moduleid, uint8_t loglevel);

WIFIReturnCode_t _nt_log_lvl_clr(void);
WIFIReturnCode_t _nt_log_lvl_get(void);
// WIFIReturnCode_t _nt_log_lvl_set (void*);
// WIFIReturnCode_t _nt_log_lvl_read (void *);
// WIFIReturnCode_t _nt_log_read (void *);
WIFIReturnCode_t _nt_log_buffer_clr(void);

#endif  // NT_FN_LOGGER

#endif  //_NT_LOGGER_API_H
