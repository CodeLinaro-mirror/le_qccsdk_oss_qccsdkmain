#===============================================================================
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#===============================================================================

import paramiko
import re
import time

class Ssh(object):
	def __init__(self, logger, serverip, username, password, privateKeyPath, cmdDoneStr):
		self.logger = logger
		self.serverip = serverip
		self.port = 22
		self.username = username
		self.password = password
		self.privateKeyPath = privateKeyPath
		self.timeout = 7200
		self.cmdPollInterval = 0.2
		self.cmdPollRxCnt = 65535
		self.cmdDoneStr = cmdDoneStr
		if (password==None) and (privateKeyPath==None):
			self.logger.fail('none of password and privateKeyPath is valid')

	def connect(self):
	    self.logger.info('ssh connect ip=%s port=%d user=%s'
	    	%(self.serverip, self.port, self.username))
	    self.transport = paramiko.Transport(self.serverip, self.port)
	    if (self.password != None):
	    	self.transport.start_client()
	    	self.transport.auth_password(self.username, self.password)
	    elif (self.privateKeyPath != None):
	    	pkey = paramiko.RSAKey.from_private_key_file(self.privateKeyPath)
	    	self.transport.connect(username=self.username, pkey=pkey)
	    self.channel = self.transport.open_session()
	    self.channel.settimeout(self.timeout)
	    self.channel.get_pty()
	    self.channel.invoke_shell()
	    self.sync_output()
	    self.logger.info('ssh connected')
	    return True

	def disconnect(self):
	    if self.channel:
	        self.channel.close()
	    if self.transport:
	        self.transport.close()
	    self.logger.info('ssh disconnected')

	def sync_output(self):
		self.run('')

	def run(self, cmd, check_output=None, logEnable=True):
		result = False
		self.logger.info('run \'%s\' '%(cmd))
		cmd += '\r'
		self.channel.send(cmd)
		self.logger.debug('+++++++++++++\n')
		while True:
		    time.sleep(self.cmdPollInterval)
		    output = self.channel.recv(self.cmdPollRxCnt).decode(errors='ignore')
		    if logEnable==True:
		        for line in output.splitlines():
		            self.logger.info('{%s<=} %s'%(self.serverip, line))
		    #check cmd completed, search the exact cmdDoneStr, use '[]' to avoid special character
		    if re.compile('[%s]'%self.cmdDoneStr).search(output):
		    	result = True
		    	break
		    #check the specified output
		    if check_output and re.compile(check_output).search(output):
		    	result = True
		    	break
		self.logger.debug('--------------\n')
		return result

