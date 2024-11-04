#!/usr/bin/env python3
#===============================================================================
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#===============================================================================


import os
import sys


if len(sys.argv) >= 2:
    source_root_path = sys.argv[1]
else:
    source_root_path = r"../../../.."

dev_cfg_inc_path = source_root_path + r"\tools\Target_tools\dev_cfg\inc"
os.chdir(dev_cfg_inc_path)
# os.chdir(r"..\..\..\..\tools\Target_tools\dev_cfg\inc")

os.system(
    "python propgen.py --XmlFile=master_xml.xml --DirName=%cd% --ConfigFile=nt_devcfg_from_master_xml.h --DevcfgDataFile=nt_devcfg_data.h --ConfigType=devcfg_xml"
)
os.system(
    "python propgen.py --XmlFile=byte_sequence.xml --DirName=%cd% --ConfigFile=nt_devcfg_from_byte_sequence.h --DevcfgDataFile=nt_devcfg_byte_data.h --ConfigType=devcfg_byte_seq_xml"
)
