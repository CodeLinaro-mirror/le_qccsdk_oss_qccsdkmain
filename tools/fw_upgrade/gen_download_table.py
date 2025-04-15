#!/usr/bin/python
#===============================================================================
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#===============================================================================

######################################################################################
# The primary use of this script is to invoke gen_part_table.py and gen_fwd_table.py
#
# Returns 0 on success; non-zero on failure
######################################################################################

import os
import sys
import glob
import re
import subprocess
import argparse
from xml.dom import minidom
from xml.dom import Node as domNode
import xml.etree.ElementTree as ET

class Fwd_Table:
    '''
    Parse the generated_fwd_table.xml
    '''
    SECTOR_SIZE = 4096

    def __init__(self):
        ''' '''
        self.sector_size = self.SECTOR_SIZE
        self.tables = []

    def from_xml_file (self, xml_file):
        self.__init__()

        xml = ET.parse(xml_file)
        fwd_table = xml.getroot()
        if fwd_table.tag != 'jdata':
            raise ValueError("XML didn't start with the correct tag <jdata>")

        for child in fwd_table:
            if child.tag == 'program':
                dirname = child.attrib['dirname']
            elif child.tag == "erase":
                dirname = ""

            begin = int(child.attrib['start_sector'], 0) * self.sector_size
            size = int(child.attrib['num_partition_sectors'], 0) * self.sector_size
            location = "flash"
            self.tables = self.tables + [(child.tag, dirname, child.attrib['filename'], begin, size, location)]
        return self.tables

class Config_File:
    '''
    Parse the download_config.xml to get the settings of flash layout and firmware description table
    '''
    def __init__(self):
        ''' '''
        #Application (e.g. QCLI_demo) Primary FS default size 64KB
        self.FS1_sizeKB = 64
        #Application (e.g. QCLI_demo) Secondary FS default size 64KB
        self.FS2_sizeKB = 64
        #Size of RAM Dump to store core dump in Flash memory partition ID 99.
        #Set it as 64 KB for by default.
        self.RAMDUMP_sizeKB = 64
        #Size of USER DATA region for users to store data in flash memory partition ID 101.
        #Set it as 64KB for by default.
        self.USERDATA_sizeKB = 64
        #RANK value that will be set in firmware description table.
        #0: Golden, 0xFFFFFFFF: Trial, other valus: Current.
        self.RANK = 1
        self.tables = []
        self.FS1IMG = ""
        self.FS2IMG = ""

    def find_file(self, image, file_spec, app_path):
        dirname = ""
        filename = ""
        if os.path.isfile(os.path.abspath(file_spec)):
            dirname, filename = os.path.split(os.path.abspath(file_spec))
        else:
            if image == "FERMION_SBL":
                img_name = ''.join((glob.glob(os.path.abspath(os.path.join(app_path, "../../../", image, "DEBUG/bin", file_spec)))))
                if img_name and os.path.isfile(img_name):
                    dirname, filename = os.path.split(img_name)
        return dirname, filename

    def from_xml_file(self, xml_file, app_path, is_all=False):
        '''
        Example of config file:
        <fw_upgrade_config>
        <config FS1_sizeKB="64" FS2_sizeKB="64" RAMDUMP_sizeKB="64" USERDATA_sizeKB="64" RANK="1">
        <flash image="FDT" file="../../scripts/storage/firmware_desc_table/curr_age_fdt/frn_curr_age_default.bin" begin="0x208000" location="rram">
        <flash image="FERMION_SBL" file="FERMION_SBL_HASHED.elf " begin="0x20a400" location="rram">
        <flash image="FERMION_SBL" file="FERMION_SBL_HASHED.elf " begin="0x212400" location="rram">
        </fw_upgrade_config>
        '''
        xml = ET.parse(xml_file)
        config_table = xml.getroot()
        if config_table.tag != 'fw_upgrade_config':
            raise ValueError("XML didn't start with the correct tag <fw_upgrade_config>")

        for child in config_table:
            if child.tag == 'config':
                self.FS1_sizeKB = int(child.attrib['FS1_sizeKB'], 0)
                self.FS2_sizeKB = int(child.attrib['FS2_sizeKB'], 0)
                self.RAMDUMP_sizeKB = int(child.attrib['RAMDUMP_sizeKB'], 0)
                self.USERDATA_sizeKB = int(child.attrib['USERDATA_sizeKB'], 0)
                self.RANK = int(child.attrib['RANK'], 0)
                self.FS1IMG = child.attrib['FS1IMG']
                self.FS2IMG = child.attrib['FS2IMG']
            elif child.tag == 'flash':
                if is_all:
                    dirname, filename = self.find_file(child.attrib['image'], child.attrib['file'], app_path)
                    if filename == "":
                        raise ValueError("Could not find file")

                    self.tables = self.tables + [("program", dirname, filename, int(child.attrib['begin'], 0), "0", child.attrib['location'])]
                else:
                    if child.attrib['image'] == 'FDT':
                        dirname, filename = self.find_file(child.attrib['image'], child.attrib['file'], app_path)
                        if filename == "":
                            raise ValueError("Could not find file")

                        self.tables = self.tables + [("program", dirname, filename, int(child.attrib['begin'], 0), "0", child.attrib['location'])]

