# -*- coding:utf-8 -*-
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#

import re
coredump_start_pattern = r'============== ramdump start =============='
coredump_end_pattern = r'============== ramdump end =============='

def hex_to_binary(hex_file_path, bin_file_path):
    with open(hex_file_path, 'r') as hex_file:
        hex_data = hex_file.read()
        coredump_start = hex_data.find(coredump_start_pattern, 0) + len(coredump_start_pattern)
        coredump_end = hex_data.find(coredump_end_pattern, 0) - 1
        binary_data = bytes.fromhex(hex_data[coredump_start:coredump_end])
 
    with open(bin_file_path, 'wb') as bin_file:
        bin_file.write(binary_data)
 
if __name__ == "__main__":
    hex_file_path = 'ramdump.txt'
    bin_file_path = 'ram.bin'
    hex_to_binary(hex_file_path, bin_file_path)
