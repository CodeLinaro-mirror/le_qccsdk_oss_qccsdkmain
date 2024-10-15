#===============================================================================
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#===============================================================================
import os
import sys
import re
import argparse

cur_dir = os.getcwd()
qccsdkpy_dir = cur_dir
project_root = os.path.dirname(os.path.dirname(qccsdkpy_dir))
qccsdkpy_lib_dir = os.path.join(qccsdkpy_dir, 'libs')

sys.path.append(qccsdkpy_dir)
sys.path.append(qccsdkpy_lib_dir)

from utils import Utils

parser = argparse.ArgumentParser(description='get', formatter_class=argparse.RawDescriptionHelpFormatter)
parser.add_argument('--appdir', '-S', required=False, action='store_true', help='get application path')
parser.add_argument('--output', '-o', required=False, action='store_true', help='get output path')
parser.add_argument('--board', '-b', required=False, action='store_true', help='get board name')
parser.add_argument('--jtag', required=False, action='store_true', help='get jtag interface name')
parser.add_argument('--debug', '--verbose', '-v', required=False, action='store_true', help='enalbe output debug info on console')
args = parser.parse_args()

cUtils = Utils(project_root, qccsdkpy_dir, enableLogDebug=args.debug)
Logger = cUtils.logger
#set param should be included in qccsdk
qccsdk = cUtils.CfgExternal['qccsdk']

cUtils.ENTER()

#by default, no parameter
if len(sys.argv)==1:
    cUtils.SUCCESS('no parameter, skip')

def get_param (name):
    if name in qccsdk.keys():
        val = qccsdk[name]
        Logger.info('qccsdk[%s]=%s'%(name, val))
        return [True, val]
    else:
        Logger.warning('%s is not supported in qccsdk[]'%(name))
        return [False, None]

if args.appdir==True:
    get_param('appdir')

if args.output==True:
    get_param('output')

if args.board==True:
    get_param('board')

if args.jtag==True:
    get_param('jtag')

cUtils.SUCCESS()
