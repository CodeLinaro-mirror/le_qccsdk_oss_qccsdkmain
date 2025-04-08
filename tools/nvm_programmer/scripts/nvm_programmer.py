#===============================================================================
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#===============================================================================
from __future__ import print_function
import sys
sys.path.append('.')
from gdb_framework import hexfile
import binascii
import os
import time
import json
import math
import yaml

PACK_ENABLE = os.getenv('PACK_ENABLE', 'False') == 'True'
from gdb_framework.gdb_framework import GDB_Framework

import socket
from socket import SOCK_DGRAM


if PACK_ENABLE:
    from fw_upgrade.gen_download_table import Download_Table
else:
    FW_UPGRADE_SCRIPTS_PATH = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "../../fw_upgrade"))
    sys.path.append(FW_UPGRADE_SCRIPTS_PATH)
    from gen_download_table import Download_Table

import re
import subprocess

class ACCESS:
    READ_ONLY = 1
    WRITE_ONLY = 2
    READ_WRITE = 3

class OTP_Field(object):
  def __init__(self, name, address, offset, length, access):
      self.__name = name
      self.__address = address
      self.__offset = offset
      self.__length = length
      self.__access = access

  @property
  def name(self):
      return self.__name

  @property
  def address(self):
      return self.__address

  @property
  def offset(self):
      return self.__offset

  @property
  def length(self):
      return self.__length

  @property
  def access(self):
      return self.__access

  def set_access(self, value):
      self.__access = value

access_mapping = {
    'READ_ONLY': ACCESS.READ_ONLY,
    'WRITE_ONLY': ACCESS.WRITE_ONLY,
    'READ_WRITE': ACCESS.READ_WRITE
}


class VerifyError(Exception):
    pass

