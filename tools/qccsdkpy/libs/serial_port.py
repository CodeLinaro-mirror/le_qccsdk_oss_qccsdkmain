#===============================================================================
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#===============================================================================

# standard lib imports
import time
import serial
import re
import sys

class SerialPort ( object ) :

    def __init__ ( self, name=None, logger=None, port=None, baudRate=115200, serialCharDelayMs=0, timeout=0.08, parity=None, stopBits=None, byteSize=None  ) :

        # class variables
        self.name = name
        self.port      = port
        self.baudRate  = baudRate
        self.parity    = parity   if parity   else serial.PARITY_NONE
        self.stopBits  = stopBits if stopBits else serial.STOPBITS_ONE
        self.byteSize  = byteSize if byteSize else serial.EIGHTBITS
        self.timeout   = timeout
        self.handle    = None
        self.fileName  = None
        self.outData   = []
        self.logger    = logger
        self.timeLimit = None
        self.waitFor   = None
        self.readBuff  = None
        self.sysDefaultEncoding = 'utf-8'
        self.serialCharDelayMs = serialCharDelayMs

        if sys.getdefaultencoding() != self.sysDefaultEncoding:
            reload(sys)
            sys.setdefaultencoding(self.sysDefaultEncoding)

        # serial port number is required
        if not self.port:
            raise Exception("Serial port number not provided")

        self.open()

        return

    def set_read_params (self, timeLimit=None, waitFor=None, default=None ) :
		# setting waitFor and timeLimit to default values
        if timeLimit : self.timeLimit = timeLimit
        if waitFor   : self.waitFor   = waitFor
        if default   :
                       self.timeLimit = None
                       self.waitFor   = None

        return

    def open ( self ) :

        # open serial port
        self.logger.info('Open serial name=%s port=COM%d'%(self.name, self.port))
        self.handle   = serial.Serial( port     = "com"+str( self.port ),
                                       parity   = self.parity,
                                       stopbits = self.stopBits,
                                       baudrate = self.baudRate,
                                       bytesize = self.byteSize,
                                       timeout  = self.timeout
                                     )
        return

    def close ( self ) :
        if self.handle :
            # close serial port handle
            self.logger.info('Close serial name=%s port=COM%d'%(self.name, self.port))
            self.handle.close()
            self.handle = None

        return

    def is_open ( self ) :

        # check if serial port is open or not
        status = False
        if self.handle : status = self.handle.isOpen()

        return status

    def write ( self, cmd, cmdEnterStr='\r\n') :
        write_cnt = 0
        if self.handle :
            #self.logger.info('{%s COM%d=>} %s'%(self.name, self.port, cmd))
            strs = cmd + cmdEnterStr
            self.handle.flushInput()
            self.handle.flushOutput()
            #self.handle.w.rite( cmd +"\r" +'\n')
            #write_cnt = self.handle.write((cmd+'\r'+'\n').encode('ascii'))
            #write_cnt = self.handle.write((cmd+cmdEnterStr).encode('ascii'))
            for i in range(0,len(strs)):
                time.sleep(0.001*self.serialCharDelayMs)
                write_cnt += self.handle.write(strs[i].encode('ascii'))
        return write_cnt

    def clear_buffer(self):
        self.readBuff = ""

    def get_buffer(self):
        return self.readBuff

    def read_serial(self):
        readList = ""
        logs = ""
        time.sleep(0.1)
        try:
            readList = self.handle.readlines()
        except:
            pass
        if len(readList) > 0:
            for line in readList:
                reading = str(line, encoding = "ISO-8859-1")
                self.logger.info('{%s COM%d<=} %s'%(self.name, self.port, reading))
                logs = logs + reading
            if (logs != ""):
                self.readBuff += logs
        return len(logs)

    def reads(self, waitMsg1, waitMsg2, timeOut):
        if (timeOut):
            msg1Done = False
            if waitMsg1 == None:
                msg1Done = True
            msg2Done = False
            if waitMsg2 == None:
                msg2Done = True
            startTime = time.time()
            while 1:
                length=self.read_serial()
                if length>0:
                    if waitMsg1:
                        found1 = re.search(waitMsg1, self.readBuff)
                        if found1 != None:
                            msg1Done = True
                    if waitMsg2:
                        found2 = re.search(waitMsg2, self.readBuff)
                        if found2 != None:
                            msg2Done = True
                    if (msg1Done==True) and (msg2Done==True):
                        return True
                if startTime and ((time.time() - startTime) > timeOut):
                    return False
        else:
            self.read_serial()
            return True

