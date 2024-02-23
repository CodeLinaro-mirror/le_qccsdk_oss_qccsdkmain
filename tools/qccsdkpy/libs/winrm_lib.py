
import winrm
import re
import time

class WinrmLib(object):
    def __init__(self, logger, ip, username, password, transport='ntlm'):
        self.ip = ip
        self.username = username
        self.password = password
        self.transport = transport
        self.conn = None
        self.logger = logger

    def connect(self):
        self.logger.info('winrm connect ip=%s user=%s' %(self.ip, self.username))
        self.conn=winrm.Session('http://'+self.ip+':5985/wsman',auth=(self.username,self.password),transport=self.transport)
        return True

    def disconnect(self):
        self.logger.info('winrm disconnected')

    def run(self, cmd, check_output=None, logEnable=True):
        result = False
        self.logger.info('winrm run \'%s\' '%(cmd))
        ret=self.conn.run_cmd(cmd)
        self.logger.debug('+++++++++++++\n')
        if logEnable==True:
            self.logger.info(ret.std_out.decode())
            self.logger.info(ret.std_err.decode())
        self.logger.debug('--------------\n')
        if check_output==None:
            result = True
        else:
            if re.compile(check_output).search(ret.std_out.decode()):
                result = True
        return result

