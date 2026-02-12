import os
import sys
import re
import copy
import argparse
import shutil

cur_dir = os.getcwd()
project_root = cur_dir
qccsdkpy_dir = os.path.join(project_root, 'tools', 'qccsdkpy')
qccsdkpy_bin_dir = os.path.join(project_root, 'modules', 'wifi', 'bin')
qccsdkpy_lib_dir = os.path.join(qccsdkpy_dir, 'libs')
is_HY11_build = os.path.isfile(cur_dir + "/../prebuilt_HY11/libwifi_core.a")
sys.path.append(qccsdkpy_dir)
sys.path.append(qccsdkpy_lib_dir)

from utils import Utils

parser = argparse.ArgumentParser(description='integration', formatter_class=argparse.RawDescriptionHelpFormatter)
parser.add_argument('--nrepo', required=False, action='store_true', help='cancle to build repo')
parser.add_argument('--fsdk', required=False, action='store_true', help='force to generate SRC-IOE-SDK')
parser.add_argument('--nzip', required=False, action='store_true', help='cancle to zip of SRC-IOE-SDK')
args = parser.parse_args()

cUtils = Utils(project_root, qccsdkpy_dir)
Logger = cUtils.logger

def apps_build (board_name, ext_demo=False):
        cUtils.python_script_op(script='qccsdk.py set -b=%s'%board_name)
        cUtils.python_script_op(script='qccsdk.py set -S=sbl build')
        cUtils.python_script_op(script='qccsdk.py set -S=prg build')
        cUtils.python_script_op(script='qccsdk.py set -S=ftm build')
        cUtils.python_script_op(script='qccsdk.py set -S=demo/hello_world build')
        cUtils.python_script_op(script='qccsdk.py set -S=demo/posix_demo build')
        cUtils.python_script_op(script='qccsdk.py set -S=demo/fs_demo build')
        cUtils.python_script_op(script='qccsdk.py set -S=demo/qcli_demo build')
        #cUtils.python_script_op(script='qccsdk.py set -S=demo/matter_demo build')
        cUtils.python_script_op(script='qccsdk.py set -S=demo/qat_demo build')
        cUtils.python_script_op(script='qccsdk.py set -S=demo/powertest_demo build')
        cUtils.python_script_op(script='qccsdk.py set -S=demo/ambient_power_demo build')

def set_default ():
	cUtils.python_script_op(script='qccsdk.py set -b=qcc730v2_evb11_hostless')
	cUtils.python_script_op(script='qccsdk.py set -S=demo/qcli_demo')


cUtils.ENTER()

#generate package for original repo
#if args.fsdk==True or (os.getenv("CRM_BUILDID")!=None):
#	if args.nzip==False:
#		cUtils.python_script_op(script='tools/pack/pack_tgz.py --src')

#build repo
if args.nrepo == False:
    if is_HY11_build:
        shutil.copy(cur_dir + "/../prebuilt_HY11/libwifi_core.a", qccsdkpy_bin_dir)
    else:
        cUtils.python_script_op(script='build.py -i FERMION_WIFI_LIB -o output/wifi_lib')
        cUtils.python_script_op(script='build.py -i FERMION_WIFI_LIB -o output/wifi_lib -p')
    apps_build(board_name='qcc730v2_evb11_hostless')
    apps_build(board_name='qcc730v2_evb13_hostless')
	#cUtils.python_script_op(script='qccsdk.py set -S=001lcli build')
	#apps_build(board_name='qcc730v2_evb13_hostless')
	#apps_build(board_name='qcc730v2_evb12_hostless')
	#apps_build(board_name='qcc730v2_socket')
	#if os.name=='posix':
		#cUtils.run_cmd('echo \"fermion.ioe.1.0\" > build.log')
		#cUtils.run_cmd('make -j8 VARIANT_NAME=FERMION_IOE_PBL BOARD_NAME=qcc730v2_evb11_hostless DFU_BUILD=ON outdir=output/qcc730v2_evb11_hostless/make clean all >> build.log 2>&1')
		#cUtils.run_cmd('make -j8 VARIANT_NAME=FERMION_IOE_QCLI_DEMO BOARD_NAME=qcc730v2_evb11_hostless DFU_BUILD=ON outdir=output/qcc730v2_evb11_hostless/make clean all >> build.log 2>&1')
	#set_default()

#build sdk and generate package
if args.fsdk==True or (os.getenv("CRM_BUILDID")!=None):
    if not is_HY11_build:
        #cUtils.rmtree('SRC-IOE-SDK')
        #cUtils.python_script_op(script='tools/pack/pack_sdk.py')
        print("no need to pack from here\r\n")
    else:
        print("HY_11 build, no need to pack\r\n")
	
    #cUtils.chdir(cur_dir)
    #if args.nzip==False:
    #    cUtils.python_script_op(script='tools/pack/pack_tgz.py --sdk')


cUtils.SUCCESS()

