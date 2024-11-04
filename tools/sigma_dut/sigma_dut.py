#===============================================================================
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#===============================================================================

import socket
import select
import time
from datetime import datetime
import threading
import re
import os
import subprocess
import sys
import logging
import logging.handlers
import logging.config
import argparse
from sigma_capi import sigma_CAPI

#################################################################################
'''
This is sigma control agent tool,  act as TCP server, handle UCC CAPI message.

sigma_dut.py -ip <server ip> -p  <port number> -S <wireless interface> -s <serial-port> -t <test item>
	   -h  -v

	options:
	  -h, --help	  show this help message and exit
	  -ip IP		  sigma control agent IP address, local IP address of PC running this tool.
	  -p PORT		  sigma control agent port, the port number running this tool.
	  -S INTERFACE	  DUT interface: wlan1
	  -s SERIAL_PORT  DUT serial port like: COM6
	  -t TEST		  WFA cert test item: 11n
	  -v, --version   show program's version number and exit

example:

python sigma_dut.py -ip 192.168.250.41 -p 9000 -S wlan1 -s com6 -t 11n

PC runtime environment:
	a. Install python3
	b. Install serial module
		pip install pyserial or
		python -m pip install pyserial

'''
parser = argparse.ArgumentParser()

parser.add_argument('-ip', action='store', required=True, dest='ip',
        help='sigma control agent IP address, local IP address of PC running this tool.')

parser.add_argument('-p', action='store', required=True, dest='port',
        help='sigma control agent port, the port number running this tool.')

parser.add_argument('-S', action='store', required=True, dest='interface',
        help='DUT interface: wlan1')

parser.add_argument('-s', action='store', required=True, dest='serial_port',
        help='DUT serial port like: COM6')

parser.add_argument('-t', action='store', dest='test', default="11n",
        help='WFA cert test item: 11n')

parser.add_argument('-v','--version', action='version', version='%(prog)s 1.0')

test_params = parser.parse_args()

#################################################################################
logger = logging.getLogger("mylogger")
logger.setLevel(logging.DEBUG)

now = datetime.now()
dt_string = now.strftime("%Y-%m-%d_%H-%M-%S")
log_file_name = "sigma_dut_logs_{}.log".format(dt_string)
if not os.path.exists("./log"):
    os.makedirs("./log")

logfile = os.path.join('./log',log_file_name)
file_handler = logging.FileHandler(logfile,encoding='UTF-8')

file_handler.setLevel(logging.DEBUG)

stream_handler = logging.StreamHandler()
stream_handler.setLevel(logging.INFO)

log_format=logging.Formatter('[%(asctime)s] [%(filename)s:%(lineno)d] %(levelname)s: %(message)s')
file_handler.setFormatter(log_format)
stream_handler.setFormatter(log_format)

#add handler to logger
logger.addHandler(file_handler)
logger.addHandler(stream_handler)

logger.info("Sigma tool; Version,1.0")
#################################################################################

objDut = sigma_CAPI(test_params)
CAPI = objDut.supported_CAPI

def start_tcp_server(test_params):
    sock = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
    server_address = (test_params.ip,int(test_params.port))
    logger.info("Local IP address:%s, port:%s"%server_address)
    sock.bind(server_address)
    global streamID
    streamID = 0
    trafficList = []

    while True:
        try:
            sock.listen(3)
            logger.info("Starting listen on IP address:%s, port:%d"%server_address)
            client,addr = sock.accept()#will block here for TCP connection
        except socket.error as e:
            logger.info( "fail to listen on port %s"%e)
            sys.exit(1)
        except KeyboardInterrupt:
            logger.info( "\r\nQuit sigma agent tool.\r\n")
            break
        while True:
            dutCrashTag = objDut.caCheckDutCrash()
            if dutCrashTag:
                logger.info( "\r\n###############################################\r\n")
                logger.info( "\r\nQuit sigma agent tool for DUT serial not accessable.\r\n")
                logger.info( "\r\n###############################################\r\n")
                sys.exit(1)

            status,uccCommand = recv_basic(client)

            if status < 0:
                break
            elif uccCommand.strip() == "":
                pass
            else:
                handle_CAPI_command(client,uccCommand,trafficList)

