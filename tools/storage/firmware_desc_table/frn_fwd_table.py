#!/usr/bin/python
#===============================================================================
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#===============================================================================

#===============================================================================
# $QTI_LICENSE_QDN_SH$

#===============================================================================

import struct
import xml.etree.ElementTree as ET
import xml.dom.minidom as minidom
import math
import os
import sys
import ntpath


class Firmware_Descriptor_Entry:
    ''' Firmware Descriptor Entry, stores the data and translates it into
    binary

    Firmware Descriptor Entries
    Little endian         <
    uint32_t id           I
    uint32_t rank         I
    uint32_t format       I
    uint32_t address      I
    uint32_t state       I
    uint32_t version      I
    uint8_t  reserved[8]  s*8
    '''

    FWD_ENTRIES_FORMAT = '<IIIIII8s'
    # size of a single firmware descriptor entry.
    fde_size = 32
    fde_packed = struct.Struct(FWD_ENTRIES_FORMAT)
    reserve_size = 8

    def __init__ (self):
        ''' Create an empty Firware Partition Entry
        '''
        # if you ever wonder why filename vs file_name, it is because rawprogra.xml used in MSMs use filename, not file_name.
        self.filename = ""

        self.id = 0
        self.rank = 3
        self.format = 0
        self.address = 0
        self.state = 0
        self.version = 0
        
        temp = [0x00] * self.reserve_size

        if sys.version_info[0] < 3:
            self.reserved = str(bytearray(temp))
        else:
            self.reserved = bytearray(temp)

    def to_binary (self):
        ''' Convert the firmware descriptor entry into a packed binary
        form '''
        if isinstance(self.reserved,str):
            self.reserved = self.reserved.encode('utf-8')            

        data = self.fde_packed.pack(self.id,
                                    self.rank,
                                    self.format,
                                    self.address,
                                    self.state,
                                    self.version,
                                    self.reserved
                                    )
        return data;

    def from_binary (self, data):
        ''' Convert the binary packed form of firmware descriptor into a
        easily manageable class. '''

        all_bytes =  self.fde_packed.unpack(data)
        self.id = all_bytes[0]
        self.rank = all_bytes[1]
        self.format = all_bytes[2]
        self.address = all_bytes[3]
        self.state = all_bytes[4]
        self.version = all_bytes[5]
        self.reserved = all_bytes[6]

    def __str__ (self):
        ''' Convert the firmware descriptor entry into a nicely printable
        string '''
        data = ""
        data = data + "Image ID:  %d (0x%X)\n" % (self.id, self.id)
        data = data + "File Name: %s\n" % (self.filename)
        data = data + "Rank:      %d (0x%X)\n" % (self.rank, self.rank)
        data = data + "Format:    %d (0x%X)\n" % (self.format, self.format)
        data = data + "Address:   0x%X\n" % (self.address)
        data = data + "State:    %d (0x%X)\n" % (self.state, self.state)
        data = data + "Version:   %d (0x%X)\n" % (self.version, self.version)
        
        if sys.version_info[0] < 3:
            data = data + "Reserved:  0x%s\n" % (self.reserved.encode('hex').upper())
        else:
            data = data + "Reserved:  0x%s\n" % (self.reserved.hex())

        return data

    def __to_xml__ (self, xml_element):
        ''' Adds all the members of this class into the xml element.
        used by to_xml and to_program Methods. '''

        xml_element.set ('id', str(self.id))
        xml_element.set ('filename', str(self.filename))
        xml_element.set ('rank', str(self.rank))
        xml_element.set ('format', str(self.format))
        xml_element.set ('address', '0x%X' % (self.address))
        xml_element.set ('state', str(self.state))
        xml_element.set ('version', str(self.version))

        if self.reserved == [0x00] * self.reserve_size:
            header.set('reserved', "0x" + str(self.reserved.encode('hex').upper()))

    def to_xml (self):
        ''' Converts a single partition entry to an XML object.This object
        can be printed to XML by using ET.tostring(fde.to_xml) '''
        data = ET.Element('fde')
        self.__to_xml__(data)
        return data

    def from_xml (self, xml_root):
        ''' Parses the XML Root from an ElementTree, the XML data should
        look like:
        '''
        if xml_root.tag != 'fde':
            raise AssertionError("Trying to parse something that is not a fde." % (size))

        self.filename = xml_root.attrib['filename']

        self.id = int(xml_root.attrib['id'], 0)
        self.rank = int(xml_root.attrib['rank'], 0)
        self.format = int(xml_root.attrib['format'], 0)
        self.address = int(xml_root.attrib['address'], 0)
        self.state = int(xml_root.attrib['state'], 0)
        self.version = int(xml_root.attrib['version'], 0)

        return