class Download_Table:
    '''
    Generate the generated_download_table.xml and parse it.
    '''
    def __init__(self):
        self.tables = []

    def from_xml_file(self, xml_file):
        xml = ET.parse(xml_file)
        download_table = xml.getroot()
        if download_table.tag != 'download_table':
            raise ValueError("XML didn't start with the correct tag <download_table>")

        for child in download_table:
            if child.tag == "program":
                file = os.path.join(child.attrib['dirname'], child.attrib['filename'])
            else:
                file = ""
            begin = int(child.attrib['begin'], 0)
            length = int(child.attrib['size'], 0)
            location = child.attrib['location']
            self.tables = self.tables + [(file, begin, length, location)]
        return self.tables

    def to_xml(self, xmlfile, tables):
        def add_basic_xml_attributes (xml_elem, operation, dirname, filename, begin, size, location):
            element = ET.SubElement(xml_elem, operation)
            if dirname != "":
                element.set ('dirname', dirname)
            element.set ('filename', filename)
            element.set ('begin', str(int(begin)))
            element.set ('size', str(int(size)))
            element.set ('location', location)
            return element

        xml_str = ET.Element('download_table')
        xml_str.append(ET.Comment('This is an autogenerated file'))

        for table in tables:
            operation, dirname, filename, begin, size, location = table
            add_basic_xml_attributes(xml_str, operation, dirname, filename, begin, size, location)

        xml_unformat = ET.tostring(xml_str, 'utf-8')
        reparse = minidom.parseString(xml_unformat)
        reparse.toprettyxml(indent="   ")

        try:
            with open(xmlfile , 'wb') as f:
                f.seek(0)
                f.write((reparse.toprettyxml(indent="   ")).encode('utf-8'))
                f.close()
        except IOError as e:
            raise AssertionError("Can't open file %s" % (xmlfile))

# path_to_app interpret an app specification which was specified on the
# command line using "--app xyz".
#
# Interpretation of an app_spec employs these heuristics which are derived
# from SDK build rules.
# Case1) If app_spec is a file, that is the app. (Most precise specification.)
# Case2) If app_spec is a directory and if there is a single file, *_HASHED.elf,
#        in that directory, then that is the app.
# Case3) If app_spec is a directory and if there is a single file, *.elf,
#        in that directory, then that is the app.
# Case4) If app_spec is a directory and if there is a directory under that
#        which contains an FERMION_IOE_QCLI_DEMO directory which contains a single file,
#        *_HASHED.elf,
#        then that is the app.
# If none of the above hold, then print an error message and EXIT qflash.py.
#
def path_to_app(app_spec):
    app_spec = os.path.abspath(app_spec)
    if os.path.isfile(app_spec):
        return app_spec # Case1

    if os.path.isdir(app_spec):
        img_name = ''.join((glob.glob(os.path.join(app_spec, "*_HASHED.elf"))));
        if img_name and os.path.isfile(img_name):
            return img_name # Case2

        img_name = ''.join((glob.glob(os.path.join(app_spec, "*.elf"))));
        if img_name and os.path.isfile(img_name):
            return img_name # Case3

        temp_dir = ''.join((glob.glob(os.path.join(app_spec, "FERMION_IOE_QCLI_DEMO"))));
        if (os.path.isdir(temp_dir)):
            img_name = ''.join((glob.glob(os.path.join(temp_dir, "/DEBUG/bin/*_HASHED.elf"))));
            if img_name and os.path.isfile(img_name):
                return img_name # Case4

    # Error case -- cannot decide what app to program
    FAIL("Cannot determine application for " + str(app_spec) + "\nPlease check --app.")

def main():
    tool_description = \
