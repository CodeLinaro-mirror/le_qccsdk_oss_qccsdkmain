#!/usr/bin/env python
############################################################################
#
############################################################################


from __future__ import print_function
from zipfile import ZipFile
import hashlib
import sys
import os, stat
import shutil

def _zip_file(filename, directory):
    """Create zip file

    Args:
        filename (zip): Zip filename to create
        directory (str): Path to compress
    """
    print('zip file %s to %s' % (directory, filename))
    zip_file = ZipFile(filename, 'w')
    for folder, _subfolders, filenames in os.walk(directory):
        for entry in filenames:
            path = os.path.join(folder, entry)
            zip_file.write(path)
    zip_file.close()

def main():
    path = os.getcwd() + '/BIN/wifi/qcc532x_qcc522x/mib'
    if not os.path.exists(path):   # create folders if not exists
        os.makedirs(path)

    path = os.getcwd() + '/BIN/wifi/qcc532x_qcc522x/binaries'
    if not os.path.exists(path):   # create folders if not exists
        os.makedirs(path)
    #os.mkdir(path,0x755)

    path = os.getcwd() + '/BIN-DEBUG/wifi/qcc532x_qcc522x/binaries/DEBUG'
    if not os.path.exists(path):   # create folders if not exists
        os.makedirs(path)

    path = os.getcwd() + '/BIN-DEBUG/wifi/qcc532x_qcc522x/binaries/RELEASE/'
    if not os.path.exists(path):   # create folders if not exists
        os.makedirs(path)

    #debug_path = '/BIN-DEBUG/wifi/qcc532x_qcc522x/binaries/DEBUG/'


    source = os.getcwd() + '/build/FERMION/DEBUG/bin/FERMION.elf'
    destin = os.getcwd() + '/BIN-DEBUG/wifi/qcc532x_qcc522x/binaries/DEBUG/qcp5321_wlan.elf'
    shutil.copy(source, destin)

    source = os.getcwd() + '/build/FERMION/DEBUG/bin/FERMION.hex'
    destin = os.getcwd() + '/BIN-DEBUG/wifi/qcc532x_qcc522x/binaries/DEBUG/qcp5321_wlan.hex'
    shutil.copy(source, destin)

    source = os.getcwd() + '/build/FERMION/DEBUG/bin/FERMION.bin'
    destin = os.getcwd() + '/BIN-DEBUG/wifi/qcc532x_qcc522x/binaries/DEBUG/qcp5321_wlan.bin'
    shutil.copy(source, destin)

    source = os.getcwd() + '/build/FERMION/DEBUG/bin/FERMION_DFU.elf'
    destin = os.getcwd() + '/BIN-DEBUG/wifi/qcc532x_qcc522x/binaries/DEBUG/qcp5321_wlan_dfu.elf'
    shutil.copy(source, destin)

    #source = os.getcwd() + '/build/FERMION/DEBUG/bin/FERMION_DFU.hex'
    #destin = os.getcwd() + '/BIN-DEBUG/wifi/qcc532x_qcc522x/binaries/DEBUG/qcp5321_wlan_dfu.hex'
    #shutil.copy(source, destin)

    '''source = os.getcwd() + '/build/FERMION/DEBUG/bin/FERMION_DFU.bin'
    destin = os.getcwd() + '/BIN-DEBUG/wifi/qcc532x_qcc522x/binaries/DEBUG/qcp5321_wlan_dfu.bin'
    shutil.copy(source, destin)'''

    source = os.getcwd() + '/build/FERMION_PBL/DEBUG/bin/FERMION_PBL.elf'
    destin = os.getcwd() + '/BIN-DEBUG/wifi/qcc532x_qcc522x/binaries/DEBUG/qcp5321_pbl.elf'
    shutil.copy(source, destin)

    source = os.getcwd() + '/build/FERMION_PBL/DEBUG/bin/FERMION_PBL.hex'
    destin = os.getcwd() + '/BIN-DEBUG/wifi/qcc532x_qcc522x/binaries/DEBUG/qcp5321_pbl.hex'
    shutil.copy(source, destin)

    source = os.getcwd() + '/build/FERMION_PBL/DEBUG/bin/FERMION_PBL.bin'
    destin = os.getcwd() + '/BIN-DEBUG/wifi/qcc532x_qcc522x/binaries/DEBUG/qcp5321_pbl.bin'
    shutil.copy(source, destin)

    source = os.getcwd() + '/build/FERMION_PBL/DEBUG/bin/FERMION_PBL_DFU.elf'
    destin = os.getcwd() + '/BIN-DEBUG/wifi/qcc532x_qcc522x/binaries/DEBUG/qcp5321_pbl_dfu.elf'
    shutil.copy(source, destin)

    source = os.getcwd() + '/build/FERMION_SBL/DEBUG/bin/FERMION_SBL.elf'
    destin = os.getcwd() + '/BIN-DEBUG/wifi/qcc532x_qcc522x/binaries/DEBUG/qcp5321_sbl.elf'
    shutil.copy(source, destin)

    source = os.getcwd() + '/build/FERMION_SBL/DEBUG/bin/FERMION_SBL.hex'
    destin = os.getcwd() + '/BIN-DEBUG/wifi/qcc532x_qcc522x/binaries/DEBUG/qcp5321_sbl.hex'
    shutil.copy(source, destin)

    source = os.getcwd() + '/build/FERMION_SBL/DEBUG/bin/FERMION_SBL.bin'
    destin = os.getcwd() + '/BIN-DEBUG/wifi/qcc532x_qcc522x/binaries/DEBUG/qcp5321_sbl.bin'
    shutil.copy(source, destin)

    source = os.getcwd() + '/build/FERMION_SBL/DEBUG/bin/FERMION_SBL_DFU.elf'
    destin = os.getcwd() + '/BIN-DEBUG/wifi/qcc532x_qcc522x/binaries/DEBUG/qcp5321_sbl_dfu.elf'
    shutil.copy(source, destin)

    source = os.getcwd() + '/build/FERMION_PBL/RELEASE/bin/FERMION_PBL.elf'
    destin = os.getcwd() + '/BIN-DEBUG/wifi/qcc532x_qcc522x/binaries/RELEASE/qcp5321_pbl.elf'
    shutil.copy(source, destin)

    source = os.getcwd() + '/build/FERMION_PBL/RELEASE/bin/FERMION_PBL_DFU.elf'
    destin = os.getcwd() + '/BIN-DEBUG/wifi/qcc532x_qcc522x/binaries/RELEASE/qcp5321_pbl_dfu.elf'
    shutil.copy(source, destin)

    source = os.getcwd() + '/build/FERMION_PBL/RELEASE/bin/FERMION_PBL.hex'
    destin = os.getcwd() + '/BIN-DEBUG/wifi/qcc532x_qcc522x/binaries/RELEASE/qcp5321_pbl.hex'
    shutil.copy(source, destin)

    source = os.getcwd() + '/build/FERMION_PBL/RELEASE/bin/FERMION_PBL.bin'
    destin = os.getcwd() + '/BIN-DEBUG/wifi/qcc532x_qcc522x/binaries/RELEASE/qcp5321_pbl.bin'
    shutil.copy(source, destin)

    source = os.getcwd() + '/core/wifi/config_ini/mib/QCP5321_ini.elf'
    destin = os.getcwd() + '/BIN-DEBUG/wifi/qcc532x_qcc522x/binaries/DEBUG/qcp5321_ini_dfu.elf'
    shutil.copy(source, destin)
    os.chmod(destin, 0o775)

    source = source = os.getcwd() + '/core/wifi/halphy/phyrf_svc/tools/bdfUtils/device/bdf/qcp5321/bdwlan.elf'
    destin = os.getcwd() + '/BIN-DEBUG/wifi/qcc532x_qcc522x/binaries/DEBUG/qcp5321_bdwlan_dfu.elf'
    shutil.copy(source, destin)
    os.chmod(destin, 0o775)

    source = source = os.getcwd() + '/core/wifi/halphy/phyrf_svc/tools/bdfUtils/device/bdf/qcp5321/bdwlan.bin'
    destin = os.getcwd() + '/BIN/wifi/qcc532x_qcc522x/binaries/qcp5321_bdwlan.bin'
    shutil.copy(source, destin)
    os.chmod(destin, 0o775)

    source = source = os.getcwd() + '/core/wifi/halphy/phyrf_svc/tools/bdfUtils/device/bdf/qcp5321/bdwlan.bin'
    destin = os.getcwd() + '/BIN-DEBUG/wifi/qcc532x_qcc522x/binaries/DEBUG/qcp5321_bdwlan.bin'
    shutil.copy(source, destin)
    os.chmod(destin, 0o775)

    source = source = os.getcwd() + '/core/wifi/halphy/phyrf_svc/tools/Regulatory/regdb.elf'
    destin = os.getcwd() + '/BIN-DEBUG/wifi/qcc532x_qcc522x/binaries/DEBUG/qcp5321_regdb_dfu.elf'
    shutil.copy(source, destin)
    os.chmod(destin, 0o775)

    source = source = os.getcwd() + '/core/wifi/halphy/phyrf_svc/tools/Regulatory/regdb.bin'
    destin = os.getcwd() + '/BIN/wifi/qcc532x_qcc522x/binaries/qcp5321_regdb.bin'
    shutil.copy(source, destin)
    os.chmod(destin, 0o775)

    source = source = os.getcwd() + '/core/wifi/halphy/phyrf_svc/tools/Regulatory/regdb.bin'
    destin = os.getcwd() + '/BIN-DEBUG/wifi/qcc532x_qcc522x/binaries/DEBUG/qcp5321_regdb.bin'
    shutil.copy(source, destin)
    os.chmod(destin, 0o775)

    source = source = os.getcwd() + '/core/wifi/config_ini/mib/mib.xml'
    destin = os.getcwd() + '/BIN/wifi/qcc532x_qcc522x/mib/mib.xml'
    shutil.copy(source, destin)
    os.chmod(destin, 0o775)

    os.chdir('BIN')
    _zip_file('wifi' + '.zip', 'wifi')

if __name__ == '__main__':
    main()

