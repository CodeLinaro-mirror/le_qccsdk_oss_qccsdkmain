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

parser = argparse.ArgumentParser(description='build', formatter_class=argparse.RawDescriptionHelpFormatter)
parser.add_argument('--build', required=False, action='store_true', help='build to generate images')
parser.add_argument('--clean', required=False, action='store_true', help='clean build')
parser.add_argument('--rebuild', required=False, action='store_true', help='clean then build')
parser.add_argument('--debug', '--verbose', '-v', required=False, action='store_true', help='enalbe output debug info on console')
args = parser.parse_args()

cUtils = Utils(project_root, qccsdkpy_dir, enableLogDebug=args.debug)
Logger = cUtils.logger
#set param should be included in qccsdk
qccsdk = cUtils.CfgExternal['qccsdk']
appdir = qccsdk['appdir']
board = qccsdk['board']

cUtils.ENTER()

def build_action2arg (action):
    global args
    if action not in cUtils.CfgInternal['build']['default_actions_supported']:
        Logger.warn_not_supported(str(action))
        cUtils.SUCCESS('invalid parameter, skip')
    elif action=='build':
        args.build = True
    elif action=='clean':
        args.clean = True
    elif action=='rebuild':
        args.rebuild = True
    elif action==None:
        Logger.warning('Skip')
    else:
        Logger.warn_not_supported(str(action))
        cUtils.SUCCESS('Skip, should be removed from default_actions_supported')    

#by default, no parameter
if len(sys.argv)==1:
    default_action = qccsdk['build']['default_action']
    Logger.info('No parameters, use default_action = %s'%str(default_action))
    if default_action != None:
        if type(default_action) == str:
            build_action2arg(default_action)
        elif type(default_action) == list:
            for act in default_action:
                build_action2arg(act)
        else:
            Logger.warn_not_supported(str(type(default_action)))
            cUtils.SUCCESS('no parameter, skip')
    else:
        cUtils.SUCCESS('no parameter, skip')

cUtils.assert_appdir_build_supported(appdir)
cUtils.assert_board_supported(board)

def gen_cmd(cur_cmd, img, is_clean=False):
    tmp_cmd = cur_cmd + ' --image %s '%img
    if is_clean==True:
        tmp_cmd += ' --clean '
    elif 'sign' in qccsdk['build'] and qccsdk['build']['sign']:
        tmp_cmd += ' --sign'
    return tmp_cmd

def build(is_clean=False):
    image_name = cUtils.appdir_2_image_name(appdir)
    build_internal = 'build.py'
    cmd = ' -b %s -o %s '%(board, cUtils.build_path)
    if appdir == 'intg':
        for img in image_name:
            cUtils.python_script_op(project_root, build_internal, gen_cmd(cmd, img, is_clean))
    else:
        cUtils.python_script_op(project_root, build_internal, gen_cmd(cmd, image_name, is_clean))

if args.clean==True:
    build(is_clean=True)

if args.build==True:    
    build()

if args.rebuild==True:    
    build(is_clean=True)
    build()

cUtils.SUCCESS()