class Firmware_Descriptor_Table:
    ''' Definition of one File Descriptor Table

    FWD Table Header
    Little endian        <
    uint32_t sig          I
    uint32_t num_fde      I
    uint8_t  reserved[24] s*24
    '''
    FWD_TABLE_HEADER = '<II24s'
    # Size in bytes of the header specified above.
    header_size = 32
    # Number of reserved bytes
    reserve_size = 24

    FWD_TABLE_SIGNATURE = 0x54445746 #same as "FWDT"

    fdt_packed = struct.Struct(FWD_TABLE_HEADER)

    def __init__ (self):
        ''' Create the Firmware Descriptor Table structure with no entries
        in it. '''
        self.signature = self.FWD_TABLE_SIGNATURE
        self.num_fde = 0

        #create a bytearray filled with 0xFF
        temp = [0x00] * self.reserve_size
        self.entries = []

        if sys.version_info[0] < 3:
            self.reserved = str(bytearray(temp))
        else:
            self.reserved = bytearray(temp)

    def add_entry (self, entry):
        self.entries.append(entry)
        self.num_fde = self.num_fde + 1

    def to_binary (self):
        ''' Convert the firmware descriptor entry into a packed binary
        form '''
        if isinstance(self.signature,str):
            self.signature = self.signature.encode('utf-8')
        if isinstance(self.reserved,str):
            self.reserved = self.reserved.encode('utf-8')            
        data = self.fdt_packed.pack(self.signature,
                                    self.num_fde,
                                    self.reserved
                                    )
        for entry in self.entries:
            data = data + entry.to_binary()

        return data;

    def from_binary (self, data):
        ''' Convert the binary packed form of firmware descriptor table
        header into a easily manageable class. '''
        offset = self.header_size

        all_bytes = self.fdt_packed.unpack(data[:offset])
        self.signature = all_bytes[0]
        self.num_fde = all_bytes[1]
        self.reserved = all_bytes[2]

        self.entries = []

        entry_size = Firmware_Descriptor_Entry.fde_size
        for count in range(self.num_fde):
            entry = Firmware_Descriptor_Entry()
            entry.from_binary (data[offset:offset + entry_size])
            self.entries.append(entry)
            offset = offset + entry_size

    def __str__ (self):
        data = "HEADER:\n"
        data = data + ""
        data = data + "Signature: 0x%X\n" % (self.signature)
        data = data + "Num_fde:%d (0x%X)\n" % (self.num_fde, self.num_fde)

        if sys.version_info[0] < 3:
            data = data + "Reserved:  0x%s\n" % (self.reserved.encode('hex').upper())
        else:
            data = data + "Reserved:  0x%s\n" % (self.reserved.hex())

        data = data + "\nENTRIES:\n"

        for entry in self.entries:
            data = data + str(entry)
            data = data + "\n"

        return data

    def to_xml(self):
        ''' Generates an XML Element that contains the Fimware Descriptor
        Table data. '''
        table = ET.Element('fdt')
        header = ET.SubElement(table, 'hdr')
        header.set('signature', hex(self.signature))

        if self.reserved == [0x00] * self.reserve_size:
            header.set('reserved', "0x" + str(self.reserved.encode('hex').upper()))

        for entry in self.entries:
            data = entry.to_xml()
            table.append(data)

        return table

    def to_xml_str (self):
        # The XML Element Tree module does not do pretty print.
        # Convert it to another module and back to string to get nice
        # output.
        table = self.to_xml()
        xml_unformat = ET.tostring(table, 'utf-8')
        reparse = minidom.parseString(xml_unformat)
        return reparse.toprettyxml(indent="   ")

    def from_xml (self, xml_root):
        ''' Parses the XML Root from an ElementTree, the XML data should
        look like:
        <fdt>
          <header signature="0x54445746"/>
          <fde filename="FERMION_SBL_HASHED.elf" type="0" rank="0" fw_format="0" address="0x20A400" state="1" version="1"/>
          <fde filename="FERMION_SBL_HASHED.elf" type="0" rank ="2" fw_format="0" address="0x212400" state="1" version="1"/>
          <fde filename="" type="1" rank="" address="" state="" version=""/>
        </fdt>
        '''
        if xml_root.tag != 'fdt':
            raise AssertionError("Trying to parse something that is not a table." % (size))

        #Clear any pre-existing data.
        self.__init__()

        for child in xml_root:
            if child.tag == 'hdr':
                if 'signature' in child.attrib:
                    self.signature = int(child.attrib['signature'], 0)

                if 'reserved' in child.attrib:
                    if child.attrib['reserved'][:2] == '0x':
                        reserved = child.attrib['reserved'][2:]
                    else:
                        reserved = child.attrib['reserved']
                    self.reserved = str(reserved.decode('hex'))

            elif child.tag == 'fde':
                entry = Firmware_Descriptor_Entry()
                entry.from_xml(child)
                self.add_entry (entry)

        return

    def __iter__(self):
        ''' Iterate through each entry in the partition table '''
        return self.entries.__iter__()

