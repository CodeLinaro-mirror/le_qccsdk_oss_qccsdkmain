#===============================================================================
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#===============================================================================

# Script to take register dump using J-Link from the generated register definitions from Qualcomm IP Catalog

# Usage:
# python RegisterDumpJLink.py <register.txt> <J-Link USB serial number>

# To generate register.txt:
# https://ipcatalog.qualcomm.com/swi/export
# Printf Register/Module List
# Data Set -> Registers
# Format String -> %s %x
# Format Parameter -> name, address
# Parse the register_hwiosave.xml file in https://ipcatalog.qualcomm.com/swi/dump/parser

import pylink
import xml.etree.ElementTree as ET
import time
import sys

register_address = []
register_name = []
register_value = []


def generate_register_list(filename):
    fp = open(filename, "r")

    for line in fp:
        line_split = line.split()
        register_name.append(line_split[0])
        register_address.append(int('0x' + line_split[1], 16))
    # print(register_address)
    # print(register_name)


def generate_xml(filename):
    hwiodump = ET.Element("hwioDump")
    hwiodump.set('version', '1')
    timestamp = ET.SubElement(hwiodump, "timestamp")
    timestamp.text = time.strftime('%d. %b %Y %H:%M:%S')
    generator = ET.SubElement(hwiodump, "generator")
    generator.text = "J-Link"
    chip = ET.SubElement(hwiodump, "chip")
    chip.set('name', 'fermion_1.0')
    chip.set('map', 'ARM_ADDRESS_FILE_SW')
    chip.set('version', 'fermion_p3q2_e2_13') # Requires update based on IPCAT

    i = 0
    for data in register_address:
        register = ET.SubElement(chip, "register")
        register.set("address", hex(register_address[i]))
        register.set("value", hex(register_value[i]))
        register.set("name", register_name[i])
        i = i + 1
    tree = ET.ElementTree(hwiodump)
    ET.indent(tree, space='  ', level=0)
    # print(ET.dump(hwiodump))
    # print(ET.dump(chip))

    with open(filename, 'wb') as f:
        tree.write(f, encoding='utf-8', xml_declaration=True)
        print("{0} saved".format(filename))
        # formatter = xmlformatter.Formatter()
        # formatter.format_file(filename)


def jlink_register_read(serial):
    jlink = ConfigTargetSettings(serial)
    InitTarget(jlink)
    jlink.connect('Cortex-M4', speed=15000)
    print("core id: 0x%x, device family: %d, ir_len: %d" % (jlink.core_id(),jlink.device_family(), jlink.ir_len()))
    print("core name: %s, jtag speed: %d" % (jlink.core_name(), jlink.speed))
    # time.sleep(5)
    if jlink.target_connected():
        print("J-Link connected. Reading registers...")
        for register in register_address:
            value = jlink.memory_read32(register, 1)[0]
            register_value.append(value)

        jlink.close()
        print("Register read complete.")
    else:
        print("Failed to connect J-Link")
        jlink.close()
        exit()

def jtag_send_tms_tdi(jlink, tms, tdi, numbits):
    while (numbits > 0):
        tms_b = tms&0x1
        tdi_b = tdi&0x1
        if (tms_b):
            jlink.set_tms_pin_high()
        else:
            jlink.set_tms_pin_low()
        if (tdi_b):
            jlink.set_tdi_pin_high()
        else:
            jlink.set_tdi_pin_low()
        jlink.set_tck_pin_low()
        jlink.set_tck_pin_high()

        numbits = numbits - 1
        tms = tms >> 1
        tdi = tdi >> 1

def qtap_exec(jlink, qtap_instr):
    orig_speed = jlink.speed
    jlink.set_speed(100)

    jlink.set_trst_pin_low()
    jlink.set_tck_pin_low()
    jlink.set_tck_pin_high()
    jlink.set_tck_pin_low()
    jlink.set_tck_pin_high()

    jlink.set_trst_pin_high()
    jlink.set_tck_pin_low()
    jlink.set_tck_pin_high()
    jlink.set_tck_pin_low()
    jlink.set_tck_pin_high()

    #jlink.jtag_send(0x1f, 0, 16)
    jtag_send_tms_tdi(jlink, 0x1f, 0, 16)

    tdi_val = qtap_instr << 4;
    #jlink.jtag_send(0x0c03, tdi_val, 13)
    jtag_send_tms_tdi(jlink, 0x0c03, tdi_val, 13)

    if (qtap_instr == 0xb or qtap_instr == 0x10 or qtap_instr == 0x11):
        #jlink.jtag_send(0x1f, 0, 16)
        jtag_send_tms_tdi(jlink, 0x1f, 0, 16)

    jlink.set_tms_pin_low()
    jlink.set_tdi_pin_low()
    jlink.set_speed(orig_speed)

def ConfigTargetSettings(serial):
    jlink = pylink.JLink()
    jlink.open(serial)
    jlink.set_tif(0) # 0 for jtag; 1 for swd
    return jlink

def InitTarget(jlink):
    jlink.exec_command("CORESIGHT_AddAP = Index=0 Type=AHB-AP BaseAddr=0x40000")
    jlink.exec_command("CORESIGHT_SetIndexAHBAPToUse = 0")

    qtap_exec(jlink, 0x0B);  # 7-bit QTAP instr to connect to the M4 TAP

if __name__ == '__main__':
    input_filename = sys.argv[1]
    xml_file = input_filename.rstrip('.txt') + '_hwiosave.xml'
    generate_register_list(input_filename)
    jlink_register_read(sys.argv[2])
    generate_xml(xml_file)


