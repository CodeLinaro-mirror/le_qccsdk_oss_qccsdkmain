import os, re

filenames = ['log_production.xml','roaming_production.xml','beacon_power_production.xml','nt_powersave_twt_production.xml',
			 'wifi_app_production.xml','rate_adaptation_production.xml','wifi_security_production.xml','connection_production.xml','performance_production.xml',
			 'mac_production.xml','rtt_ftm_production.xml','system_production.xml','neut2_production.xml']
basefile = 'base_xml_to_compare.xml'

for fname in filenames:  	
    with open(fname) as f1, open(basefile) as f2:
        for l1, l2 in zip(f1, f2):
            if l1 != l2:
                print (l1);
