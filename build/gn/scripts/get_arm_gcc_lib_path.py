# -*- coding:utf-8 -*-
#

import os
import subprocess
import sys

lib_name = sys.argv[1]

if lib_name is None:
    sys.exit(-1)

para = None
if (lib_name == "c_nano"):
    para = "-print-file-name=libc_nano.a"
if (lib_name == "gcc"):
    para = "-print-libgcc-file-name"

if para is None:
    sys.exit(-1)

cmd = [ 'arm-none-eabi-gcc', '-mcpu=cortex-m4', '-mthumb',
    '-mfloat-abi=softfp', para ]
process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
(printout, stderr) = process.communicate()
if (process.returncode != 0):
    print('find lib_name failed')
    sys.exit(-1)
printout = printout.decode('utf-8')
#printout = printout.replace('\n', '')
#printout = printout.replace('\r', '')
toollibpath = os.path.dirname(printout)
print(toollibpath)
