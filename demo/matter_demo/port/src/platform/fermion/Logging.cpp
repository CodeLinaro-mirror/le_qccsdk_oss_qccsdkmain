/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <platform/logging/LogV.h>

#include <lib/core/CHIPConfig.h>
#include <lib/support/EnforceFormat.h>
#include <lib/support/logging/Constants.h>
#include <platform/CHIPDeviceConfig.h>

#include <stdio.h>

typedef void *QAPI_Console_Group_Handle_t;
extern "C" {
void QCLI_Printf(QAPI_Console_Group_Handle_t Group_Handle, const char *format, ...);
}
extern QAPI_Console_Group_Handle_t qcli_matter_group;

namespace chip {
    namespace Logging {
        namespace Platform {
            void ENFORCE_FORMAT(3, 0) LogV(const char *module, uint8_t category, const char *msg, va_list v)
            {
                char formattedMsg[CHIP_CONFIG_LOG_MESSAGE_MAX_SIZE];
                vsnprintf(formattedMsg, sizeof(formattedMsg), msg, v);

                switch (category) {
                    case kLogCategory_Error:
                    case kLogCategory_Progress:
                    case kLogCategory_Detail:
                    default:
                        QCLI_Printf(qcli_matter_group, "%s", formattedMsg);
                        break;
                }
                QCLI_Printf(qcli_matter_group, "\n");
            }

        }  // namespace Platform
    }      // namespace Logging
}  // namespace chip
