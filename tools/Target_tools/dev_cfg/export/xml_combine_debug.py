import os, re

filenames = ['log_debug.xml','roaming_debug.xml','beacon_power_debug.xml','nt_powersave_twt_debug.xml',
			 'wifi_app_debug.xml','rate_adaptation_debug.xml','wifi_security_debug.xml','connection_debug.xml','performance_debug.xml',
			 'mac_debug.xml','neut2_debug.xml']
basefile = 'base_xml_to_compare.xml'

for fname in filenames:  	
    with open(fname) as f1, open(basefile) as f2:
        for l1, l2 in zip(f1, f2):
            if l1 != l2:
                print (l1);
