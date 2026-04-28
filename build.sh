#!/bin/bash

#========================================================================
#Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
#SPDX-License-Identifier: BSD-3-Clause-Clear
#
# Fermion integration entry
#========================================================================
pushd ..
if [ ! -f SRC-IOE-SDK.tar.gz ]; then
    tar --exclude=.git --exclude=.gitignore -czpf SRC-IOE-SDK.tar.gz qccsdk comp/backoffAlgorithm comp/exhost comp/freertos \
    comp/littlefs comp/lwip comp/matter comp/mbedtls comp/mqtt comp/posix comp/qurt comp/segger-rtt comp/qtcapy comp/hostap
fi
popd
SCRIPT_PATH=$(dirname "$(readlink -f "$0")")
export PATH=/pkg/qct/software/arm/linaro-toolchain/gcc-arm-none-eabi-8-2019-q3-update/bin:$PATH
python intg.py --fsdk
if [ ! -d ${SCRIPT_PATH}/../prebuilt_HY11 ]; then
    mkdir -p ${SCRIPT_PATH}/../prebuilt_HY11
    mkdir -p ${SCRIPT_PATH}/../prebuilt_HY11/p2p/lib
    mkdir -p ${SCRIPT_PATH}/../prebuilt_HY11_ART
    cp ${SCRIPT_PATH}/../qccsdk/output/wifi_lib/FERMION_WIFI_LIB/DEBUG/lib/libwifi_core.a  ${SCRIPT_PATH}/../prebuilt_HY11/
    cp ${SCRIPT_PATH}/../qccsdk/output/wifi_lib/FERMION_WIFI_LIB/DEBUG/p2p/lib/libwifi_core.a  ${SCRIPT_PATH}/../prebuilt_HY11/p2p/lib/
    cp ${SCRIPT_PATH}/../comp/wifi/NOTICE.txt ${SCRIPT_PATH}/../prebuilt_HY11/
    cp ${SCRIPT_PATH}/../comp/wifi/LICENSE.txt ${SCRIPT_PATH}/../prebuilt_HY11/
    cp -r ${SCRIPT_PATH}/../prebuilt_HY11/* ${SCRIPT_PATH}/../prebuilt_HY11_ART/
else
    mkdir -p ${SCRIPT_PATH}/../prebuilt_HY11_ART
    cp -r ${SCRIPT_PATH}/../prebuilt_HY11/* ${SCRIPT_PATH}/../prebuilt_HY11_ART/
fi

