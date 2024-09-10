'''
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
* *======================================================================== '''

import xml.etree.ElementTree as ET
import sys
import os

def generate_xml(filename):
    tree = ET.parse(filename)

    #xml_doc = ET.Element('root')
    root = tree.getroot()

    #product = ET.SubElement(xml_doc, 'product')

    #ml = ET.Element("metadata_list")
    #xsi = ET.SubElement(ml, "xmlns:xsi")

    dest = os.getcwd() + '/modules/wifi/config_ini/mib/mib.xml'
    #print(dest)
    if os.path.exists(dest):
        os.chmod(dest, 0o665)

    #ET.SubElement(product, name="").text = 0
    print('<!--  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.  -->')
    print('<!--    %%version  -->')
    print('<metadata_list xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="HydraMeta.xsd">')
    print('<!--  App  -->')
    print('<!--  TODO: derive the name and number from system.xml  -->')
    print('<metadata subsystem_name="wlan" subsystem_alias="wlan" subsystem_id="2" version="0">')

    print()
    i = 1 
    for props in root.iter('props'):
        print('<config_element name="{}" psid="{}">' .format(props.attrib.get('id_name'), str(i)))
        #print(props.attrib.get('id_name'))
        print('<label> {} </label>'.format(props.attrib.get('id_name')))
        print('<description_user>{} </description_user>' .format(props.attrib.get('helptext')))
        print('<type>uint32</type>')
        print('<default>{}</default>'.format(props.text))
        if props.attrib.get('min') is not None:
            print('<range_min>{}</range_min>'.format(props.attrib.get('min')))
        if props.attrib.get('max') is not None:
            print('<range_max>{}</range_max>'.format(props.attrib.get('max')))
        if props.attrib.get('oem_configurable') is not None:
            if props.attrib.get('oem_configurable') == 'true': 
                print('<is_internal>{}</is_internal>'.format('false'))
            else:
                print('<is_internal>{}</is_internal>'.format('true'))
        print('</config_element>')
        print()
        #print(props.text)
        i = i + 1

    #trees = ET.ElementTree(xml_doc)
    # trees.write('mib.xml')
    print('</metadata>')
    print('</metadata_list>') 
    
    
if __name__ == '__main__':
    generate_xml(sys.argv[1])