def recv_basic(the_socket):
    buf = ""
    status = 0
    try:
        buf = the_socket.recv(1024)
    except KeyboardInterrupt:
        logger.info( "Quit sigma agent tool.")
        the_socket.close()
        status= -1
        return status,""
    except socket.error as e:
        logger.info( "Socket disconnected.")
        the_socket.close()
        status= -1
        return status,""

    if buf.decode('utf-8').strip() != "":
        logger.info( "---> %s"%buf.decode('utf-8'))
        return status,buf.decode('utf-8')
    else:
        logger.info( "---> %s"%buf.decode('utf-8'))
        status = -1
        return status,buf.decode('utf-8')

def send_basic(the_socket,dutStatus,parameters):
    if(re.search("RUNNING",dutStatus,re.I)):
        logger.info("<--- status,RUNNING")
        the_socket.send("status,RUNNING".encode())
    elif(re.search("INVALID",dutStatus,re.I)):
        logger.info("<--- status,INVALID")
        the_socket.send("status,INVALID".encode())
    elif(re.search("COMPLETE",dutStatus,re.I)):
        if parameters:
            logger.info("<--- %s,%s" %(dutStatus,parameters))
            the_socket.send((dutStatus + "," + parameters).encode())
        else:
            logger.info("<--- %s" %dutStatus)
            the_socket.send(dutStatus.encode())
    elif(re.search("ERROR",dutStatus,re.I)):
        logger.info("<--- status,ERROR")
        the_socket.send("status,ERROR".encode())


