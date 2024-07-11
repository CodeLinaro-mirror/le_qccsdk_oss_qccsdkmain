#===============================================================================
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#===============================================================================

import logging
import time
import os
import sys

class Logger ( object ) :
    def __init__ ( self, dispatchName=None, logdir=None, stream2file=True, logging2file=True, filename_timestamp=False, file_append=False, logging_timestamp=True) :
        # initialize logger
        if dispatchName==None:
            self.dispatchName       = ''
        else:
            self.dispatchName       = '_'+dispatchName
        self.caller_filename = os.path.splitext(os.path.basename(sys.argv[0]))[0]
        self.localtime = time.localtime()
        self.startDateTimeStamp = time.strftime("%Y%m%d-%H%M%S", self.localtime)
        if filename_timestamp==True:
            self.logfileFullName            = "%s_%s%s.log" % (self.startDateTimeStamp, self.caller_filename, self.dispatchName )
        else:
            self.logfileFullName            = "%s%s.log" % (self.caller_filename, self.dispatchName )
        self.logfileName = os.path.splitext(self.logfileFullName)[0]
        if logging_timestamp==True:
            self.formatter              = logging.Formatter('%(asctime)s %(levelname)s %(message)s', datefmt='%Y-%m-%d %H:%M:%S')
        else:
            self.formatter              = logging.Formatter('%(levelname)s %(message)s')
        self.reducedFormatter       = logging.Formatter('%(message)s')
        #self.logsDirPath            = os.path.abspath( os.path.join( os.path.split( os.path.abspath(__file__) )[0],"..\..\ABT_logs") )
        self.logsDirPath = logdir
        self.logfileFullPath = os.path.join( self.logsDirPath, self.logfileFullName )
        if (os.path.isdir(self.logsDirPath) == False):
            os.makedirs(self.logsDirPath)
        if file_append==False:
            if os.path.isfile(self.logfileFullPath):
                #print('remove file=%s'%self.logfileFullPath)
                os.remove(self.logfileFullPath)
        self.log = logging.getLogger( self.logfileFullName )
        self.log.setLevel( logging.DEBUG )
        self.handlers = []
        # add out filel log file and console handler
        if (logging2file == True):
            self.addHandler( self.logfileFullName, level=logging.DEBUG )
        if stream2file==True:
            self.addHandler( "Stream", stream=True)

    def addHandler( self, logFileName, level=logging.INFO, stream=False):
        """Function setup as many loggers as you want"""
        if stream : handler = logging.StreamHandler()
        else      : handler = logging.FileHandler( os.path.join( self.logsDirPath, logFileName ), encoding="utf-8",mode="a")
        self.handlers.append(handler)
        handler.setFormatter(self.formatter)
        handler.setLevel(level)
        self.log.addHandler(handler)
        return handler

    def setLogLevel(self, level):
        #print(self.handlers)
        for handler in self.handlers:
            handler.setLevel(level)

    def enableLogDebug(self):
        #print('enableLogDebug')
        self.setLogLevel(logging.DEBUG)

    def reduceFormater(self):
        for handler in self.handlers:
            handler.setFormatter(self.reducedFormatter)

    def restoreFormater(self):
        for handler in self.handlers:
            handler.setFormatter(self.formatter)

    def info ( self, data) :
        # remove NULL values
        data = data.replace('\0','').replace("\n\r", "\n")
        for subLine in data.split("\n") :
            if subLine :
                subLine = subLine.replace("\r", "")
                self.log.info( subLine )

    def debug ( self, data ) :
        # remove NULL values
        data = data.replace('\0','').replace("\n\r", "\n")
        for subLine in data.split("\n") :
            if subLine :
                subLine = subLine.replace("\r", "")
                self.log.debug( subLine )

    def fail ( self, data ) :
        self.log.error(self.caller_filename + " Error: " + data)
        sys.exit(1)

    def warning ( self, data ) :
        # remove NULL values
        data = data.replace('\0','').replace("\n\r", "\n")
        for subLine in data.split("\n") :
            if subLine :
                subLine = subLine.replace("\r", "")
                self.log.warning( subLine )

    def error ( self, data ) :
        # remove NULL values
        data = data.replace('\0','').replace("\n\r", "\n")
        for subLine in data.split("\n") :
            if subLine :
                subLine = subLine.replace("\r", "")
                self.log.error( subLine )
