# -*- coding:utf-8 -*-
#

import os
import shutil
import sys

(input, output) = sys.argv[1:]

if os.path.exists(output):
    if os.path.isdir(output):
        shutil.rmtree(output)
    else:
        os.remove(output)

with open(output, 'wb') as outp:
    with open(input, 'r') as inp:
        for item in inp.readlines():
            if item.startswith("#"):
                continue
            item = item.strip()
            if item == "":
                continue

            pos = item.find('=y')
            if (pos != -1):
                item_orig = item
                item = item_orig[0:pos] + '=true' + item_orig[pos+2:]

            pos = item.find('=n')
            if (pos != -1):
                item_orig = item
                item = item_orig[0:pos] + '=false' + item_orig[pos+2:]

            pos = item.find('=0x')
            if (pos != -1):
                item_orig = item
                hex_digit = item_orig[pos+1:len(item_orig)]
                dec_int = int(hex_digit, 16)
                item = item_orig[0:pos] + '=%d'%dec_int
            item = item + '\n'
            outp.write(item.encode('utf-8'))
