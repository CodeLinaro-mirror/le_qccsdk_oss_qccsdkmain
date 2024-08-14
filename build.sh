#!/bin/bash

#========================================================================
#Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
#SPDX-License-Identifier: BSD-3-Clause-Clear
#
# Fermion integration entry
#========================================================================
SCRIPT_PATH=$(dirname "$(readlink -f "$0")")
export PATH=/pkg/qct/software/arm/linaro-toolchain/gcc-arm-none-eabi-8-2019-q3-update/bin:$PATH
python intg.py
mkdir -p ${SCRIPT_PATH}/../prebuilt_HY11
mkdir -p ${SCRIPT_PATH}/../prebuilt_HY11_ART
cp ${SCRIPT_PATH}/../qccsdk/output/wifi_lib/FERMION_WIFI_LIB/DEBUG/lib/libwifi_core.a  ${SCRIPT_PATH}/../prebuilt_HY11/
cp ${SCRIPT_PATH}/../comp/wifi/NOTICE ${SCRIPT_PATH}/../prebuilt_HY11/
cp ${SCRIPT_PATH}/../comp/wifi/LICENSE.txt ${SCRIPT_PATH}/../prebuilt_HY11/
cp ${SCRIPT_PATH}/../qccsdk/output/wifi_lib/FERMION_WIFI_LIB/DEBUG/lib/libwifi_core.a ${SCRIPT_PATH}/../prebuilt_HY11_ART/
cp ${SCRIPT_PATH}/../comp/wifi/NOTICE ${SCRIPT_PATH}/../prebuilt_HY11_ART/
cp ${SCRIPT_PATH}/../comp/wifi/LICENSE.txt ${SCRIPT_PATH}/../prebuilt_HY11_ART/

if [ -d "${SCRIPT_PATH}/SRC-IOE-SDK/" ]; then
	echo "SDK packed successfully, start pack lib files.. "
	cp -r ${SCRIPT_PATH}/../prebuilt_HY11 ${SCRIPT_PATH}/../qccsdk/SRC-IOE-SDK/.
	cp -r ${SCRIPT_PATH}/../prebuilt_HY11_ART ${SCRIPT_PATH}/../qccsdk/SRC-IOE-SDK/.
	tar -czf SRC-IOE-SDK.tar.gz SRC-IOE-SDK
	rm -rf ${SCRIPT_PATH}/../qccsdk/SRC-IOE-SDK
	mv ${SCRIPT_PATH}/../qccsdk/SRC-IOE-SDK.tar.gz ${SCRIPT_PATH}/../
fi

