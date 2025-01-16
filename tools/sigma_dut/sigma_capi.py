#===============================================================================
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#===============================================================================

import time
import re
import sys
import threading
import logging
import logging.handlers
from fermion import *

logger = logging.getLogger('mylogger.sigmaCAPI')
logger.info("[DUT_LOG] sigmaCAPI test")

class sigma_CAPI:
    def __init__(self,test_params):
        self.supported_CAPI = (
            'ca_get_version',\
            'device_get_info',\
            'device_list_interfaces',\
            'sta_preset_testparameters',\
            'sta_get_info',\
            'sta_set_psk',\
            'sta_set_encryption',\
            'sta_set_ip_config',\
            'sta_associate',\
            'sta_is_connected',\
            'sta_get_bssid',\
            'sta_get_ip_config',\
            'traffic_send_ping',\
            'traffic_stop_ping',\
            'traffic_agent_reset',\
            'traffic_agent_config',\
            'traffic_agent_send',\
            'traffic_agent_receive_start',\
            'traffic_agent_receive_stop',\
            'sta_set_11n',\
            'sta_disconnect',\
            'sta_set_pwrsave',\
            'sta_get_mac_address',\
            'sta_scan',\
            'sta_set_security',\
            'sta_reassoc',\
            'sta_reset_default',\
            'ap_reset_default',\
            'ap_set_wireless',\
            'ap_set_security',\
            'ap_config_commit',\
            'ap_get_mac_address',\
            )
        self.objDut = Fermion_Command(test_params)
        self.dev_name = test_params.interface
        self.reassoc  = {}

    def caCheckDutCrash(self):
        crashTag = self.objDut.getDutCrashStatus()
        return crashTag

    def sta_reset_default(self):
        dutResult = self.objDut.initReset()

        self.objDut.enableWireless()
        self.objDut.setDevice()
        self.objDut.workMode()
        return None

    def ap_reset_default(self):
        #ap_reset_default,NAME,QCS605,program,FFD,type,DUT
        #self.objDut.initReset()
        #self.objDut.enableWireless()
        self.objDut.disconnectAp()
        self.objDut.setDevice()
        return None

    def ap_set_wireless(self,uccCommand):
        #ap_set_wireless,NAME,QCS605,channel,1,SSID,testffd,mode,11ng,width,20
        ssid           = self.objDut.getMatchingInfo('SSID,(.*?)(\,|\s)',uccCommand,1)
        channel        = self.objDut.getMatchingInfo('channel,(.*?)(\,|\s)',uccCommand,1)
        work_mode      = self.objDut.getMatchingInfo('mode,(.*?)(\,|\s)',uccCommand,1)
        band_width     = self.objDut.getMatchingInfo('width,(.*?)(\,|\s)',uccCommand,1)
        self.ap_ssid   = ssid

        self.objDut.setApWireless()
        return None

    def ap_config_commit(self,uccCommand):
        #ap_config_commit,NAME,QCS605
        logger.info ("DUT SAP ip is: %s, netmask is %s" %(self.ap_ip, self.ap_mask))
        if (self.ap_ip or self.ap_mask) is None:
            logger.info ("Please assign SAPUT ip and netmask by sigma tool!")
            sys.exit(1)
        self.objDut.connectAp(self.ap_ssid)
        self.objDut.setStaticIp(self.dev_name,self.ap_ip,self.ap_mask)
        #self.objDut.set_ap_dhcp_pool(self.dev_name,self.ap_ip)
        return None

    def ap_get_mac_address(self,uccCommand):
        #ap_get_mac_address,NAME,QCS605,interface,24G
        dutResult = self.sta_get_mac_address()
        return dutResult

    def sta_scan(self):
        dutResult = self.objDut.wlanScan()
        return None

    def caGetVersion(self):
        version = "version,1.0"
        return version

    def device_get_info(self):
        dut_version = self.objDut.getVersion()
        dutResult = "vendor,Qualcomm,model,Fermion,version,%s" %(dut_version)
        return dutResult

    def device_list_interfaces(self):
        dutResult = "interfaceType,802.11,interfaceID,%s" %(self.dev_name)
        return dutResult

    def sta_preset_testparameters(self,uccCommand):
        #sta_preset_testparameters,interface,wlan1,program,FFD,supplicant,Default,mode,11ng
        interface = self.objDut.getMatchingInfo('interface,(.*?)(\,|\s)',uccCommand,1)
        mode = self.objDut.getMatchingInfo('mode,(.*?)(\,|\s)',uccCommand,1)
        self.objDut.enableWireless()
        self.objDut.setDevice()
        return None

    def sta_get_info(self):
        dutResult = "interface,%s" %(self.dev_name)
        return dutResult

    def sta_set_psk(self,uccCommand):
        #2020-05-20 03:57:05,908-main-INFO----> sta_set_psk,interface,wlan1,ssid,PMF-5.2,passphrase,12345678,encpType,AES-CCMP,keymgmttype,WPA2,PMF,Required
        psk            = self.objDut.getMatchingInfo('passphrase,(.*),encpType',uccCommand,1)
        encpType       = self.objDut.getMatchingInfo('encpType,(.*),keymgmttype',uccCommand,1)
        keymgmttype    = self.objDut.getMatchingInfo('keymgmttype,(\w+)(\,|\s)',uccCommand,1)
        pmfTag         = self.objDut.getMatchingInfo('PMF,(\w+)(\,|\s)',uccCommand,1)
        saeType        = ""
        self.objDut.setPrivatekey(psk)
        self.objDut.pmfMode(pmfTag)
        self.objDut.setSecurity(encpType,keymgmttype,saeType)
        return None

    def ap_set_security(self,uccCommand):
        #ap_set_security,NAME,QCS605,KEYMGNT,WPA2-PSK,PSK,12345678,PMF,optional
        keymgmttype    = self.objDut.getMatchingInfo('KEYMGNT,(.*?)(\,|\s)',uccCommand,1)
        psk            = self.objDut.getMatchingInfo(',PSK,(.*?)(\,|\s)',uccCommand,1)
        pmfTag         = self.objDut.getMatchingInfo('PMF,(.*?)(\,|\s)',uccCommand,1)
        groupID        = self.objDut.getMatchingInfo('ECGroupID,(.*?)(\,|\s)',uccCommand,1)


        saeType        = self.objDut.getMatchingInfo('Type,(.*?)(\,|\s)',uccCommand,1)
        encpType       = self.objDut.getMatchingInfo('encpType,(.*?)(\,|\s)',uccCommand,1)
        logger.debug("keymgmttype is %s, psk is %s , pmfTag is %s" %(keymgmttype,psk,pmfTag))

        self.objDut.setPrivatekey(psk)
        self.objDut.setSecurity(encpType,keymgmttype,saeType)
        #self.objDut.pmfMode(pmfTag) CA not support pmf
        if groupID == "":
            self.objDut.setSaeGroup(groupID)
        return None

    def sta_set_security(self,uccCommand):
        #sta_set_security,Interface,wlan1,SSID,Wi-Fi,Type,SAE,KeyMgmtType,WPA2,EncpType,AES-CCMP,passphrase,01234567
        #sta_set_security,interface,wlan1,SSID,Wi-Fi,Type,SAE,KeyMgmtType,WPA2,EncpType,AES-CCMP,passphrase,12345678,ECGroupID,20
        #sta_set_security,interface,wlan1,type,psk,ssid,testffd,passphrase,12345678,pmf,optional,encpType,aes-ccmp,keymgmttype,wpa2
        #sta_set_security,interface,wlan0,SSID,Wi-Fi-5.6.1_250,Type,PSK-SAE,AKMSuiteType,2;8,PairwiseCipher,AES-CCMP-128,GroupCipher,AES-CCMP-128,Passphrase,12345678,PMF,Optional

        ssid           = self.objDut.getMatchingInfo('SSID,(.*?)(\,|\s)',uccCommand,1)
        saeType        = self.objDut.getMatchingInfo('Type,(.*?)(\,|\s)',uccCommand,1)
        keymgmttype    = self.objDut.getMatchingInfo('keymgmttype,(.*?)(\,|\s)',uccCommand,1)
        encpType       = self.objDut.getMatchingInfo('encpType,(.*?)(\,|\s)',uccCommand,1)
        psk            = self.objDut.getMatchingInfo('passphrase,(.*?)(\,|\s)',uccCommand,1)
        groupID        = self.objDut.getMatchingInfo('ECGroupID,((\d+ )*)',uccCommand,1)
        pmfTag         = self.objDut.getMatchingInfo('PMF,(.*?)(\,|\s)',uccCommand,1)
        akmSuite       = self.objDut.getMatchingInfo('AKMSuiteType,(.*?)(\,|\s)',uccCommand,1)#For WPA3 R3 Testbed
        pairCipher     = self.objDut.getMatchingInfo('PairwiseCipher,(.*?)(\,|\s)',uccCommand,1)
        groupCipher    = self.objDut.getMatchingInfo('GroupCipher,(.*?)(\,|\s)',uccCommand,1)

        logger.debug("ssid:%s,saeType is %s,keymgmttype is %s, encpType is %s" %(ssid,saeType,keymgmttype,encpType))
        if pmfTag == "":
            if saeType == "SAE":
                pmfTag = "required"
            elif saeType == "PSK-SAE":
                pmfTag = "optional"

        if pmfTag != "":
            self.objDut.pmfMode(pmfTag)
        if psk != "":
            self.objDut.setPrivatekey(psk)
        if akmSuite != "":
            encpType    =  pairCipher
            keymgmttype =  groupCipher

        self.objDut.setSecurity(encpType,keymgmttype,saeType)
        if groupID != "":
            self.objDut.setSaeGroup(groupID)
        #recon_para={'ssid':'','saeType':'','keymgmttype':'','encpType':'','psk':'','pmfMode':''}#for quartz reconn
        self.set_reassoc_param(ssid,saeType,keymgmttype,encpType,psk,pmfTag,groupID)
        return None

    def sta_set_encryption(self,uccCommand):
        ssid            = self.objDut.getMatchingInfo('ssid,(.*),encpType',uccCommand,1)
        encpType       = self.objDut.getMatchingInfo('encpType,(.*)',uccCommand,1)
        if (re.search('none',encpType,re.I)):
            pass
        elif (re.search('wep',encpType,re.I)):
            pass# No wep
        return None

    def sta_set_ip_config(self,uccCommand):
        wirelessInterface    = self.objDut.getMatchingInfo('interface,(\w+),',uccCommand,1)
        dhcpEnable    = self.objDut.getMatchingInfo('dhcp,(\d+),',uccCommand,1)
        wirelessIp    = self.objDut.getMatchingInfo('ip,(\d+.\d+.\d+.\d+),',uccCommand,1)
        netmask    = self.objDut.getMatchingInfo('mask,(([0-9]{1,3}[.]){3}[0-9]{1,3})',uccCommand,1)
        if (re.search('1',dhcpEnable,re.I)):
            self.objDut.initDhcp(wirelessInterface)
        elif(re.search('0',dhcpEnable,re.I)):
            self.objDut.setStaticIp(wirelessInterface,wirelessIp,netmask)
        return None

    def sta_associate(self,uccCommand):
        ssid    = self.objDut.getMatchingInfo('ssid,(.*)(\s)',uccCommand,1)
        if (ssid):
            self.objDut.connectAp(ssid)
        return None

    def sta_reassoc(self,uccCommand):
        #sta_reassoc,interface,wlan1,Channel,6,bssid,8c:fd:f0:20:88:88
        reassoc_dict = self.reassoc
        self.objDut.reAssocAp(reassoc_dict)
        return None

    def set_reassoc_param(self,ssid,saeType,keymgmttype,encpType,psk,pmfTag,groupID):
        self.reassoc = {}
        if ssid != "":
            self.reassoc['ssid'] =  ssid
        if saeType != "":
            self.reassoc['saeType'] =  saeType
        if keymgmttype != "":
            self.reassoc['keymgmttype'] =  keymgmttype
        if encpType != "":
            self.reassoc['encpType'] =  encpType
        if psk != "":
            self.reassoc['psk'] =  psk
        if pmfTag != "":
            self.reassoc['pmfTag'] =  pmfTag
        if groupID != "":
            self.reassoc['groupID'] =  groupID

    def sta_is_connected(self):
        connectTag = self.objDut.getDutConnectStatus()
        dutResult = "connected,%d" %connectTag
        return dutResult

    def sta_get_bssid(self):
        bssid = self.objDut.getBssid()
        if bssid is not None:
            dutResult = "%s" %bssid
            logger.debug("DUT bssid is:%s!!!" %bssid)
        else:
            dutResult = None
        return dutResult

    def sta_get_mac_address(self):
        MacAddr = self.objDut.getMacAddr()
        if MacAddr is not None:
            dutResult = "mac,%s" %MacAddr
            logger.debug( "DUT MacAddr is:%s!!!" %MacAddr)
        else:
            dutResult = None
        return dutResult

    def sta_get_ip_config(self,uccCommand):
        wirelessInterface    = self.objDut.getMatchingInfo('interface,(\w+)(\s)',uccCommand,1)
        (ip,netmask,dns,gateway) = self.objDut.getIPconfig(wirelessInterface)
        dutResult  = "dhcp,0"
        if ip is not None:
            dutResult = dutResult + ",ip,%s" %ip
            self.dutIp = ip
        if netmask is not None:
            dutResult = dutResult + ",mask,%s" %netmask
        if dns is not None:
            dutResult = dutResult + ",primary-dns,%s" %dns
        return dutResult

    def traffic_send_ping(self,uccCommand):
        #traffic_send_ping,destination,192.165.100.30,framesize,1000,frameRate,3,duration,10 \r\n'
        logger.info( "uccCommand is %s" %uccCommand)
        destination    = self.objDut.getMatchingInfo('destination,(\d+.\d+.\d+.\d+),',uccCommand,1)
        framesize    = self.objDut.getMatchingInfo('framesize,(\d+),',uccCommand,1)
        frameRate    = self.objDut.getMatchingInfo('frameRate,(\d+),',uccCommand,1)
        duration    = self.objDut.getMatchingInfo('duration,(\d+)(\s)',uccCommand,1)
        count       = int(duration)
        self.objDut.sendPing(destination,count,framesize)
        return None

    def traffic_stop_ping(self,uccCommand):
        #traffic_stop_ping,streamID,10 \r\n'
        time.sleep(30)# wait for ping stop
        dutOutput = self.objDut.clearDutBuffer()
        pingSucessTimes = self.objDut.ping_success_times(dutOutput)
        pingTotalTimes   = self.objDut.ping_send_times(dutOutput)
        dutResult = "sent,{},replies,{}".format(pingTotalTimes,pingSucessTimes)
        #dutResult = "sent,"+ pingTotalTimes + ",replies," + pingSucessTimes + "consectimeout,0"
        return dutResult

    def traffic_agent_reset(self):
        self.objDut.trafficReset()
        return None

    def traffic_agent_config(self,uccCommand):
        dutCommand,port,time = self.objDut.configTrafficCommand(uccCommand)
        return (dutCommand,port,time)

    def traffic_agent_send(self,uccCommand,trafficList):
        self.runBenchCli(uccCommand,trafficList)
        trafficTime = self.getTrafficSendingTime(uccCommand,trafficList)
        time.sleep(trafficTime)
        #wait more time for traffic serial output complete
        time.sleep(2)
        dutOutput = self.objDut.clearDutBuffer()

        dutResult = self.collectSendTputValue(dutOutput,uccCommand,trafficList)
        return dutResult

    def traffic_agent_receive_start(self,uccCommand,trafficList):
        self.runBenchCli(uccCommand,trafficList)
        dutOutput = self.objDut.clearDutBuffer()
        return None

    def traffic_agent_receive_stop(self,uccCommand,trafficList):
        dutOutput = self.objDut.trafficReset()
        dutResult = self.collectRecvTputValue(dutOutput,uccCommand,trafficList)
        return dutResult

    def sta_disconnect(self,uccCommand):
        dutOutput = self.objDut.disconnectAp()

    def sta_set_11n(self,uccCommand):
        return None

    def sta_setPwrsave(self,uccCommand):
        #sta_set_pwrsave,interface,wlan0,mode,on
        #sta_set_pwrsave,interface,wlan0,mode,off
        pwrMode = self.objDut.getMatchingInfo('mode,(.*)',uccCommand,1)
        self.objDut.setPowerSave(pwrMode)
        return None

    def runBenchCli(self,uccCommand,trafficList):
        #traffic_agent_send,streamID,74   streamID,8 9 10
        runningID    = self.objDut.getMatchingInfo('streamID,(.*)',uccCommand,1)
        runIDList = re.split(' ',runningID)
        dutOutput  = ""
        dutOutputs = ""
        for runID in runIDList:
            if runID.strip() != '':
                for i in range(0,len(trafficList)):
                    if int(runID) == int(trafficList[i]['id']):
                        self.objDut.runCLIQuick("%s" %(trafficList[i]['cli']))

    def collectSendTputValue(self,dutOutput,uccCommand,trafficList):
        runningID    = self.objDut.getMatchingInfo('streamID,(.*)',uccCommand,1)
        runIDList = re.split(' ',runningID)
        ValueList  = ['','','','','','']
        ValueLists = ['','','','','','']
        for runID in runIDList:
            if runID.strip() != '':
                for i in range(0,len(trafficList)):
                    if int(runID) == int(trafficList[i]['id']):
                        port = trafficList[i]['port']
                        ValueList = self.objDut.analysisSendResults(runID,port,dutOutput)
                for i in range(0,len(ValueLists)):
                    if ValueLists[i] == '':
                        ValueLists[i] = "%s" %(str(ValueList[i]))
                    else:
                        ValueLists[i] = "%s %s" %(str(ValueLists[i]),str(ValueList[i]))

        dutResult = "streamID,%s,txFrames,%s,rxFrames,%s,txPayloadBytes,%s,rxPayloadBytes,%s,outOfSequenceFrames,%s" %(ValueLists[0],ValueLists[1],ValueLists[2],ValueLists[3],ValueLists[4],ValueLists[5])

        return dutResult

    def collectRecvTputValue(self,dutOutput,uccCommand,trafficList):
        runningID    = self.objDut.getMatchingInfo('streamID,(.*)',uccCommand,1)
        runIDList = re.split(' ',runningID)
        ValueList  = ['','','','','','']
        ValueLists = ['','','','','','']
        for runID in runIDList:
            if runID.strip() != '':
                for i in range(0,len(trafficList)):
                    if int(runID) == int(trafficList[i]['id']):
                        port = trafficList[i]['port']
                        ValueList = self.objDut.analysisRecvResults(runID,port,dutOutput)
                for i in range(0,len(ValueLists)):
                    if ValueLists[i] == '':
                        ValueLists[i] = "%s" %(str(ValueList[i]))
                    else:
                        ValueLists[i] = "%s %s" %(str(ValueLists[i]),str(ValueList[i]))
        dutResult = "streamID,%s,txFrames,%s,rxFrames,%s,txPayloadBytes,%s,rxPayloadBytes,%s,outOfSequenceFrames,%s" %(ValueLists[0],ValueLists[1],ValueLists[2],ValueLists[3],ValueLists[4],ValueLists[5])
        return dutResult

    def getTrafficSendingTime(self,uccCommand,trafficList):
        #traffic_agent_send,streamID,74   streamID,8 9 10
        runningID    = self.objDut.getMatchingInfo('streamID,(.*)',uccCommand,1)
        runIDList = re.split(' ',runningID)
        timeList   = []
        for runID in runIDList:
            if runID.strip() != '':
                for i in range(0,len(trafficList)):
                    if int(runID) == int(trafficList[i]['id']):
                        timeList.append(int(trafficList[i]['time']))
        return max(timeList)

