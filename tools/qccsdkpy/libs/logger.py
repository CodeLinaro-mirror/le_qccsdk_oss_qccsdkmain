#===============================================================================
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#===============================================================================

import logging
import time
import os
import sys

import cfg_common as CfgCommon

class Logger ( object ) :
    def __init__ ( self, dispatchName=None, logdir=None) :
        # initialize logger
        if dispatchName==None:
            self.dispatchName       = ''
        else:
            self.dispatchName       = '_'+dispatchName
        self.caller_filename = os.path.splitext(os.path.basename(sys.argv[0]))[0]
        self.localtime = time.localtime()
        self.startDateTimeStamp = time.strftime("%Y%m%d-%H%M%S", self.localtime)
        self.logfileFullName            = "%s_%s%s.log" % (self.startDateTimeStamp, self.caller_filename, self.dispatchName )
        self.logfileName = os.path.splitext(self.logfileFullName)[0]
        self.formatter              = logging.Formatter('%(asctime)s %(levelname)s %(message)s', datefmt='%Y-%m-%d %H:%M:%S')
        self.reducedFormatter       = logging.Formatter('%(message)s')
        #self.logsDirPath            = os.path.abspath( os.path.join( os.path.split( os.path.abspath(__file__) )[0],"..\..\ABT_logs") )
        self.logsDirPath = logdir
        if (os.path.isdir(self.logsDirPath) == False):
            os.makedirs(self.logsDirPath)
        self.log                    = logging.getLogger( self.logfileFullName )
        self.log.setLevel( logging.DEBUG )
        self.handlers = []
        self.CfgCommon = CfgCommon
        # add out filel log file and console handler
        if (self.CfgCommon.CFG_LOG_FILE_ENABLE == True):
            self.addHandler( self.logfileFullName, level=logging.DEBUG )
        self.addHandler( "Stream", stream=True )

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