def execute_CAPI_command(uccCommand,trafficList):
    dutTestResult = None
    global streamID
    if(re.match('ca_get_version',uccCommand,re.I)):
        dutTestResult = objDut.caGetVersion()
    elif(re.match('device_get_info',uccCommand,re.I)):
        dutTestResult = objDut.device_get_info()
    elif(re.match('device_list_interfaces',uccCommand,re.I)):
        dutTestResult = objDut.device_list_interfaces()
    elif(re.match('sta_preset_testparameters',uccCommand,re.I)):
        dutTestResult = objDut.sta_preset_testparameters(uccCommand)
    elif(re.match('sta_get_info',uccCommand,re.I)):
        dutTestResult = objDut.sta_get_info()
    elif(re.match('sta_set_psk',uccCommand,re.I)):
        dutTestResult = objDut.sta_set_psk(uccCommand)
    elif(re.match('sta_set_encryption',uccCommand,re.I)):
        dutTestResult = objDut.sta_set_encryption(uccCommand)
    elif(re.match('sta_set_ip_config',uccCommand,re.I)):
        dutTestResult = objDut.sta_set_ip_config(uccCommand)
    elif(re.match('sta_associate',uccCommand,re.I)):
        dutTestResult = objDut.sta_associate(uccCommand)
    elif(re.match('sta_is_connected',uccCommand,re.I)):
        dutTestResult = objDut.sta_is_connected()
    elif(re.match('sta_get_bssid',uccCommand,re.I)):
        dutTestResult = objDut.sta_get_bssid()
    elif(re.match('sta_get_ip_config',uccCommand,re.I)):
        dutTestResult = objDut.sta_get_ip_config(uccCommand)
    elif(re.match('traffic_send_ping',uccCommand,re.I)):
        streamID = streamID + 1
        objDut.traffic_send_ping(uccCommand)
        dutTestResult = "streamID,%d" %streamID
    elif(re.match('traffic_stop_ping',uccCommand,re.I)):
        dutTestResult = objDut.traffic_stop_ping(uccCommand)
    elif(re.match('traffic_agent_reset',uccCommand,re.I)):
        dutTestResult = objDut.traffic_agent_reset()
        trafficList   = []
    elif(re.match('traffic_agent_config',uccCommand,re.I)):
        dutCommand,port,sleepTime = objDut.traffic_agent_config(uccCommand)
        streamID = streamID + 1
        dictory = {'id':streamID,'cli':dutCommand,'port':port,'time':sleepTime}
        trafficList.append(dictory)
        dutTestResult = "streamID,%d" %streamID
    elif(re.match('traffic_agent_send',uccCommand,re.I)):
        dutTestResult = objDut.traffic_agent_send(uccCommand,trafficList)
    elif(re.match('traffic_agent_receive_start',uccCommand,re.I)):
        dutTestResult = objDut.traffic_agent_receive_start(uccCommand,trafficList)
        time.sleep(5)
        t1 = None
        return ("COMPLETE",dutTestResult,t1)
    elif(re.match('traffic_agent_receive_stop',uccCommand,re.I)):
        dutTestResult = objDut.traffic_agent_receive_stop(uccCommand,trafficList)
        trafficList   = []
    elif(re.match('sta_set_11n',uccCommand,re.I)):
        dutTestResult = objDut.sta_set_11n(uccCommand)
    elif(re.match('sta_disconnect',uccCommand,re.I)):
        dutTestResult = objDut.sta_disconnect(uccCommand)
    elif(re.match('sta_set_pwrsave',uccCommand,re.I)):
        dutTestResult = objDut.sta_setPwrsave(uccCommand)
    elif(re.match('sta_reset_default',uccCommand,re.I)):
        dutTestResult = objDut.sta_reset_default()
    elif(re.match('ap_reset_default',uccCommand,re.I)):
        dutTestResult = objDut.ap_reset_default()
    elif(re.match('ap_set_wireless',uccCommand,re.I)):
        dutTestResult = objDut.ap_set_wireless(uccCommand)
    elif(re.match('ap_set_security',uccCommand,re.I)):
        dutTestResult = objDut.ap_set_security(uccCommand)
    elif(re.match('ap_config_commit',uccCommand,re.I)):
        dutTestResult = objDut.ap_config_commit(uccCommand)
    elif(re.match('ap_get_mac_address',uccCommand,re.I)):
        dutTestResult = objDut.ap_get_mac_address(uccCommand)
    elif(re.match('sta_get_mac_address',uccCommand,re.I)):
        dutTestResult = objDut.sta_get_mac_address()
    elif(re.match('sta_scan',uccCommand,re.I)):
        dutTestResult = objDut.sta_scan()
    elif(re.match('sta_set_security',uccCommand,re.I)):
        dutTestResult = objDut.sta_set_security(uccCommand)
    elif(re.match('sta_reassoc',uccCommand,re.I)):
        dutTestResult = objDut.sta_reassoc(uccCommand)
    return ("COMPLETE",dutTestResult,None)

def process_CAPI_command(the_socket,uccCommand,trafficList):
    logger.info("<--- status,RUNNING")
    the_socket.send("status,RUNNING\r\n".encode())

    (dutStatus,parameters,threadHD) = execute_CAPI_command(uccCommand,trafficList)
    if parameters:
        logger.info("<--- status,%s,%s" %(dutStatus,parameters))
        the_socket.send(("status,"+ dutStatus +"," + parameters+ "\r\n").encode())
    else:
        logger.info("<--- status,%s" %dutStatus)
        the_socket.send(("status," + dutStatus + "\r\n").encode())
    if threadHD is not None:
        threadHD.join()
        logger.debug( "Join the thread.")

def CAPI_exist(uccCommand):
    for api in CAPI:
        if(re.search(api,uccCommand,re.I)):
            return 1
    return 0

def handle_CAPI_command(the_socket,uccCommand,trafficList):
    capi_exist = CAPI_exist(uccCommand)
    if (capi_exist == 1):
        process_CAPI_command(the_socket,uccCommand,trafficList)
    else:
        logger.info("Command {} from UCC is not defined by sigma agent tool.".format(uccCommand))

start_tcp_server(test_params)

