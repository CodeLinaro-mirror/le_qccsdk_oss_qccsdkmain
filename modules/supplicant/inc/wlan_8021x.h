/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _WLAN_8021X_H_
#define _WLAN_8021X_H_

int wlan_8021x_init(void);
void wlan_8021x_exit(void);
int wlan_8021x_event_cb(unsigned char device_ID, unsigned int event_ID,
                        void *cb_cxt, void *payload,
                        unsigned int payload_Length);

#endif /* _WLAN_8021X_H_ */
