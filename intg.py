import os
import sys
import re
import copy
import argparse

cur_dir = os.getcwd()
project_root = cur_dir
qccsdkpy_dir = os.path.join(project_root, 'tools', 'qccsdkpy')
qccsdkpy_lib_dir = os.path.join(qccsdkpy_dir, 'libs')

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

def sdk_build ():
	cUtils.rmtree('SRC-IOE-SDK')
	cUtils.python_script_op(script='tools/pack/pack_sdk.py')
	cUtils.chdir('SRC-IOE-SDK')
	#build qcc730v2_evb11_hostless
	cUtils.python_script_op(script='qccsdk.py set -b=qcc730v2_evb11_hostless')
	#cUtils.python_script_op(script='qccsdk.py set -S=sbl build')
	#cUtils.python_script_op(script='qccsdk.py set -S=demo/hello_world build')
	#cUtils.python_script_op(script='qccsdk.py set -S=demo/posix_demo build')
	cUtils.python_script_op(script='qccsdk.py set -S=demo/qcli_demo build')
	#build qcc730v2_evb13_hostless
	cUtils.python_script_op(script='qccsdk.py set -b=qcc730v2_evb13_hostless')
	#cUtils.python_script_op(script='qccsdk.py set -S=sbl build')
	#cUtils.python_script_op(script='qccsdk.py set -S=demo/hello_world build')
	#cUtils.python_script_op(script='qccsdk.py set -S=demo/posix_demo build')
	cUtils.python_script_op(script='qccsdk.py set -S=demo/qcli_demo build')
	#set default board and appdir
	cUtils.python_script_op(script='qccsdk.py set -b=qcc730v2_evb11_hostless')
	cUtils.python_script_op(script='qccsdk.py set -S=demo/qcli_demo')
	cUtils.chdir(cur_dir)

cUtils.ENTER()

if args.fsdk==True or (os.getenv("CRM_BUILDID")!=None):
	if args.nzip==False:
		cUtils.python_script_op(script='scripts/pack/pack_tgz.py --src')

if args.nrepo == False:
	#build on qcc730v2_evb11_hostless
	cUtils.python_script_op(script='qccsdk.py set -b=qcc730v2_evb11_hostless')
	#cUtils.python_script_op(script='qccsdk.py set -S=sbl build')
	cUtils.python_script_op(script='qccsdk.py set -S=prg build')
	#cUtils.python_script_op(script='qccsdk.py set -S=ftm build')
	cUtils.python_script_op(script='qccsdk.py set -S=demo/qcli_demo build')
	#cUtils.python_script_op(script='qccsdk.py set -S=001lcli build')
	#cUtils.python_script_op(script='qccsdk.py set -S=demo/hello_world build')
	#cUtils.python_script_op(script='qccsdk.py set -S=demo/posix_demo build')
	#build on qcc730v2_evb13_hostless
	cUtils.python_script_op(script='qccsdk.py set -b=qcc730v2_evb13_hostless')
	#cUtils.python_script_op(script='qccsdk.py set -S=sbl build')
	cUtils.python_script_op(script='qccsdk.py set -S=prg build')
	#cUtils.python_script_op(script='qccsdk.py set -S=ftm build')
	cUtils.python_script_op(script='qccsdk.py set -S=demo/qcli_demo build')
	#if os.name=='posix':
		#cUtils.run_cmd('echo \"fermion.ioe.1.0\" > build.log')
		#cUtils.run_cmd('make -j8 VARIANT_NAME=FERMION_IOE_PBL BOARD_NAME=qcc730v2_evb11_hostless DFU_BUILD=ON outdir=output/qcc730v2_evb11_hostless/make clean all >> build.log 2>&1')
		#cUtils.run_cmd('make -j8 VARIANT_NAME=FERMION_IOE_QCLI_DEMO BOARD_NAME=qcc730v2_evb11_hostless DFU_BUILD=ON outdir=output/qcc730v2_evb11_hostless/make clean all >> build.log 2>&1')
	#set default board and appdir
	cUtils.python_script_op(script='qccsdk.py set -b=qcc730v2_evb11_hostless')
	cUtils.python_script_op(script='qccsdk.py set -S=demo/qcli_demo')

if args.fsdk==True or (os.getenv("CRM_BUILDID")!=None):
	sdk_build()
	if args.nzip==False:
		cUtils.python_script_op(script='scripts/pack/pack_tgz.py --sdk')


cUtils.SUCCESS()

