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

parser = argparse.ArgumentParser(description='set', formatter_class=argparse.RawDescriptionHelpFormatter)
parser.add_argument('--appdir', '-S', required=False, action="store", type=str, help='set application path')
parser.add_argument('--output', '-o', required=False, action="store", type=str, help='set output path')
parser.add_argument('--board', '-b', required=False, action="store", type=str, help='set board name')
parser.add_argument('--jtag', required=False, action="store", type=str, help='set jtag interface name')
parser.add_argument('--menuconfig', required=False, action='store_true', help='set Kconfig manually')
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

def set_param_pre (name, value):
    if value == qccsdk[name]:
        Logger.warning('CfgExternal[\'qccsdk\'][%s]==%s, skip'%(name, value))
        return False
    else:
        cUtils.qassert((cUtils.is_cfg_external_writable()==True), 'cfg_external not writable, operation failed')
        return True

def set_param (name, value, pattern):
    global qccsdk
    cUtils.cfg_external_write(pattern, name, value)
    qccsdk = cUtils.CfgExternal['qccsdk']
    cUtils.qassert((qccsdk[name]==value), 'Should not happen')

if args.output!=None:
    output = args.output
    if set_param_pre('output', output):
        set_param('output', output, 'output: &output ')
        cUtils = Utils(project_root, qccsdkpy_dir, enableLogDebug=args.debug)
        Logger = cUtils.logger

if args.appdir!=None:
    appdir = args.appdir
    if set_param_pre('appdir', appdir):
        cUtils.assert_appdir_supported(appdir)
        set_param('appdir', appdir, 'appdir: &appdir ')

if args.board!=None:
    board = args.board
    if set_param_pre('board', board):
        cUtils.assert_board_supported(board)
        set_param('board', board, 'board: &board ')

if args.jtag!=None:
    jtag = args.jtag
    if set_param_pre('jtag', jtag):
        cUtils.assert_jtag_supported(jtag)
        set_param('jtag', jtag, 'jtag: &jtag ')

cUtils.SUCCESS()
