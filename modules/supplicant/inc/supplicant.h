/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _SUPPLICANT_H_
#define _SUPPLICANT_H_

int wlan_supplicant_init(void);
void wlan_supplicant_thread_stop(void);
void wlan_supplicant_exit(void);
int wlan_supplicant_add_interface(unsigned char devid);
void wlan_supplicant_remove_interface(unsigned char devid);

#endif /* _SUPPLICANT_H_ */
