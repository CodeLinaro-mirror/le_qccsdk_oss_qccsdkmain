
import telnetlib
import time

class TelnetLib(object):
    def __init__(self, logger, ip, username, password):
        self.ip = ip
        self.username = username
        self.password = password
        self.logger = logger
        self.conn = telnetlib.Telnet(self.ip)

    def connect(self):
        self.logger.info('telnet connect ip=%s user=%s' %(self.ip, self.username))
        self.conn.read_until(b'login: ', timeout=10)
        self.conn.write(self.username.encode('ascii') + b'\r\n')
        self.conn.read_until(b'password: ', timeout=10)
        self.conn.write(self.password.encode('ascii') + b'\r\n')
        time.sleep(2)
        command_result = self.conn.read_very_eager().decode('ascii')
        self.logger.info(command_result)
        if 'Login incorrect' not in command_result:
            self.logger.info('connected')
            return True
        else:
            self.logger.error('failed to connect')
            return False

    def disconnect(self):
        self.logger.info('telnet disconnected')
        self.conn.close()

    def run(self, cmd, check_output=None, logEnable=True):
        result = False
        self.logger.info('telnet run \'%s\' '%(cmd))
        self.conn.write(cmd.encode('ascii')+b'\r\n')
        self.logger.debug('+++++++++++++\n')
        time.sleep(2)
        command_result = self.conn.read_very_eager().decode('ascii')
        if logEnable==True:
            self.logger.info(command_result)
        self.logger.debug('--------------\n')
        if check_output==None:
            result = True
        else:
            if re.compile(check_output).search(command_result):
                result = True
        return result

