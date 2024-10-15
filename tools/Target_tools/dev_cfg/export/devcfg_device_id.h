/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/
/*
 * devcfg_device_id.h
 *
 *  Created on: 28-Jul-2020
 *      Author: HIMADRI
 */

#ifndef CORE_DEV_CFG_EXPORT_DEVCFG_DEVICE_ID_H_
#define CORE_DEV_CFG_EXPORT_DEVCFG_DEVICE_ID_H_

/**
 *  @brief Device Id for all the modules are
 *  defined here.
 *  when new modules are created need to keep
 *  the record here manually.
 */
// 0x03000001 is free can be used for other xml
#define DEVICE_ID_LOG 						0x03000002				// for log xml
#define DEVICE_ID_ROAMING					0x03000003				// for roaming xml
#define DEVICE_ID_BC_PW						0x03000004				// for beacon_power xml
#define DEVICE_ID_TWT						0x03000005				// for powersave TWT xml
#define DEVICE_ID_WIFI_APP				    0x03000006				// for wifi_app xml
#define DEVICE_ID_RATE_ADAPTATION			0x03000007				// for rate_adaptation xml
#define DEVICE_ID_WIFI_SECURITY				0x03000008				// for wifi_security xml
#define DEVICE_ID_CONNECTION				0x03000009				// for connection xml
#define DEVICE_ID_PERFORMANCE				0x03000010				// for performance xml
#define DEVICE_ID_MAC						0x03000011				// for mac xml
#define DEVICE_ID_RTT_FTM					0x03000012				// for rtt_ftm xml
#define DEVICE_ID_SYSTEM					0x03000013				// for system xml
#define DEVICE_ID_NEUT2						0x03000014				// for neutrino2 xml

/** @brief Device Id section for byte sequence support
 * for now one common byte sequence xml is used
 * for new byte sequence xml device id should start by 0x04......
 */
#define DEVICE_ID_BYTE_SEQUENCE			    0x04000001				// common device id for byte sequence xml

#endif /* CORE_DEV_CFG_EXPORT_DEVCFG_DEVICE_ID_H_ */
