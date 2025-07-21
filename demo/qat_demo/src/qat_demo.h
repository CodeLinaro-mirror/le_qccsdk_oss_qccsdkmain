#ifndef QAT_DEMO_H
#define QAT_DEMO_H
/*
#Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
#SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/**
 * Initialize this mgmt filter demo:
 * - start the mgmt_frame_recv_thread
 */
void Initialize_QAT_Common_Demo(void);

void Initialize_QAT_Mqtt_Demo(void);

void Initialize_QAT_TCPIP_Demo(void);

void Initialize_QAT_Wlan_Demo(void);

void Initialize_QAT_Http_Server_Demo(void);

void Initialize_QAT_HttpC_Demo(void);

void Initialize_QAT_OTA_Demo(void);

#endif /* QAT_DEMO_H */
