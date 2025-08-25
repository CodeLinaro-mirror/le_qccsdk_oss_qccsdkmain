/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "nt_osal.h"
#include "mbedtls/mbedtls_config.h"
#include "nt_prng.h"
#include "string.h"

#if defined(MBEDTLS_ENTROPY_HARDWARE_ALT)
int mbedtls_hardware_poll(void __attribute__((__unused__)) * data, unsigned char *output, size_t len, size_t *olen)
{
    if (nt_wlan_hw_prng_get(output, len) == 1) {
        *olen = len;
        return 0;
    } else {
        return -1;
    }
}
#endif  // MBEDTLS_ENTROPY_HARDWARE_ALT