def xml_to_bin(fdtxml, fdtbin, printtable, size):
        xml = ET.parse(fdtxml)
        fdt_descriptor = xml.getroot()

        if fdt_descriptor.tag != 'fdt':
            raise ValueError("XML didn't start with the correct tag <fdt>")

        fdt = Firmware_Descriptor_Table()
        fdt.from_xml(fdt_descriptor)

        data = bytearray()
        #create a bytearray filled with 0xFF
        temp = [0xff] * size
        pad = bytearray(temp)

        t = fdt.to_binary()
        size = len(t)
        data.extend(t)
        data.extend(pad[size:])
        
        try:
            with open(fdtbin , 'wb') as f:
                f.seek(0)
                f.write(data)
                f.close()
        except IOError as e:
            raise AssertionError("Can't open file %s" % (fdtbin))

        if printtable:
            print(str(fdt))

def bin_to_xml(fdtbin, fdtxml, printtable):
        fdt = Firmware_Descriptor_Table()

        data = bytearray()

        try:
            with open(fdtbin , 'rb') as f:
                f.seek(0)
                data = f.read()
                f.close()
        except IOError as e:
            raise AssertionError("Can't open file %s" % (fdtbin))
        
        fdt.from_binary(data)

        xml_string = fdt.to_xml_str()
        
        try:
            with open(fdtxml , 'wb') as f:
                f.seek(0)
                f.write(xml_string.encode("utf-8"))
                f.close()
        except IOError as e:
            raise AssertionError("Can't open file %s" % (fdtxml))

        if printtable:
            print(str(fdt))

def main():

    # Give a version number to this script.
    script_version = '1.0'

    import argparse

    tool_verbose_description = """Tool to generate fdt bin from fdt xml or generate fdt xml from fdt bin
    Example Usage:
    1. Generate fdt bin from fdt xml
       Run: python frn_fwd_table.py --fdtxml fdtxml.xml --fdtbin fdtbin.bin --ctype 0 --printtable
    2. Generate fdt xml from fdt bin
       Run: python frn_fwd_table.py --fdtxml fdtxml.xml --fdtbin fdtbin.bin --ctype 1 --printtable
    """
    parser = argparse.ArgumentParser(description=tool_verbose_description, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--fdtxml', type=str, required=True, help='The fdt.xml file for the Firmware Descriptor')
    parser.add_argument('--fdtbin', type=str, required=True, help='Firmware Descriptor Table Binary output file')
    parser.add_argument('--ctype', help='Convert type. 0: xml to binary, 1: binary to xml', type=int, choices=[0,1], default=0)
    parser.add_argument('--printtable', help='Print the partition table as a friendly string to the output.', default=False, action='store_true')
    
    args = parser.parse_args()

    convert_type = args.ctype

    if convert_type == 0:
        print("Convert from xml to bin")
        binary_size = 1024
        xml_to_bin(args.fdtxml, args.fdtbin, args.printtable, binary_size)

    elif convert_type == 1:
        print("Convert from bin to xml")
        bin_to_xml(args.fdtbin, args.fdtxml, args.printtable)    

if __name__ == "__main__":
    main()
