/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#include "nt_prng.h"
uint8_t qapi_prng_get(uint8_t *ptr, uint16_t len);

#define qapi_prng_get nt_wlan_hw_prng_get
