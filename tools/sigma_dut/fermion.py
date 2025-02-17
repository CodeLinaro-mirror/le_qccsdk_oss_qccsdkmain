#===============================================================================
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#===============================================================================

import time
import re
import serial
import threading
import logging
import logging.handlers
from cli_helper import CLI_Helper

logger = logging.getLogger('mylogger.fermion')
logger.info("[DUT_LOG] Fermion test\r\n")

class Fermion_Command:
    def __init__(self,test_params):
        self.dutCrashTag  = 0
        self.ser = CLI_Helper(test_params.serial_port)
        self.dev_name = test_params.interface

    def getDutCrashStatus(self):
        tag = self.ser.checkdutCrashTag()
        return tag

    def clearDutBuffer(self):
        dutOutput = self.ser.readSerialbuffer()
        return dutOutput

    def runCLIQuick(self,command):
        dutOutput = self.ser.writeSerialQuick(command)
        #wait for serial output complete
        time.sleep(0.05)

    def initReset(self):
        dutOutput = self.ser.writeSerial("platform reset")
        return dutOutput

    def setApWireless(self):
        dutOutput = self.ser.writeSerial("wlan SetOperatingMode ap")
        dutOutput = self.ser.writeSerial("wlan Set11nHTCap ht20")
        dutOutput = self.ser.writeSerial("wlan SetChannel 1")
        return None

    def enableWireless(self):
        dutOutput = self.ser.writeSerial("wlan enable")
        return dutOutput

    def wlanScan(self):
        dutOutput = self.ser.writeSerial("wlan scan")
        return dutOutput

    def setDevice(self):
        dev = ""
        if self.dev_name == "wlan1":
            dev = "1"
        elif self.dev_name == "wlan0":
            dev = "0"
        dutOutput = self.ser.writeSerial("wlan SetDevice %s" %dev)
        return dutOutput

    def workMode(self):
        dutOutput = self.ser.writeSerial("wlan SetOperatingMode station")
        return dutOutput

    def powerSave(self):
        dutOutput = self.ser.writeSerial("wlan EnableGTX 0")
        dutOutput = self.ser.writeSerial("wlan EnableLPL 0")
        return dutOutput

    def setPowerSave(self,params):
        if (re.search("on",params,re.I)):
            dutOutput = self.ser.writeSerial("wlan SetPowerMode 1")
        else:
            dutOutput = self.ser.writeSerial("wlan SetPowerMode 0")
        return dutOutput

    def disableDeepSleep(self):
        dutOutput = self.ser.writeSerial("LP DeepSleep Disable")
        return dutOutput

    def enableAggr(self):
        dutOutput = self.ser.writeSerial("wlan SetAggregationParameters 0xff 0xff")
        return dutOutput

    def getVersion(self):
        dutOutput = self.ser.writeSerial("ver")
        pattern   = "crm num: (.*)"
        dutOutput    = self.getMatchingInfo(pattern,dutOutput,1)
        return dutOutput

    def getBssid(self):
        dutOutput = self.ser.writeSerial("wlan info")
        pattern   = "bssid(\s+)=(\s+)(([0-9A-F]{1,2}[:]){5}[0-9A-F]{1,2})"
        bssid    = self.getMatchingInfo(pattern,dutOutput,3)
        return bssid

    def getMacAddr(self):
        dutOutput = self.ser.writeSerial("wlan info")
        pattern   = "Mac Addr(\s+)=(\s+)(([0-9A-F]{1,2}[:]){5}[0-9A-F]{1,2})"
        MacAddr    = self.getMatchingInfo(pattern,dutOutput,3)
        return MacAddr

    def getDutConnectStatus(self):
        dutOutput = self.ser.writeSerial("wlan info")
        if re.search("ssid(\s+)=(\s+)",dutOutput):
            return 1
        else:
            return 0

    def getIPconfig(self,wirelessInterface):
        dutOutput = self.ser.writeSerial("net ifconfig %s" %wirelessInterface)
        pattern   = "IPv4:(\s+)(\d+.\d+.\d+.\d+)(\s+)Subnet Mask: (\d+.\d+.\d+.\d+)(\s+)Default Gateway:(\s+)(([0-9]{1,3}[.]){3}[0-9]{1,3})"
        ip         = None
        netmask    = None
        dns        = None
        gateway    = None

        ip         = self.getMatchingInfo(pattern,dutOutput,2)
        netmask    = self.getMatchingInfo(pattern,dutOutput,4)
        gateway    = self.getMatchingInfo(pattern,dutOutput,7)
        self.dutIp = ip
        return (ip,netmask,dns,gateway)

    def trafficReset(self):
        self.ser.writeSerialQuick("WiFiCert udpquit")
        #wait for the RX/TX traffic result.
        time.sleep(1.5)
        dutOutput = self.ser.readSerialbuffer()
        return dutOutput

    def setPrivatekey(self,psk):
        dutOutput = self.ser.writeSerial("wlan SetWpaPassPhrase %s" %psk)
        return dutOutput

    def setEnterpriseParms(self,eapMethod,username,password):
        if re.match("TTLS",eapMethod,re.I):
            dutOutput = self.ser.writeSerial("wlan SetWpaCertParameters TTLS-MSCHAPV2 ioeDevice %s %s" %(username,password))
        elif re.match("PEAP",eapMethod,re.I):
            dutOutput = self.ser.writeSerial("wlan SetWpaCertParameters PEAP-MSCHAPV2 ioeDevice %s %s" %(username,password))
        elif re.match("TLS",eapMethod,re.I):
            dutOutput = self.ser.writeSerial("wlan SetWpaCertParameters TLS %s %s %s 0 rootCA wifiuser 0 0 0x1006b" %(username,username,password))
            #rootCA is calist and wifiuser is certificate,should modify by qlan
        return dutOutput

    def setSecurity(self,encpType,keymgmttype,saeType):
        if (re.match('^TKIP$',encpType,re.I)):
            dutEncpType = "TKIP"
        elif (re.match('^AES-CCMP$',encpType,re.I)):
            dutEncpType = "CCMP"
        elif (re.match('^AES-CCMP-128$',encpType,re.I)):
            dutEncpType = "CCMP"

        if dutEncpType == "":
            if "WPA2" in keymgmttype:
                dutEncpType = "CCMP"
            elif "WPA" in keymgmttype:
                dutEncpType = "TKIP"

        if (re.match('^WPA2$',keymgmttype,re.I)):
            dutkeymgmttype = "WPA2"
        elif (re.match('^WPA$',keymgmttype,re.I)):
            dutkeymgmttype = "WPA"
        elif (re.match('^WPA2-PSK$',keymgmttype,re.I)):
            dutkeymgmttype = "WPA2"
        elif (re.match('^WPA-PSK$',keymgmttype,re.I)):
            dutkeymgmttype = "WPA"
        else:
            dutkeymgmttype = ""

        if (re.match('^SAE$',saeType,re.I)):
            dutkeymgmttype = "SAE"
        elif (re.match('^PSK-SAE$',saeType,re.I)):
            dutkeymgmttype = "SAE_WPA2"

        dutOutput = self.ser.writeSerial("wlan SetWpaParameters %s %s %s" %(dutkeymgmttype,dutEncpType,dutEncpType))
        return dutOutput

    def setEnterpriseSecurity(self,encpType,keymgmttype):
        if (re.match('^TKIP$',encpType,re.I)):
            dutEncpType = "TKIP"
        elif (re.match('^AES-CCMP$',encpType,re.I)):
            dutEncpType = "CCMP"
        else:
            dutEncpType = ""
        logger.info( "keymgmttype in the params is %s !!!" %keymgmttype)

        if (re.match('^WPA2$',keymgmttype,re.I)):
            dutkeymgmttype = "WPA2CERT"
        elif (re.match('^WPA$',keymgmttype,re.I)):
            dutkeymgmttype = "WPACERT"
        else:
            dutkeymgmttype = ""

        dutOutput = self.ser.writeSerial("wlan SetWpaParameters %s %s %s" %(dutkeymgmttype,dutEncpType,dutEncpType))
        return dutOutput

    def setSaeGroup(self,groupID):
        dutOutput = self.ser.writeSerial("wlan SetSaeGroups %s" %groupID)
        return dutOutput

    def pmfMode(self,pmf):
        num = ""
        if re.search('require',pmf,re.I):
            num = 2
        elif re.search('optional',pmf,re.I):
            num = 1
        elif re.search('disable',pmf,re.I):
            num = 0
        else:
            return None
        dutOutput = self.ser.writeSerial("wlan SetPmfMode %s" %num)
        return dutOutput

    def reAssocAp(self,reassoc_dict):
        self.disconnectAp()
        if "pmfTag" in reassoc_dict.keys():
            self.pmfMode(reassoc_dict['pmfTag'])
        if "psk" in reassoc_dict.keys():
            self.setPrivatekey(reassoc_dict['psk'])
        if "saeType" in reassoc_dict.keys():
            self.setSecurity(reassoc_dict['encpType'],reassoc_dict['keymgmttype'],reassoc_dict['saeType'])
        if "groupID" in reassoc_dict.keys():
            self.setSaeGroup(reassoc_dict['groupID'])
        if "ssid" in reassoc_dict.keys():
            self.connectAp(reassoc_dict['ssid'])

    def connectAp(self,ssid):
        self.ser.writeSerial("wlan SetAMSDU rx enable")
        dutOutput = self.ser.writeSerial("wlan Connect %s" %ssid)
        return dutOutput

    def disconnectAp(self):
        dutOutput = self.ser.writeSerial("wlan disconnect")

    def setUapsd(self,params):
        dutOutput = self.ser.writeSerial("wlan setSTAUapsd %s" %params)
        return dutOutput

    def sendUapsd(self,params,readTag):
        dutOutput = ""
        if readTag:
            dutOutput = self.ser.writeSerial("Net uapsdc send %s" %params)
        else:
            self.ser.writeSerialQuick("Net uapsdc send %s" %params)
        return dutOutput

    def connectEnterpriseAp(self,eapMethod,ssid,username,password,encpType,keymgmttype):
        self.setEnterpriseSecurity(encpType,keymgmttype)
        self.setEnterpriseParms(eapMethod,username,password)
        dutOutput = self.ser.writeSerial("wlan Connect %s" %ssid)
        return dutOutput

    def initDhcp(self,interface):
        dutOutput = self.ser.writeSerial("net dhcpv4c %s new" %interface)
        return dutOutput

    def setStaticIp(self,interface,ip,mask):
        gateway = "192.165.100.1"
        #m = re.compile('(([0-9]{1,3}[.]){3}[0-9]{1,3})',gateway)
        #m.replace('(([0-9]{1,3}[.]){3}[0-9]{1,3})','(([0-9]{1,3}[.]){3})')
        dutOutput = self.ser.writeSerial("net ifconfig %s %s %s %s" %(interface,ip,mask,gateway))
        return dutOutput

    def set_ap_dhcp_pool(self,interface,ip):
        gateway = "192.165.100.1"
        #m = re.compile('(([0-9]{1,3}[.]){3}[0-9]{1,3})',gateway)
        #m.replace('(([0-9]{1,3}[.]){3}[0-9]{1,3})','(([0-9]{1,3}[.]){3})')
        dutOutput = self.ser.writeSerial("net dhcpv4s %s pool 192.165.100.100 192.165.100.200 3600" %(interface))
        return dutOutput

    def sendPing(self,destination,count,framesize):
        dutOutput = self.ser.writeSerialQuick(("net ping " + destination + " -c " + str(count) + " -s " + str(framesize)))
        return dutOutput

    def ping_success_times(self,dutOutput):
        pingSucessTimes = self.getMatchingInfo('Received packets = (\d+)',dutOutput,1)

        if pingSucessTimes == "":
            pingSucessTimes =  dutOutput.count("bytes from")

        return pingSucessTimes

    def ping_send_times(self,dutOutput):
        sentTimes = self.getMatchingInfo('Sent packets = (\d+)',dutOutput,1)
        if sentTimes == "":
            pingSucessTimes = dutOutput.count("timed out")
            pingFailureTimes =  dutOutput.count("bytes from")
            sentTimes = pingSucessTimes + pingFailureTimes
        return sentTimes

    def configTrafficCommand(self,uccCommand):
        #traffic_agent_config,profile,Multicast,direction,send,destination,224.0.0.5,destinationPort,223,source,192.165.100.90,sourcePort,223,duration,90,trafficClass,BestEffort,payloadSize,350,frameRate,50
        #traffic_agent_config,profile,Multicast,direction,receive,sourcePort,223,destinationPort,223,destination,224.0.0.5
        #traffic_agent_config,profile,IPTV,direction,receive,source,192.165.100.30,sourcePort,4600,destinationPort,4600
        #traffic_agent_config,profile,uapsd,direction,receive,source,192.165.100.30,sourcePort,4600,destinationPort,4600
        profile    = self.getMatchingInfo('profile,(\w+),',uccCommand,1)
        direction    = self.getMatchingInfo('direction,(\w+),',uccCommand,1)
        destinationPort    = self.getMatchingInfo('destinationPort,(\d+),',uccCommand,1)
        source    = self.getMatchingInfo('source,(([0-9]{1,3}[.]){3}[0-9]{1,3}),',uccCommand,1)
        sourcePort    = self.getMatchingInfo('sourcePort,(\d+),',uccCommand,1)
        if re.search('send',direction,re.I):
            duration    = self.getMatchingInfo('duration,(\d+),',uccCommand,1)
            trafficClass    = self.getMatchingInfo('trafficClass,(\w+),',uccCommand,1)
            payloadSize    = self.getMatchingInfo('payloadSize,(\d+),',uccCommand,1)
            frameRate    = self.getMatchingInfo('frameRate,(.*)(\s)',uccCommand,1)
            destination    = self.getMatchingInfo('destination,(([0-9]{1,3}[.]){3}[0-9]{1,3}),?',uccCommand,1)
        else:
            duration       = ""
            trafficClass   = ""
            payloadSize    = ""
            frameRate      = ""

        commandLine = ''
        port = ''
        time = ''
        protocol = ""
        if profile:
            if   re.search('File_Transfer',profile,re.I):
                protocol = ''
            elif re.search('Multicast',profile,re.I):
                protocol = ''
            elif re.search('IPTV',profile,re.I):
                protocol = ''
            elif re.search('Transaction',profile,re.I):
                protocol = '-e '
            elif re.search('Start_Sync',profile,re.I):
                protocol = ''


        if frameRate:
            if re.match("0",frameRate,re.I):
                delay = 0
            else:
                delay    = int(1000/float(frameRate))#delay in ms
                if delay < 1:
                    delay = 1

        if trafficClass:
            if   re.search('Voice',trafficClass,re.I):
                traffcTc = '0xE0'
            if   re.search('Video',trafficClass,re.I):
                traffcTc = '0xA0'
            if   re.search('BestEffort',trafficClass,re.I):
                traffcTc = '0x00'
            if   re.search('Background',trafficClass,re.I):
                traffcTc = '0x32'

        if re.search('Receive',direction,re.I):
            if  re.search('Multicast',profile,re.I):
                destination    = self.getMatchingInfo('destination,(([0-9]{1,3}[.]){3}[0-9]{1,3}),?',uccCommand,1)
                dutIp = self.dutIp
                commandLine = "WiFiCert udp -s "+ protocol + " -p " + sourcePort + " -B " + destination
            else:
                commandLine = "WiFiCert udp -s "+ protocol + " -p " + sourcePort
            port = sourcePort
        elif  re.search('Send',direction,re.I):
            commandLine = "WiFiCert udp -c " + destination + protocol + " -p " + destinationPort + " -l " + payloadSize + " -i " + str(delay) + " -S " + traffcTc + " -t " + str(duration)
            port = destinationPort
            time = duration
        return (commandLine,port,time)

    def analysisSendResults(self,runID,port,dutOutput):
        # Sent 1756 packets, 2458400 bytes to 192.165.100.30 5601
        # Sent 14423 packets, 20192200 bytes to 192.165.100.30 5600
        ValueList  = ['0','0','0','0','0','0']
        ValueList[0] = str(runID)

        matchSendPattern = re.search('Sent (.*) packets, (.*) bytes to (.*) %s' %port,dutOutput)
        if matchSendPattern is not None:
            ValueList[1] = matchSendPattern.group(1)
            ValueList[3] = matchSendPattern.group(2)
        matchRecvPattern = re.search('Received (.*) bytes, Packets (.*)  from :%s' %port,dutOutput)
        if matchRecvPattern is not None:
            rxPayloadBytes = matchRecvPattern.group(1)
            rxPacketsNum = matchRecvPattern.group(2)
            logger.debug( "rxPayloadBytes is %s" %rxPayloadBytes)
            logger.debug( "rxPacketsNum is %s" %rxPacketsNum)
            ValueList[4] = int(rxPayloadBytes)
            ValueList[2] = int(rxPacketsNum)
        ValueList[5] = 0
        return ValueList
    def analysisRecvResults(self,runID,port,dutOutput):
        ValueList  = ['0','0','0','0','0','0']
        ValueList[0] = str(runID)
        matchSendPattern = re.search('Sent (.*) packets, (.*) bytes',dutOutput)

        if matchSendPattern is not None:
            ValueList[1] = matchSendPattern.group(1)
            ValueList[3] = matchSendPattern.group(2)
        matchRecvPattern = re.search('Received (.*) bytes, Packets (.*) from(.*):%s' %port,dutOutput)
        if matchRecvPattern is not None:
            rxPayloadBytes = matchRecvPattern.group(1)
            rxPacketsNum = matchRecvPattern.group(2)
            ValueList[4] = int(rxPayloadBytes)
            ValueList[2] = int(rxPacketsNum)
        ValueList[5] = 0
        return ValueList

    def getMatchingInfo(self,pattern,dutOutput,feedbackLocation = 0):
        matchInfo = ""
        m = re.search(pattern,dutOutput,re.I)
        if m is not None:
            matchInfo = m.group(feedbackLocation)
        return matchInfo

