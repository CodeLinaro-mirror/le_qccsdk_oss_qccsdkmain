/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "DALSysTypes.h"
#include "DALStdDef.h"

extern const void *DALPROP_StructPtrs_qca4020_devcfg_xml[];

extern const uint32 DALPROP_PropBin_qca4020_devcfg_xml[];

int parse_dev(uint32 DALPROP_PropBin_qca4020_devcfg_xml);

const DALProps DALPROP_PropsInfo_qca4020_devcfg_xml = {(const byte *)DALPROP_PropBin_qca4020_devcfg_xml,
                                                       DALPROP_StructPtrs_qca4020_devcfg_xml, 0, NULL};