class NVM_Programmer(GDB_Framework):
    ERROR_MAP = {
        0:"Success",
        1:"Failed",
        100:"Programm error",
        101:"Partial erase error",
        102:"Chip erase error",
        103:"Invalid address",
        104:"Device attach error",
        105:"Read error",
        141:"Not implemented"
    }
    #commands for jtag programmer
    JTAG_COMMAND_PROGRAM              = 0x00000001
    JTAG_COMMAND_CHIP_ERASE           = 0x00000005
    JTAG_COMMAND_COMPUTE_CRC          = 0x00000041
    JTAG_COMMAND_READ                 = 0x00000043
    JTAG_COMMAND_PARTIAL_ERASE	      = 0x00000045
    JTAG_COMMAND_SYSTEM_RESET         = 0x00000047
    JTAG_COMMAND_OTP_WRITE            = 0x00000051
    JTAG_COMMAND_OTP_READ             = 0x00000052

    #commads parameters offset of JTAG_PARAM
    JTAG_PARAM_OFFSET			= 0x0C
    JTAG_PARAM_ADDRESS  		= 0x10
    JTAG_PARAM_SIZE				= 0x14
    JTAG_PARAM_COMMAND 			= 0x1C
    JTAG_PARAM_BUFFER_OFFSET	= 0x20
    JTAG_PARAM_READ_BUFFER_OFFSET	= 0x4020
    JTAG_PARAM_CRC_RESULT		= 0x18
    JTAG_PARAM_STATUS			= 0x1C

    #NVM type name
    NVM_TYPE_NAME_RRAM			= (0)
    NVM_TYPE_NAME_FLASH 		= (1)
    NVM_TYPE_NAME_OTP                   = (2)

    #erase timeout
    ERASE_TIMEOUT = 300

    if PACK_ENABLE:
        DEFAULT_RAM_IMAGE = os.path.join(sys._MEIPASS, 'FERMION_NVM_PROGRAMMER.elf')
    else:
        DEFAULT_RAM_IMAGE = '../bin/FERMION_NVM_PROGRAMMER.elf'

    #READ_WRITE_PERMISSIONS
    READ_WRITE_PERMISSIONS_OFFSET_0 = 0x30
    READ_WRITE_PERMISSIONS_OFFSET_1 = 0x34

    PBL_VERSION_ADDR = 0x200168
    READ_WRITE_PERMISSIONS_REGION = ['READ_PERMISSION_HW_ENCRYPTION_KEY', 'WRITE_PERMISSION_READ_WRITE_PERMISIONS', 'WRITE_PERMISSION_HW_ENCRYPTION_KEY', 'WRITE_PERMISSION_PK_HASH', 'WRITE_PERMISSION_OEM_SECURE_BOOT', 'WRITE_PERMISSION_ANTI_ROLL_BACK', 'WRITE_PERMISSION_FIRMWARE', 'WRITE_PERMISSION_NPS_CONFIG' ]

    OEM_SECURE_BOOT_REGION = ['TOTAL_ROT_NUM', 'MODEL_ID', 'SECURE_BOOT_ENFORCE', 'OEM_ID', 'OEM_DEBUG_DISABLE', 'DISABLE_QC_RMA', 'ROT_INDEX']

    def __init__(self):
        '''
        Initializes the GDB tool.
        '''
        # *** Tool description ***
        self.ToolDescription = 'NVM programming tool'

        self.fields = []

        super(NVM_Programmer, self).__init__()

        # *** Tool specific initialization ***
        pass

    def add_arguments(self):
        '''
        Add tool specific arguments.
        '''
        # *** Tool specific arguments ***
        self.argparser.add_argument('-f', '--file', help='File to program or store the data read from target.')
        self.argparser.add_argument('-i', '--ram-image', default=NVM_Programmer.DEFAULT_RAM_IMAGE, help='Location of the tool application RAM image.')
        self.argparser.add_argument('-b', '--begin-address', type=lambda x: int(x,0), default=None, metavar='ADDRESS', help='Indicates the file program address, partial erase or read address.')
        self.argparser.add_argument('-n', '--nvm-name', default='rram', choices=['rram', 'flash', 'otp'], help='Non-Volatile Memory type name.')
        self.argparser.add_argument('-E', '--chip-erase', default=False, action='store_true', help='Chip erase the flash.')
        self.argparser.add_argument('-e', '--partial-erase', default=False, action='store_true', help='Partial erase the NVM(rram or flash).')
        self.argparser.add_argument('-d', '--read', default=False, action='store_true', help='Read data from the NVM(rram or flash).')
        self.argparser.add_argument('-S', '--size', type=lambda x: int(x,0), default=None, help='Indicates partial erase or read size.')
        self.argparser.add_argument('-t', '--table', help='xml file of download table to download images to memory and earse memory')
        self.argparser.add_argument('-P', '--partition', default=False, action='store_true', help='program file to flash together with partition table and program fdt to rram')
        self.argparser.add_argument('-A', '--all', default=False, action='store_true', help='program fdt, sbl, partition table and app all in once, can only be used with -P')
        self.argparser.add_argument('--reset', default=False, action='store_true', help='Reset target after program, erase or read complete.')

        self.argparser.add_argument('-k', '--key-cfg', help='One key-value pair of OTP region to program')
        self.argparser.add_argument('-g', '--get-key', help='The key of OTP region to read')
        self.argparser.add_argument('--config', help='The yaml file which may contain OTP key-value pairs')
        self.argparser.add_argument('--otp_field_cfg', default='otp_field_list.yaml', help='OTP field configuration file')

    def load_fields(self, file_name):
        with open(file_name, 'r') as f:
            data = yaml.safe_load(f)

        for entry in data:
            name = entry['name']
            address = int(entry['address'], 0)
            offset = int(entry['offset'], 0)
            length = int(entry['length'], 0)
            access = access_mapping[entry['access']]
            self.fields.append(OTP_Field(name, address, offset, length, access))
    def get_field(self, field_name) -> OTP_Field:
        for field in self.fields:
            if field.name == field_name:
                return field
        print('{} Not in otp field list!!!'.format(field_name))
        return None

    def check_pbl_version(self):
        device_type_name_buf = self.get_symbol_info('Device_Type')['address']
        self.write_int(device_type_name_buf, NVM_Programmer.NVM_TYPE_NAME_RRAM)
        
        data_buf = self.get_symbol_info('JTAG_Param')
        data_buf_addr = data_buf['address'] + NVM_Programmer.JTAG_PARAM_BUFFER_OFFSET
        self.write_int(self.param_buf + NVM_Programmer.JTAG_PARAM_ADDRESS, NVM_Programmer.PBL_VERSION_ADDR)
        self.write_int(self.param_buf + NVM_Programmer.JTAG_PARAM_SIZE, 4)
        self.write_int(self.param_buf + NVM_Programmer.JTAG_PARAM_COMMAND, NVM_Programmer.JTAG_COMMAND_READ)
        self.gdb_execute('c')
        res = self.read_int(data_buf_addr)
        #print('PBL Ver is {}.'.format(hex(res)))

        self.write_int(device_type_name_buf, NVM_Programmer.NVM_TYPE_NAME_OTP)
        if res == 0x10020001:
            return True
        else:
            return False

    def reverse_bytes(self, hex_str):
        #print('str: {}'.format(hex_str.upper()))
        # Ensure the input is a string
        hex_str = str(hex_str)
        # Remove the '0x' prefix if it exists
        if hex_str.startswith('0x'):
            hex_str = hex_str[2:]
        # Split the string into chunks of 8 characters (32 bits)
        chunks = [hex_str[i:i+8] for i in range(0, len(hex_str), 8)]
        return ''.join(''.join(reversed([chunk[i:i+2] for i in range(0, len(chunk), 2)])) for chunk in chunks)

    def write_otp_field(self, field, value):
        print('{}={}'.format(field.name, value.upper()))
        length = field.length
        offset = field.offset
        size = math.ceil(field.length/8)
        if size < 4:
            #check the value
            mask = ((1 << length) - 1)
            if int(value,16) > mask:
                print('The value of {} should not greater than {}. Please check the value again.'.format(field.name, hex(mask)))
                return
            #need to read OTP first
            res = self.read_otp_field(field, False)
            # set the value
            int_value = int.from_bytes(b''.join(res), byteorder='little')
            int_value = self.set_bits(int_value, field.offset, field.length, int(value,16))
            size = 4
            bytes_value = int_value.to_bytes(4, 'little')
        else:
            value=value[2:]
            if len(value)%2:
                print('Length of the input value is wrong, it should be even number, but now it is {}'.format(len(value)))
                return
            bytes_value=bytes.fromhex(value)
        if field.name == 'PK_HASH':
            if self.check_pbl_version() == True:
                str_value=self.reverse_bytes(value)
                bytes_value=bytes.fromhex(str_value)
        self.write_int(self.param_buf + NVM_Programmer.JTAG_PARAM_ADDRESS, field.address)
        self.write_int(self.param_buf + NVM_Programmer.JTAG_PARAM_SIZE, size)
        self.write_buf(self.param_buf + NVM_Programmer.JTAG_PARAM_BUFFER_OFFSET, bytes_value)
        self.write_int(self.param_buf + NVM_Programmer.JTAG_PARAM_COMMAND, NVM_Programmer.JTAG_COMMAND_OTP_WRITE)
        self.gdb_execute('c')
        write_result = self.read_int(self.result_buf + NVM_Programmer.JTAG_PARAM_STATUS)
        if write_result:
            print('Write OTP {} failed, error:{}'.format(field.name, NVM_Programmer.ERROR_MAP[write_result]))
        else:
            print('Write OTP {} success.'.format(field.name))

    def write_otp(self, key, value):
        #print('key is {}'.format(key))
        field = self.get_field(key)
        #check the access for write
        if field != None :
            if (field.access & ACCESS.WRITE_ONLY) != 0:
                self.write_otp_field(field, value)
                #if key in self.READ_WRITE_PERMISSIONS_REGION:
                #    print('need to call update_permission: key={}, value={}'.format(key, value))
                #    self.update_permission()
            else:
                print('ERROR: Write OTP {} is not allowed'.format(field.name))
        else:
            print('ERROR: OTP {} is not found'.format(key))
        return

    def read_otp(self, key):
        field = self.get_field(key)
        self.update_permission()
        print('********************************************************************************')
        print('Read OTP {} from command line.'.format(key))
        if field != None:
            if (field.access & ACCESS.READ_ONLY):
                self.read_otp_field(field)
            else:
                print('ERROR: Read OTP {} is not allowed'.format(field.name))
        else:
            print('ERROR: OTP {} is not found'.format(key))
        print('********************************************************************************')
        return

    def read_otp_field(self, field, flag=True):
        #field is OTP_Field
        length = field.length
        offset = field.offset
        size = math.ceil(field.length/8)
        if size < 4:
            size = 4
        data_buf = self.get_symbol_info('JTAG_Param')
        data_buf_addr = data_buf['address'] + NVM_Programmer.JTAG_PARAM_READ_BUFFER_OFFSET
        self.write_int(self.param_buf + NVM_Programmer.JTAG_PARAM_ADDRESS, field.address)
        self.write_int(self.param_buf + NVM_Programmer.JTAG_PARAM_SIZE, size)
        self.write_int(self.param_buf + NVM_Programmer.JTAG_PARAM_COMMAND, NVM_Programmer.JTAG_COMMAND_OTP_READ)
        self.gdb_execute('c')
        res = self.read_buf(data_buf_addr, size)
        if length < 48:
            mask:int = ((1 << length) - 1) << offset
            int_value = int.from_bytes(b''.join(res), byteorder='little')
            int_value &= mask
            int_value >>= offset
            res_hex_string=hex(int_value)
        else:
            res_hex_string='0x'+''.join([hex(int.from_bytes(x, byteorder='big'))[2:].zfill(2) for x in res])
        if flag == True:
            print('{}={}'.format(field.name, res_hex_string.upper()))
        return res

    def set_bits(self, i: int, offset: int, length: int, value: int) -> int:
        mask = ((1 << length) - 1) << offset
        i &= ~mask
        i |= (value << offset) & mask
        return i

    def do_config(self, cfg_file):
        device_type_name_buf = self.get_symbol_info('Device_Type')['address']
        with open(cfg_file, 'r') as f:
            cfg_data=yaml.load(f, Loader=yaml.BaseLoader)
        for nvm in cfg_data.keys():
            if nvm == 'OTP':
                self.update_permission()
                print('********************************************************************************')
                print('Write OTP from command line.')
                self.write_int(device_type_name_buf, NVM_Programmer.NVM_TYPE_NAME_OTP)
                for key in cfg_data[nvm].keys():
                    self.write_otp(key, cfg_data[nvm][key])
                print('********************************************************************************')
            if nvm == 'RRAM':
                self.write_int(device_type_name_buf, NVM_Programmer.NVM_TYPE_NAME_RRAM)
                print('********************************************************************************')
                if cfg_data['RRAM']['erase'] == False:
                    print('Do not erase.')
                for i in range(len(cfg_data['RRAM']['images'])):
                    print('Flashing image:{} at {}'.format(cfg_data['RRAM']['images'][i][0], cfg_data['RRAM']['images'][i][1]))
                    self.download(cfg_data['RRAM']['images'][i][0], True, int(cfg_data['RRAM']['images'][i][1], 16))
        pass
    def update_access(self, name, read, write):
        field = self.get_field(name)
        if field != None:
            if read == 0:
                field.set_access(field.access & ~(1<<0))
            if write == 0:
                field.set_access(field.access & ~(1<<1))
        return

    def update_permission(self):
        #to read READ_WRITE_PERMISSIONS
        #read the READ_WRITE_PERMISSIONS, then update the access to each field
        size = 8
        data_buf = self.get_symbol_info('JTAG_Param')
        data_buf_addr = data_buf['address'] + NVM_Programmer.JTAG_PARAM_READ_BUFFER_OFFSET
        self.write_int(self.param_buf + NVM_Programmer.JTAG_PARAM_ADDRESS, NVM_Programmer.READ_WRITE_PERMISSIONS_OFFSET_0)
        self.write_int(self.param_buf + NVM_Programmer.JTAG_PARAM_SIZE, size)
        self.write_int(self.param_buf + NVM_Programmer.JTAG_PARAM_COMMAND, NVM_Programmer.JTAG_COMMAND_OTP_READ)
        self.gdb_execute('c')
        res = self.read_buf(data_buf_addr, size)
        int_value = int.from_bytes(b''.join(res), byteorder='little')
        #debug
        #print("0x{:016x}".format(int_value))
        if int_value == 0:
            return

        #check the bit value and update the field.access
        for field in self.fields:
            #print('field.name={}'.format(field.name))
            if field.name == 'READ_PERMISSION_HW_ENCRYPTION_KEY':
                if (int_value >> field.offset) & 1:
                    #update the HW_ENCRYPTION_KEY read permission
                    self.update_access('HW_ENCRYPTION_KEY', 0, 1)
                continue
            if field.name == 'WRITE_PERMISSION_READ_WRITE_PERMISIONS':
                if ((int_value >> field.offset) & 1) == 1:
                    #update all READ_WRITE_PERMISSIONS fields's WRITE permission
                    for region in self.READ_WRITE_PERMISSIONS_REGION:
                        self.update_access(region, 1, 0)
                continue
            if field.name == 'WRITE_PERMISSION_HW_ENCRYPTION_KEY':
                if (int_value >> field.offset) & 1:
                    self.update_access('HW_ENCRYPTION_KEY', 1, 0)
                continue
            if field.name == 'WRITE_PERMISSION_PK_HASH':
                if (int_value >> field.offset) & 1:
                    self.update_access('PK_HASH',1,0)
                continue
            if field.name == 'WRITE_PERMISSION_OEM_SECURE_BOOT':
                if ((int_value >> field.offset) & 1) == 1:
                    #update OEM_SECURE_BOOT_REGION write permission
                    for region in self.OEM_SECURE_BOOT_REGION:
                        self.update_access(region, 1, 0)
                continue
            if field.name == 'WRITE_PERMISSION_ANTI_ROLL_BACK':
                if ((int_value>>32)>> field.offset) & 1:
                    self.update_access('M4_ANTI_ROLLBACK', 1, 0)
                continue
            if field.name == 'WRITE_PERMISSION_FIRMWARE':
                if ((int_value>>32)>> field.offset) & 1:
                    self.update_access('MAC_ADDRESSES', 1, 0)
                continue
        return
    def run(self):
        '''
        Start tool.
        '''
        if self.config['config'] != None:
            self.setup()
            self.init_ram_image(self.config['ram_image'])
            self.load_fields(self.config['otp_field_cfg'])
            self.do_config(self.config['config'])
            self.cleanup()
            return

        if self.config['get_key'] != None:
            if self.config['nvm_name'] != 'otp':
                print('\'-n otp\' should be used when read OTP')
                return
            self.setup()
            self.init_ram_image(self.config['ram_image'])
            self.load_fields(self.config['otp_field_cfg'])
            self.set_nvm_name()
            key=self.config['get_key']
            #print('key is {}'.format(key))
            self.read_otp(key)

            self.write_int(self.param_buf + NVM_Programmer.JTAG_PARAM_COMMAND, NVM_Programmer.JTAG_COMMAND_SYSTEM_RESET)
            self.gdb_execute('c')

            self.cleanup()
            return

        if self.config['key_cfg'] != None:
            if self.config['nvm_name'] != 'otp':
                print('\'-n otp\' should be used when write OTP')
                return
            self.setup()
            self.init_ram_image(self.config['ram_image'])
            self.load_fields(self.config['otp_field_cfg'])
            self.set_nvm_name()
            kv=self.config['key_cfg']
            key, value = kv.split('=', 2)
            field = self.get_field(key)
            if field != None:
                self.update_permission()
                print('********************************************************************************')
                print('Write OTP from command line.')
                self.write_otp(key, value)
                print('********************************************************************************')
            self.cleanup()
            return
        # check the address of updated programming image
        if self.config['partial_erase'] ==True and (self.config['size'] == None or self.config['begin_address'] == None):
            self.argparser.print_help()
            sys.exit('-S and -b must be specified if -e is used.')

        if self.config['read'] ==True and (self.config['size'] == None or self.config['begin_address'] == None):
            self.argparser.print_help()
            sys.exit('-S, -b must be specified if -r is used.')

        if self.config['read'] == False and self.config['chip_erase'] == False and self.config['partial_erase'] == False and (self.config['file'] == None  or  self.config['begin_address'] == None) and self.config['reset'] == None:
            self.argparser.print_help()
            sys.exit('-b and -f must be specified if programming image.')

        # Perform common tool setup
        self.setup()

        try:
            # *** Tool specific code ***
            self.init_ram_image(self.config['ram_image'])

            try:
                if self.config['partial_erase']:
                    self.partial_erase()

                    if self.config['reset']:
                        self.write_int(self.param_buf + NVM_Programmer.JTAG_PARAM_COMMAND, NVM_Programmer.JTAG_COMMAND_SYSTEM_RESET)
                        self.gdb_execute('c')

                elif self.config['chip_erase']:
                    self.chip_erase()

                    if self.config['reset']:
                        self.write_int(self.param_buf + NVM_Programmer.JTAG_PARAM_COMMAND, NVM_Programmer.JTAG_COMMAND_SYSTEM_RESET)
                        self.gdb_execute('c')

                elif self.config['read']:
                    self.read(self.config['file'], self.config['begin_address'], self.config['size'])

                    if self.config['reset']:
                        self.write_int(self.param_buf + NVM_Programmer.JTAG_PARAM_COMMAND, NVM_Programmer.JTAG_COMMAND_SYSTEM_RESET)
                        self.gdb_execute('c')

                elif self.config['file']:
                    if self.config['partition']:
                        self.generate_download_table()
                        self.write_download_table(os.path.abspath(os.path.join(FW_UPGRADE_SCRIPTS_PATH, "generated_download_table.xml")))
                    else:
                        self.download(self.config['file'], True, self.config['begin_address'])

                    if self.config['reset']:
                        self.write_int(self.param_buf + NVM_Programmer.JTAG_PARAM_COMMAND, NVM_Programmer.JTAG_COMMAND_SYSTEM_RESET)
                        self.gdb_execute('c')

                elif self.config['reset']:
                    print('Reset system.')
                    self.write_int(self.param_buf + NVM_Programmer.JTAG_PARAM_COMMAND, NVM_Programmer.JTAG_COMMAND_SYSTEM_RESET)
                    self.gdb_execute('c')
                    
                elif self.config['table']:
                    self.write_download_table(self.config['table'])
                    
                else:
                    raise Exception(' error: Invalid commands')

            except:
                self.cleanup_ram_image()
                raise

            # *** End tool specific code ***
        except:
            raise
        finally:
            self.cleanup()

    def init_ram_image(self, ram_image=DEFAULT_RAM_IMAGE):
        '''
        Downloads a RAM applcation onto the target.

        Parameters:
            ram_image: path to the RAM image being downloaded and initialized.
        '''

        # Load the NVM programmer RAM image.
        self.load_elf(ram_image)
        self.gdb_execute('b JTAG_Run')

        self.result_buf = self.get_symbol_info('JTAG_Param')['address']
        self.param_buf = self.result_buf
        self.gdb_execute('c')

    def cleanup_ram_image(self):
        '''
        Cleans up the RAM application on the target.
        '''
        pass

    def set_nvm_name(self):
        '''
        Set nvm device type name on the target.
        '''
        device_type_name_buf = self.get_symbol_info('Device_Type')['address']

        if self.config['nvm_name'] == 'flash':
            self.write_int(device_type_name_buf, NVM_Programmer.NVM_TYPE_NAME_FLASH)
        elif self.config['nvm_name'] == 'rram':
            self.write_int(device_type_name_buf, NVM_Programmer.NVM_TYPE_NAME_RRAM)
        elif self.config['nvm_name'] == 'otp':
            self.write_int(device_type_name_buf, NVM_Programmer.NVM_TYPE_NAME_OTP)
        else:
            raise Exception(' error: argument -n/--nvm-name: invalid choice, choose from rram, flash')

    def download(self, nvm_image, is_binary=False, begin_address=0):
        '''
        Downloads and initialize a RAM applcation on the target.

        Parameters:
            nvm_image: path to the NVM image being programmed.
            is_binary: flag indicates if the NVM image is a bin file.
            begin_address:  the start address where the bin file should be programmed.
        '''
        start_time = time.time()
        nvm_image = nvm_image.replace(os.sep, '/')

        self.set_nvm_name()

        # Get the address and size of the buffers.
        data_buf = self.get_symbol_info('JTAG_Param')
        data_buf_addr = data_buf['address'] + NVM_Programmer.JTAG_PARAM_BUFFER_OFFSET
        data_buf_size = data_buf['size'] - NVM_Programmer.JTAG_PARAM_BUFFER_OFFSET

        if self.config['nvm_name'] == 'flash':
            if begin_address & 0xFFF:
                raise Exception('flash programming begin address is not 4096 bytes aligned')
        elif self.config['nvm_name'] == 'rram':
            if begin_address & 0x3:
                raise Exception('rram programming address is not 4 bytes aligned')

        if is_binary:
            bin_str = 'binary'
            hex_file = hexfile.HexFile()
            with open(nvm_image, 'rb') as file:
                hex_file.add(begin_address, file.read())
        else:
            bin_str = ''
            hex_file = hexfile.HexFile(nvm_image)

        print('Programming {}'.format(nvm_image))

        # Loop through and program all blocks in the file.
        for start_address, size in hex_file.block_list():
            print('Programming {} bytes to {} 0x{:08X}'.format(size, self.config['nvm_name'], start_address))

            # Determine the offset address of the data in the file. If
            # this is a binary file, the offset will be zero.
            if is_binary:
                file_offset = 0;
            else:
                file_offset = start_address

            write_len = data_buf_size

            # Transfer the image to data_buf_addr on target in pieces
            # and program them into NVM
            address = start_address
            length = size
            try:
                while length:
                    if write_len > length:
                        write_len = length

                    # Load the next block of the image into the DataBuffer
                    self.gdb_execute('restore {} {} {} 0x{:08X} 0x{:08X}'.format(nvm_image, bin_str, data_buf_addr - file_offset, file_offset, file_offset + write_len))

                    # Execute the Write command and verify the result
                    self.write_int(self.param_buf + NVM_Programmer.JTAG_PARAM_ADDRESS, address)
                    self.write_int(self.param_buf + NVM_Programmer.JTAG_PARAM_SIZE, write_len)
                    self.write_int(self.param_buf + NVM_Programmer.JTAG_PARAM_COMMAND, NVM_Programmer.JTAG_COMMAND_PROGRAM)
                    self.gdb_execute('c')

                    write_result = self.read_int(self.result_buf + NVM_Programmer.JTAG_PARAM_STATUS)
                    if write_result:
                        raise Exception('Write operation failed:{}, error code = {}'.format(NVM_Programmer.ERROR_MAP[write_result], write_result))

                    address += write_len
                    file_offset += write_len
                    length -= write_len
                    print('.', end='', file=sys.stdout, flush=True)
            except:
                raise
            finally:
                print()

            #Get the checksum of what was programmed
            self.write_int(self.param_buf + NVM_Programmer.JTAG_PARAM_ADDRESS, start_address)
            self.write_int(self.param_buf + NVM_Programmer.JTAG_PARAM_SIZE, size)
            self.write_int(self.param_buf + NVM_Programmer.JTAG_PARAM_COMMAND, NVM_Programmer.JTAG_COMMAND_COMPUTE_CRC)
            self.gdb_execute('c')
            crc_result = self.read_int(self.result_buf + NVM_Programmer.JTAG_PARAM_STATUS)

            # Compare the checksum
            image_crc = binascii.crc32(hex_file.get(start_address, size)) & 0xFFFFFFFF
            if crc_result != image_crc:
               raise Exception('CRC32 verification failed.')

            end_time = time.time()
            print('Image programmed successfully, time elapsed {} seconds.'.format(end_time - start_time))

    def partial_erase(self):
        '''
        partial erase for nvm.
        '''
        start_time = time.time()
        self.set_nvm_name()

        begin_address = self.config['begin_address']
        erase_size = self.config['size']

        if self.config['nvm_name'] == 'flash':
            if begin_address & 0xFFF:
                raise Exception('flash partial erase address is not 4096 bytes aligned')
            if erase_size & 0xFFF:
                raise Exception('flash partial erase size is not 4096 bytes aligned')
        elif self.config['nvm_name'] == 'rram':
            if begin_address & 0x3:
                raise Exception('rram partial erase address is not 4 bytes aligned')

        print('Partial erasing {} {} bytes from 0x{:08X}, please wait for a moment ......'.format(self.config['nvm_name'], erase_size, begin_address))

        # Execute the partial erase command and verify the result
        self.write_int(self.param_buf + NVM_Programmer.JTAG_PARAM_ADDRESS, begin_address)
        self.write_int(self.param_buf + NVM_Programmer.JTAG_PARAM_SIZE, erase_size)
        self.write_int(self.param_buf + NVM_Programmer.JTAG_PARAM_COMMAND, NVM_Programmer.JTAG_COMMAND_PARTIAL_ERASE)
        self.gdb_execute('c', NVM_Programmer.ERASE_TIMEOUT)
        erase_result = self.read_int(self.result_buf + NVM_Programmer.JTAG_PARAM_STATUS)
        if erase_result:
            raise Exception('Partial erase operation failed:{}, error code = {}'.format(NVM_Programmer.ERROR_MAP[erase_result], erase_result))


        end_time = time.time()

        print('Partial erase successfully, time elapsed {} seconds.'.format(end_time - start_time))

    def chip_erase(self):
        '''
        Chip erase for flash.
        '''
        start_time = time.time()
        if self.config['nvm_name'] != 'flash':
            print('Chip erase only support flash')
            return

        self.set_nvm_name()

        print('Chip erasing {}, wait for a moment ......'.format(self.config['nvm_name']))

        # Execute the chip command and verify the result
        self.write_int(self.param_buf + NVM_Programmer.JTAG_PARAM_COMMAND, NVM_Programmer.JTAG_COMMAND_CHIP_ERASE)
        self.gdb_execute('c', NVM_Programmer.ERASE_TIMEOUT)
        erase_result = self.read_int(self.result_buf + NVM_Programmer.JTAG_PARAM_STATUS)
        if erase_result:
            raise Exception('Chip erase operation failed:{}, error code = {}'.format(NVM_Programmer.ERROR_MAP[erase_result], erase_result))

        end_time = time.time()

        print('Chip erase successfully, time elapsed {} seconds.'.format(end_time - start_time))

    def read(self,nvm_read, begin_address, size):
        '''
        Read the nvm data.

        Parameters:
            nvm_read: path to store the NVM data being read.
            begin_addr:  the start address where the NVM data should be read.
            size:  the size shold be read.
        '''
        DEFAULT_FILE = 'nvm_read.txt'
        start_time = time.time()

        if nvm_read == None:
          nvm_read = DEFAULT_FILE.replace(os.sep, '/')
          print_to_console = True
        else:
          nvm_read = nvm_read.replace(os.sep, '/')
          print_to_console = False

        self.set_nvm_name()

        # Get the address and size of the buffers.
        data_buf = self.get_symbol_info('JTAG_Param')
        data_buf_addr = data_buf['address'] + NVM_Programmer.JTAG_PARAM_BUFFER_OFFSET
        data_buf_size = data_buf['size'] - NVM_Programmer.JTAG_PARAM_BUFFER_OFFSET

        print('********************************************************************************')
        print('Reading {} {} bytes from 0x{:08X}'.format(self.config['nvm_name'], size, begin_address))


        read_len = data_buf_size

        # First read don't append, all subsequent read do append to output file.
        append = False

        # Read the nvm data from data_buf_addr on target in pieces
        address = begin_address
        length = size
        try:
            while length:
                if read_len > length:
                    read_len = length

                # Execute the read command and verify the result
                self.write_int(self.param_buf + NVM_Programmer.JTAG_PARAM_ADDRESS, address)
                self.write_int(self.param_buf + NVM_Programmer.JTAG_PARAM_SIZE, read_len)
                self.write_int(self.param_buf + NVM_Programmer.JTAG_PARAM_COMMAND, NVM_Programmer.JTAG_COMMAND_READ)
                self.gdb_execute('c')
                read_status = self.read_int(self.result_buf + NVM_Programmer.JTAG_PARAM_STATUS)
                if read_status:
                    raise Exception('Read operation failed:{}, error code = {}'.format(NVM_Programmer.ERROR_MAP[read_status], read_status))

                # Read the DataBuffer to file on Host(PC)
                if append == True:
                    self.gdb_execute('append binary memory {} 0x{:08X} 0x{:08X}'.format(nvm_read, data_buf_addr, data_buf_addr + read_len))
                else:
                    self.gdb_execute('dump binary memory {} 0x{:08X} 0x{:08X}'.format(nvm_read, data_buf_addr, data_buf_addr + read_len))

                address += read_len
                length -= read_len
                # All subsequent reads should be appended to file on Host (PC)
                append = True
                #print('.', end='', file=sys.stdout, flush=True)
        except:
            raise
        #finally:
        #    print()

        end_time = time.time()

        if print_to_console == True:
          with open(DEFAULT_FILE, 'rb') as f:
            #content=f.read()
            print('Value:')
            #print(content.hex().upper())
            cnt = 0
            format_str = ""
            while True:
                byte = f.read(1)
                str = byte.hex().upper()
                if not byte:
                    break
                cnt = cnt + 1
                mid_line = cnt % 8
                end_line = cnt % 16

                if mid_line != 0:
                    format_str += f"{str} "
                else:
                    if end_line != 0:
                        format_str += f"{str}  "
                    else:
                        format_str += f"{str}"
                        print (format_str)
                        format_str = ""

            if format_str != "":
                print (format_str)

        print('Read {} successfully, time elapsed {} seconds.'.format(self.config['nvm_name'], end_time - start_time))
        print('********************************************************************************')

    def generate_download_table(self):
        cmd = ["python", os.path.abspath(os.path.join(FW_UPGRADE_SCRIPTS_PATH, "gen_download_table.py")),
                "--app", os.path.abspath(self.config['file']),
                "--config", os.path.abspath(os.path.join(FW_UPGRADE_SCRIPTS_PATH,"download_config.xml"))]
        if self.config['all']:
            cmd.append("--all")
        cur_dir = os.getcwd()
        try:
            os.chdir(FW_UPGRADE_SCRIPTS_PATH)
            subprocess.check_call(cmd)
        finally:
            os.chdir(cur_dir)

    def write_download_table(self, xml):
        table = []
        download_table_parser = Download_Table()
        table = download_table_parser.from_xml_file(xml)
        for item in table:
            file, begin, size, location = item
            self.config['nvm_name'] = location
            if file == "":
                self.config['begin_address'] = begin
                self.config['size'] = size
                self.partial_erase()
            else:
                self.download(file, True, begin)
        
def main():
    '''
    Main entry point for the tool script.
    '''
    tool = NVM_Programmer()

    try:
        tool.run()
    except VerifyError as error:
        print('Verification Failed: {}!!!'.format(str(error)))

    print()

if __name__ == "__main__":
    main()
