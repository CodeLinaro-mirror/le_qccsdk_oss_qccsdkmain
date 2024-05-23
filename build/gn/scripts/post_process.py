# -*- coding:utf-8 -*-
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#

import os
import subprocess
import sys
import shutil


def dbg_print(s):
    print(s)
    sys.stdout.flush()

def main():
    if len(sys.argv) != 2:
        dbg_print("Usage: python post_processs.py <elf image file>")
        sys.exit(-1)
    elf_file = sys.argv[1]
    if not os.path.exists(elf_file):
        dbg_print("ELF image file not existed: %s" % elf_file)
    elf_dir = os.path.dirname(elf_file)
    elf_base = os.path.basename(elf_file)
    elf_base = elf_base[0:-4]

    # arm-none-eabi-objcopy -O ihex FERMION_PBL/FERMION_PBL.elf FERMION_PBL/FERMION_PBL.hex
    cmd = [ 'arm-none-eabi-objcopy', '-O', 'ihex', elf_file, os.path.join(elf_dir, elf_base+'.hex'), ]
    dbg_print('Creating Hex Image ....')
    dbg_print(cmd)
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (stdout,stderr) = process.communicate()
    if (process.returncode != 0):
        dbg_print('Creating Hex Image failed')
        sys.exit(-1)

    # arm-none-eabi-size --format=berkeley --totals FERMION_PBL/FERMION_PBL.elf
    cmd = [ 'arm-none-eabi-size', '--format=berkeley', '--totals', elf_file, ]
    dbg_print('Creating Print Size ....')
    dbg_print(cmd)
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (stdout,stderr) = process.communicate()
    dbg_print(stdout.decode('utf-8').replace('\r', ''))
    if (process.returncode != 0):
        dbg_print('Creating Print Size failed')
        sys.exit(-1)

    # arm-none-eabi-objcopy FERMION_PBL/FERMION_PBL.elf --dump-section .image_version=FERMION_PBL/version.bin
    #cmd = [ 'arm-none-eabi-objcopy', elf_file, '--dump-section', '.image_version='+os.path.join(elf_dir, 'version.bin'), ]
    #dbg_print('Creating Bin Image ....')
    #dbg_print(cmd)
    #process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    #(stdout,stderr) = process.communicate()
    #if (process.returncode != 0):
    #    dbg_print('Creating Bin Image failed')
    #    sys.exit(-1)
    # arm-none-eabi-objcopy FERMION_PBL/FERMION_PBL.elf --strip-all FERMION_PBL/FERMION_PBL_STRIPPED.elf
    cmd = [ 'arm-none-eabi-objcopy', elf_file, '--strip-all', os.path.join(elf_dir, elf_base+'_STRIPPED.elf'), ]
    dbg_print('Creating stripped Image ....')
    dbg_print(cmd)
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (stdout,stderr) = process.communicate()
    if (process.returncode != 0):
        dbg_print('Creating stripped Image failed')
        sys.exit(-1)
    # arm-none-eabi-objcopy FERMION_PBL/FERMION_PBL.elf -O binary FERMION_PBL/FERMION_PBL.bin
    cmd = [ 'arm-none-eabi-objcopy', elf_file, '-O', 'binary', os.path.join(elf_dir, elf_base+'.bin'), ]
    dbg_print('Creating Bin Image ....')
    dbg_print(cmd)
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (stdout,stderr) = process.communicate()
    if (process.returncode != 0):
        dbg_print('Creating Bin Image failed')
        sys.exit(-1)

    #copy FERMION_NVM_PROGRAMMER.elf to tools/bin
    if elf_base == 'FERMION_NVM_PROGRAMMER':
        current_path = os.path.dirname(os.path.abspath(__file__))
        tools_bin_path = os.path.join(current_path, '../../../tools/nvm_programmer/bin')
        if not os.path.exists(tools_bin_path):
            os.makedirs(tools_bin_path)
        absolute_dest_file = os.path.join(tools_bin_path, elf_base + '.elf')
        shutil.copy2(elf_file, absolute_dest_file)


    # Generate Hashed image	
    curr_path = os.path.dirname(os.path.abspath(__file__))
    hash_path = os.path.join(curr_path, '../../../tools/sechash') 
    cmd = [ 'python', os.path.join(hash_path, 'createxbl.py'), '-f', elf_file, '-a32', '-o', os.path.join(elf_dir, elf_base+'_HASHED.elf'), ]
    dbg_print('Creating Hashed Image ....')
    dbg_print(cmd)
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (stdout,stderr) = process.communicate()
    dbg_print(stdout.decode('utf-8').replace('\r', ''))
    if (process.returncode != 0):
        dbg_print(stderr.decode('utf-8').replace('\r', ''))
        dbg_print('Creating hashed Image failed')
        sys.exit(-1)
if __name__== "__main__":
    main()
    sys.exit(0)