r"""
Generate download table for nvm_programmer to program.
Step 1: parse the config file download_config.xml. Users can change "config" of this file about
        the upgraded memory loayout, for exmaple the File system size, RAM DUMP region size, USER
        DATA REGION size... Currently upgraded memory only support flash.
        From this config file, it can also parse the potentional download files as described in
        "flash". Users can change the file path or delete a "flash" item if the specific image will
        not be programmed any more.
Step 2: invoke gen_part_table.py to generate partition table as generated_partition_tbale.xml.
Step 3: invoke gen_fwd_table.py to generate partition table binary and generated_fwd_table.xml.
Step 4: generate generated_download_table.xml, this file can be used by nvm_programmer.py to program
        images and erase memory.

Example Usage:
        If want to generate a download table with FDT/SBL/APP images to program:
        python gen_download_table.py --app ..\..\output\qcc730v2_evb11_hostless\FERMION_IOE_QCLI_DEMO\DEBUG\bin\FERMION_IOE_QCLI_DEMO_HASHED.elf -c download_config.xml -A
        If want to generate a download table with APP image to program, FDT and SBL images are programed before with no change
        python gen_download_table.py --app ..\..\output\qcc730v2_evb11_hostless\FERMION_IOE_QCLI_DEMO\DEBUG\bin\FERMION_IOE_QCLI_DEMO_HASHED.elf -c download_config.xml
"""
    def INFO(strarg):
        print(" Info: " + strarg)

    def DEBUG(strarg):
        global want_debug
        if want_debug:
            INFO(strarg)

    def FAIL(strarg):
        print(" Error: " + strarg)
        sys.exit(1)

    def validate_file(path):
        if not os.path.isfile(path):
            FAIL ("Missing file: " + path)

    # Parse command-line options
    parser = argparse.ArgumentParser(description=tool_description, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--app', type=str, required=True, action='append', help='Specify the path and/or name of an Application Image to run')
    parser.add_argument('-c', '--config', type=str, required=True, help='The config xml file')
    parser.add_argument('-o' ,'--output', type=str, required=False, help='output download raw program xml file')
    parser.add_argument('-A' ,'--all', default=False, action='store_true', help='download  images listed in download_config.xml together with Application iamge')
    parser.add_argument('--debug', '--verbose', required=False, help='Enable debug messages', default=False, action='store_true')

    args = parser.parse_args()

    # Do this first thing so DEBUG works properly
    global want_debug
    want_debug=args.debug

    # Flag that tells whether or not this is a FTM (mfg) ImageSet
    FTM_image_set = False

    if re.search("win32", sys.platform):
        using_windows = True
        need_shell = False # Avoid "line too long" bug in Windows when using double-quoted args
    else:
        using_windows = False
        need_shell = True

    # Create a Partition Table
    validate_file(os.path.join(os.getcwd(), "gen_part_table.py"))

    # Set APP_img_list to a list of all APP applications
    # --app was specified at least once
    APP_img_list = []
    for app in args.app:
        APP_img_list.append(app)

    # At this point, APP_img_list contains a list of one or more APP applications
    # to be programmed to flash.
    if not APP_img_list:
        FAIL("Please check the value of --app.")
    DEBUG("Use APP image(s): " + str(APP_img_list))

    INFO("Generate partition table...")
    APP_part = ""
    for APP_img_name in APP_img_list:
        DEBUG("APP_img_name is " + APP_img_name)
        validate_file(APP_img_name)
        APP_part = APP_part + " --partition " + " --id=APP "
        APP_part = APP_part + " --file=" + '"' + APP_img_name + '" '
        APP_name = os.path.basename(APP_img_name)
        DEBUG("APP_name is " + APP_name)
        if APP_name == "FERMION_FTM.elf":
            FTM_image_set = True

    config_file_desc = Config_File()

    config_file_desc.from_xml_file(args.config, os.path.dirname(os.path.abspath(APP_img_name)), args.all)
    tables = config_file_desc.tables
    FS1IMG = config_file_desc.FS1IMG
    FS2IMG = config_file_desc.FS2IMG
    # Primary Filesystem size
    FS1_sizeKB = config_file_desc.FS1_sizeKB
    # Secondary Filesystem size?
    FS2_sizeKB = config_file_desc.FS2_sizeKB
    RAMDUMP_sizeKB = config_file_desc.RAMDUMP_sizeKB
    USERDATA_sizeKB = config_file_desc.USERDATA_sizeKB

    # If FS2IMG environment variable is not set, default to DEFAULT
    if str(FS2IMG) == "None":
        FS2IMG = "DEFAULT"

    if FS2_sizeKB == 0:
        FS2IMG = "NONE"

    if FTM_image_set:
        #
        # Special handling for FTM (manufacturing) firmware.
        #
        FS_floating_part=" --partition --id=FS1 --size=" + str(FS1_sizeKB) + "KB"

    else:
        #
        # Normal application; not FTM/manufacturing firmware.
        # By default, use a fixed-position FS1 and FS2. FS1 is the automounted file system
        # used during normal operation whereas FS2's purpose is to support concurrent
        # FW Upgrade (so the Upgrade engine copies files from FS1 to FS2).
        #
        # If a secondary file system is not desired, set FS2IMG=NONE.
        #
        FS_floating_part = ""
        FS_fixed_part = ""
        if FS1_sizeKB != 0:
            FS_fixed_part= FS_fixed_part + " --partition --id=FS1 --start=12KB --size=" + str(FS1_sizeKB) + "KB"

        if FS1IMG:
            FS_fixed_part = FS_fixed_part + ' --file="' + FS1IMG + '"'

        if FS2IMG and (FS2IMG.upper() == "NONE"):
            DEBUG("FS2IMG IS NONE")
            FS2_sizeKB = 0
        else:
            FS_fixed_part = FS_fixed_part + " --partition --id=FS2 --start=" + str(FS1_sizeKB+12) + "KB --size=" + str(FS2_sizeKB) + "KB"
            if FS2IMG and (FS2IMG.upper() != "DEFAULT"):
                DEBUG("FS2IMG IS " + str(FS2IMG))
                FS_fixed_part = FS_fixed_part + ' --file="' + FS2IMG + '"'

    if RAMDUMP_sizeKB != 0:
        RAMDUMP_part=" --partition --id=99 --size=" + str(RAMDUMP_sizeKB) + "KB" + " --start=" + str(12 + FS1_sizeKB + FS2_sizeKB) + "KB"
    else:
        RAMDUMP_part=""

    if USERDATA_sizeKB != 0:
        USERDATA_part=" --partition --id=101 --size=" + str(USERDATA_sizeKB) + "KB" + " --start=" + str(12 + FS1_sizeKB + FS2_sizeKB + RAMDUMP_sizeKB) + "KB"
    else:
        USERDATA_part=""

    begin_offset=" --begin=" + str(12 + FS1_sizeKB + FS2_sizeKB + RAMDUMP_sizeKB + USERDATA_sizeKB) + "KB"
    rank_set=" --rank="+str(config_file_desc.RANK)

    # Remove old version of partition_table, if any
    try:
        os.remove(os.path.join(os.getcwd(), "generated_partition_table.xml"))
    except:
        pass

    # Generate a new partition table
    # Always includes APP firmware.
    cmd_string = "python " + '"' + os.path.join(os.getcwd(), "gen_part_table.py") + '"' +\
        " --output=" + '"' + os.path.join(os.getcwd(), "generated_partition_table.xml") + '"' +\
        begin_offset +\
        rank_set +\
        APP_part +\
        FS_floating_part +\
        FS_fixed_part +\
        " --partition --id=UNUSED --size=8KB --start=4KB" +\
        RAMDUMP_part +\
        USERDATA_part

    DEBUG("Execute: " + cmd_string);
    try:
        subprocess.check_output(cmd_string, shell=need_shell)
    except:
        FAIL("gen_part_table.py failed")

    validate_file(os.path.join(os.getcwd(), "generated_partition_table.xml"))

    validate_file(os.path.join(os.getcwd(), "gen_fwd_table.py"))
    INFO("Generate FWD table...")
    # Create a FWD Table
    # Remove old version of fwd_table, if any
    try:
        os.remove(os.path.join(os.getcwd(), "generated_fwd_table.xml"))
    except:
        pass

    cmd_string = "python " + '"' + os.path.join(os.getcwd(), "gen_fwd_table.py") + '"' +\
        " -x " + '"' + os.path.join(os.getcwd(), "generated_partition_table.xml") + '"' +\
        " --rawprogram " + '"' + os.path.join(os.getcwd(), "generated_fwd_table.xml") + '"' +\
        " --fdtbin " + '"' + os.path.join(os.getcwd(), "firmware_table.bin") + '"'

    DEBUG("Execute: " + cmd_string);
    try:
        subprocess.check_output(cmd_string, shell=need_shell)
    except:
        FAIL("gen_fwd_table.py failed")

    validate_file(os.path.join(os.getcwd(), "generated_fwd_table.xml"))

    INFO("Generate download table...")
    # Create a Download Table
    # Remove old version of download_table, if any
    try:
        os.remove(os.path.join(os.getcwd(), "generated_download_table.xml"))
    except:
        pass

    fwd_table_desc = Fwd_Table()
    fwd_table_desc.from_xml_file(os.path.join(os.getcwd(), "generated_fwd_table.xml"))
    tables = tables + fwd_table_desc.tables
    download_table_desc = Download_Table()
    if args.output:
        output = args.output
    else:
        output = os.path.join(os.getcwd(), "generated_download_table.xml")
    download_table_desc.to_xml(output, tables)

    validate_file(output)
    INFO("Done generating download table")

if __name__ == "__main__":
    main()
