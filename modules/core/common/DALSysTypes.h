/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * DALSysTypes.h
 *
 *  Created on: 02-Jun-2020
 *      Author: HIMADRI
 */

#ifndef CORE_COMMON_DALSYSTYPES_H_
#define CORE_COMMON_DALSYSTYPES_H_

#include "DALStdDef.h"
#include "DALStdErr.h"

typedef uint32 DALSYSPropertyHandle[2];

typedef struct DALSYSConfig DALSYSConfig;
struct DALSYSConfig {
    void *pCfgShared;
    void *pCfgMode;
    void *reserved;
};

typedef struct DALSYSPropertyVar DALSYSPropertyVar;
struct DALSYSPropertyVar {
    uint32 dwType;
    uint32 dwLen;
    union {
        byte *pbVal;
        char *pszVal;
        uint32 dwVal;
        uint32 *pdwVal;
        const void *pStruct;
    } Val;
};

typedef struct DALProps DALProps;
struct DALProps {
    const byte *pDALPROP_PropBin;
    const void **pDALPROP_StructPtrs;
    uint32 dwDeviceSize;   // Size of Devices array
    const void *pDevices;  // String Device array
};

typedef struct DALPropsDir DALPropsDir;
struct DALPropsDir {
    uint32 dwLen;
    uint32 dwPropsNameSectionOffset;
    uint32 dwPropsStringSectionOffset;
    uint32 dwPropsByteSectionOffset;
    uint32 dwPropsUint32SectionOffset;
    uint32 dwNumDevices;
    uint32 dwPropsDeviceOffset[1][2];  // structure like this will follow...
};

#endif /* CORE_COMMON_DALSYSTYPES_H_ */
