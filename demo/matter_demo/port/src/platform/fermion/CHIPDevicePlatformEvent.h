/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 *    @file
 *          Defines platform-specific event types and data for the
 *          CHIP Device Layer on the Fermion.
 */

#pragma once

#include <platform/CHIPDeviceEvent.h>
#include <system/SystemPacketBuffer.h>
extern "C" {
#include <qapi/wlan/qapi_wlan_misc.h>
#include <qapi/wlan/qapi_wlan_base.h>
}

namespace chip {
    namespace DeviceLayer {

        namespace DeviceEventType {

            /**
             * Enumerates Fermion platform-specific event types that are visible to the application.
             */
            enum PublicPlatformSpecificEventTypes { kFermionSystemEvent = kRange_PublicPlatformSpecific };

            enum PlatformSpecificEventId {
                QAPI_WLAN_UNKNOWN_ID,
                QAPI_WLAN_DISCONNECT,
                QAPI_WLAN_CONNECT,
                QAPI_WLAN_SCAN_COMPLETE
            };

        }  // namespace DeviceEventType

        /**
         * Represents platform-specific event information for Fermion platforms.
         */

        struct ChipDevicePlatformEvent {
            union {
                struct {
                    int32_t Id;
                    union {
                        qapi_WLAN_Connect_Cb_Info_t WiFiStaConnected;
                        qapi_WLAN_Scan_Comp_Evt_t WiFiStaScanDone;
                    } Data;
                } FermionSystemEvent;
            };
        };

    }  // namespace DeviceLayer
}  // namespace chip
