import os
import sys
import re
import copy

cur_dir = os.getcwd()
project_root = cur_dir
qccsdkpy_dir = os.path.join(project_root, 'tools', 'qccsdkpy')
qccsdkpy_lib_dir = os.path.join(qccsdkpy_dir, 'libs')

sys.path.append(qccsdkpy_dir)
sys.path.append(qccsdkpy_lib_dir)

from utils import Utils

argvs = copy.deepcopy(sys.argv)
cUtils = Utils(project_root, qccsdkpy_dir)
Logger = cUtils.logger

qccsdk_supported_scripts = cUtils.CfgInternal['qccsdk_supported_scripts']

script_index = []
script_fake = False

argvs.pop(0)
argvs_cnt = len(argvs)
for n in range(0, argvs_cnt):
	if re.match('[a-zA-Z]', argvs[n]):
		#script must be started with character
		script = argvs[n]
		cUtils.qassert((script in qccsdk_supported_scripts), '%s is not supported script'%script)
		script_index.append(n)
	elif re.match('[-]', argvs[n]):
		#parameter must be started with '-'
		continue
	else:
		cUtils.FAIL('%s is invalid arg'%argvs[n])
script_cnt = len(script_index)

parent_parameters_cnt = 0
if (script_cnt==0):
	#no scripts
	parent_parameters_cnt = argvs_cnt
else:
	parent_parameters_cnt = script_index[0]
	#pesudo script index is appended after last script
	script_index.append(argvs_cnt)

#hanle parrent parameter
for n in range(0, parent_parameters_cnt):
	if argvs[n] in ['-v', '--verbose', '--debug']:
		Logger.enableLogDebug()
	elif argvs[n] in ['-h', '--help']:
		Logger.info('help information:')
		Logger.info('-v, --verbose, --debug: to output more debug information')
		Logger.info('-h, --help: to show help information')
		Logger.info('--fake: to show all scripts but does not run')
		Logger.info('pattern: [scriptA --paramA1 --paramA2] [scriptB --paramB1 --paramB2], will call scriptA, then scriptB')
		Logger.info('example: [buid --build flash --flash], will call build script to build, flash script to flash')
		Logger.info('supported scripts include:')
		for script_tmp in qccsdk_supported_scripts:
			Logger.info(script_tmp)
		cUtils.SUCCESS('Show help info then EXIT')
	elif argvs[n] in ['--fake']:
		script_fake = True
	else:
		Logger.info('%s is not supported yet'%argvs[n])

cUtils.ENTER()

#handle scripts
for m in range(0, script_cnt):
	script = argvs[script_index[m]]+'.py'
	cmd = ''
	for n in range(script_index[m]+1,script_index[m+1]):
		cmd += ' %s '%argvs[n]
	if script_fake==True:
		Logger.info('run %d: %s %s'%(m,script,cmd))
	else:
		Logger.debug('run %d: %s %s'%(m,script,cmd))
		cUtils.python_script_op_local(script, cmd)

cUtils.SUCCESS()
