/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _PHYDEVLIB_HWIO_H_
#define _PHYDEVLIB_HWIO_H_

//#include "wcss_version.h"

#if WCSS_VERSION <= 1000
#include "fermion_reg.h"
#elif WCSS_VERSION >= 1000 && WCSS_VERSION < 2000
/* Lithium based HWIO files: >= 1000 and < 2000 */
#include "wcss_seq_hwioreg.h"
#include "wcss_seq_hwiobase.h"
#else
/* Beryllium based HWIO files */
#include "wcss_seq_hwioreg_phy.h"
#include "wcss_seq_hwioreg_rfa.h"
#include "wcss_seq_hwioreg_wifi_top.h"
#include "wcss_seq_hwioreg_wcss.h"
#endif

#endif /* _PHYDEVLIB_HWIO_H_ */
