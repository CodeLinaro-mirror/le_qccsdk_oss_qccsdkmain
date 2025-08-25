/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _WPS_MISC_H_
#define _WPS_MISC_H_

#ifdef __cplusplus
extern "C" {
#endif

// int8_t gen_random_number(uint8_t *dest,uint8_t len);
uint32_t wps_pin_checksum(uint32_t pin);
uint8_t wps_pin_valid(uint8_t *pin);

#ifdef WPS_DEBUG
void dump_hex(const char *info, const uint8_t *data, uint16_t len);
void dump_ascii(const char *info, const uint8_t *data, uint16_t len);
void wps_dump_remote_info(WPS_CONTEXT *wps);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _WPS_MISC_H */
