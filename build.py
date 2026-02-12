# -*- coding:utf-8 -*-
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#

import os
import sys
import subprocess
import logging
from optparse import OptionParser
import shutil
import platform
import re

image_list = [ 
    'FERMION_SBL',
	'FERMION_HOSTED_SBL',
    'FERMION_IOE_QCLI_DEMO',
    'FERMION_FTM',
    'FERMION_HELLO_WORLD',
    'FERMION_POSIX_DEMO',
    'FERMION_NVM_PROGRAMMER',
    'FERMION_WIFI_LIB',
    'FERMION_FS_DEMO',
    'FERMION_MQTT_DEMO',
    'FERMION_MATTER_DEMO',
    'FERMION_QAT_DEMO',
    'FERMION_POWER_TEST_DEMO',
    'FERMION_AMBIENT_POWER_DEMO']
proj_conf = { 
    'FERMION_IOE_QCLI_DEMO':'demo/qcli_demo/prj.conf',
    'FERMION_SBL':'demo/qcli_demo/prj.conf',
	'FERMION_HOSTED_SBL':'demo/qcli_demo/prj.conf',
    'FERMION_FTM':'demo/ftm/ftm_prj.conf',
    'FERMION_HELLO_WORLD':'demo/hello_world/prj.conf',
    'FERMION_POSIX_DEMO':'demo/posix_demo/prj.conf',
    'FERMION_NVM_PROGRAMMER':'demo/qcli_demo/prj.conf',
    'FERMION_FS_DEMO':'demo/fs_demo/prj.conf',
    'FERMION_MQTT_DEMO':'demo/mqtt_demo/prj.conf',
    'FERMION_MATTER_DEMO':'demo/matter_demo/prj.conf',
    'FERMION_QAT_DEMO':'demo/qat_demo/prj.conf',
    'FERMION_POWER_TEST_DEMO':'demo/powertest_demo/prj.conf',
    'FERMION_AMBIENT_POWER_DEMO':'demo/ambient_power_demo/prj.conf'
}
default_build_output = 'build'
gn_path = '/pkg/qct/software/ubuntu/matter_tool'
default_build_id = '0999'

SOCKET_BOARD_CHIPV1 = 'qcc730v1_socket'
SOCKET_BOARD_CHIPV2 = 'qcc730v2_socket'
EVB_V11_HOSTLESS = 'qcc730v2_evb11_hostless'
EVB_V12_HOSTLESS = 'qcc730v2_evb12_hostless'
EVB_V13_HOSTLESS = 'qcc730v2_evb13_hostless'
BOARD_MQM405X = 'mqm405x'
BOARD_MQM405I = 'mqm405i'
BOARD_MQM730X = 'mqm730x'
BOARD_MQM730I = 'mqm730i'
BOARD_CQM730X = 'cqm730x'
BOARD_CQM730I = 'cqm730i'
BOARD_NONE = 'noboard' #means not related to any board
DEFAULT_BOARD_NAME = BOARD_NONE
ENV_BOARD_NAME = 'QCCSDK_BOARD_NAME'

log_formatter = logging.Formatter('[%(asctime)s]: %(message)s', datefmt = '%a, %d %b %Y %H:%M:%S')

global build_output
global dotconfig
global autoconfig
global gnconfig
global build_id
global g_val_board_name
global g_is_sdk_packed
global log_path

global main_options

cur_dir = os.getcwd()
project_root = cur_dir

def log_to_file_deco(arg = True, arg2 = True):
    def _deco(func):
        def wrapper(*args, **kwargs):
            logger = logging.getLogger()
            fh = logging.FileHandler(os.path.join(log_path, 'build-%s.log'%func.__name__), mode = 'w')
            fha = logging.FileHandler(os.path.join(log_path, 'build-all.log'), mode = 'a')
            fh.setFormatter(log_formatter)
            fha.setFormatter(log_formatter)
            if arg:
                logger.addHandler(fh)
                logger.addHandler(fha)
            ch = logging.StreamHandler()
            #ch.setFormatter(log_formatter)
            if arg2:
                logger.addHandler(ch)
            ret = func(*args, **kwargs)
            if arg:
                logger.removeHandler(fh)
                logger.removeHandler(fha)
            if arg2:
                logger.removeHandler(ch)
            return ret
        return wrapper
    return _deco

def print_cmd (cmd):
    cmd_dbg = ''
    for param in cmd:
        cmd_dbg += ' %s '%param
    logging.info(cmd_dbg)

def execute_cmd(cmd, log_file = None):
    #logging.info(cmd)
    print_cmd(cmd)
    log_to_file = []
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    while True:
        if process.poll() is None:
            line = process.stdout.readline()
            line = line.decode('utf-8').replace('\r', '')
            if line:
                logging.info(line)
                log_to_file.append(line)
        else:
            # to handle some last stdout is cached in linux
            line = process.stdout.readline()
            line = line.decode('utf-8').replace('\r', '')
            if line:
                logging.warning(line)
                log_to_file.append(line)
            else:
                break

    lines = process.stderr.readlines()
    for line in lines:
        line = line.decode('utf-8').replace('\r', '')
        if line:
            logging.warning(line)
            log_to_file.append(line)

    if (log_file):
        with open(log_file, 'w') as op:
            for ln in log_to_file:
                op.write(ln)
    #(stdout,stderr) = process.communicate()
    if (process.returncode != 0):
        logging.error('Failed')
        sys.exit(-1)

def execute_cmd_with_log(cmd):
    #logging.info(cmd)
    print_cmd(cmd)
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (stdout,stderr) = process.communicate()
    if (process.returncode != 0):
        logging.error('Failed')
    return (stdout,stderr,process.returncode)

def option_parser():
    parser = OptionParser(usage="usage: %prog [options] arguments", version="%prog 1.0")
    parser.add_option("--image", "-i", action="store", type="string", dest="image", help="Image name [FERMION, FERMION_QCLI_DEMO, FERMION_PBL]")
    parser.add_option("--board", "-b", action="store", type="string", dest="board", help="board name, also board dir name under boards/, such as [%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s]"
        %(SOCKET_BOARD_CHIPV1, SOCKET_BOARD_CHIPV2, EVB_V11_HOSTLESS, EVB_V12_HOSTLESS, EVB_V13_HOSTLESS, BOARD_NONE, BOARD_MQM405X, BOARD_MQM405I, BOARD_MQM730X, BOARD_MQM730I,BOARD_CQM730X, BOARD_CQM730I))
    parser.add_option("--all", "-a", action="store_true", default=False, dest="build_all", help="To build all images")
    parser.add_option("--out", "-o", action="store", type="string", dest="out_dir", help="Output directory")
    parser.add_option("--clean", "-c", action="store_true", default=False, dest="clean", help="To clean the build")
    parser.add_option("--menuconfig", "-m", action="store_true", default=False, dest="menuconfig", help="To run menuconfig")
    parser.add_option("--sign", "-s", action="store_true", default=False, dest="sign", help="Enable sign image")
    parser.add_option("--p2p", "-p", action="store_true", default=False, dest="p2p", help="Enable p2p flag")
    # parser.add_option("-d", action="store_true", default=True,  dest="debug", help="debug")
    return parser.parse_args()

@log_to_file_deco(True)
def gen_bdf_obj():
    global g_val_board_name
    regdb_path = os.path.join(project_root, 'modules/wifi/bin/regdb.bin')
    bdf_dir = os.path.join(project_root, 'modules/wifi/bin')
    #generate regdb.o
    cmd = 'arm-none-eabi-objcopy -I binary -O elf32-littlearm --binary-architecture arm'
    logging.info('Gen regdb obj ....')
    os.system('%s --rename-section .data=.regdb %s %s'%(cmd, regdb_path, os.path.join(build_output, 'regdb.o')))
    #generate bdwlan.o
    """
    if g_val_board_name==SOCKET_BOARD_CHIPV1:
        bdf_name = 'bdwlan.bin'
    elif g_val_board_name==SOCKET_BOARD_CHIPV2:
        bdf_name = 'bdwlan03.bin'
    elif g_val_board_name in [EVB_V11_HOSTLESS, BOARD_MQM405X, BOARD_MQM730X]:
        bdf_name = 'bdwlan01.bin'
    elif g_val_board_name==EVB_V12_HOSTLESS:
        bdf_name = 'bdwlan02.bin'
    elif g_val_board_name in [EVB_V13_HOSTLESS, BOARD_MQM405I, BOARD_MQM730I]:
        bdf_name = 'bdwlan03.bin'
    else:
        logging.warning('board=%s not supported', g_val_board_name)
        sys.exit(-1)
    logging.info('Gen bdf obj ....')
    os.system('%s --rename-section .data=.bdf %s %s'%(cmd, os.path.join(bdf_dir, bdf_name), os.path.join(build_output, 'bdwlan.o')))
    """

@log_to_file_deco(True)
def gen_dot_conf(image = 'fermion_legacy'):
    global g_val_board_name
    # python tools/kconfig_scripts/kconfig.py --handwritten-input-configs Kconfig build/output/.config build/output/include/autoconf.h build/output/kconfig-files-list.log demo/qcli_demo/prj.conf
    Kconfig_logfile = os.path.join(build_output, 'kconfig-files-list.log')
    if image == 'FERMION_WIFI_LIB':
        if not (main_options.p2p):
            Kconfig_file = './modules/wifi/Kconfig.lib'
        else:
            Kconfig_file = './modules/wifi/Kconfig_p2p.lib'
    else:
        prj_conf = proj_conf[image]
        board_defconfig = 'boards/%s/%s_defconfig'%(g_val_board_name, g_val_board_name)
        Kconfig_file = 'Kconfig'
    cmd = [ 'python', 'tools/kconfig_scripts/kconfig.py',
        '--handwritten-input-configs', Kconfig_file, 
        dotconfig,
        autoconfig,
        Kconfig_logfile,
    ]
    if image != 'FERMION_WIFI_LIB':
        cmd += [ '--configs_in', board_defconfig, prj_conf, ]
    logging.info('Gen dot config ....')
    execute_cmd(cmd)

@log_to_file_deco(True)
def gen_auto_conf():
    # python tools/kconfig_scripts/kconfig.py Kconfig .config build/output/include/autoconf.h build/output/kconfig-files-list.log .config
    Kconfig_logfile = os.path.join(build_output, 'kconfig-files-list.log')
    cmd = [ 'python', 'tools/kconfig_scripts/kconfig.py',
        'Kconfig',
        dotconfig,
        autoconfig,
        Kconfig_logfile,
        '--configs_in',
        dotconfig,
    ]
    logging.info('Gen auto config ....')
    execute_cmd(cmd)

@log_to_file_deco(True)
def gen_gn_main_config():
    # Gen gn config file
    cmd = [ 'python', 'build/gn/scripts/process_dotconfig.py',
        dotconfig,
        gnconfig,
    ]
    logging.info('Gen gn main config ....')
    execute_cmd(cmd)

@log_to_file_deco(True)
def gen_from_xml():
    # python tools/host_tools/dev_cfg/dev_xml_cfg_debug.py .
    cmd = [ 'python', 'tools/host_tools/dev_cfg/dev_xml_cfg_debug.py', '.', ]
    logging.info('Run dev_xml_cfg_debug.py ....')
    execute_cmd(cmd)

    # python tools/host_tools/dev_cfg/dev_cfg_debug.py .
    cmd = [ 'python', 'tools/host_tools/dev_cfg/dev_cfg_debug.py', '.', ]
    logging.info('Run dev_cfg_debug.py ....')
    execute_cmd(cmd)

@log_to_file_deco(True)
def pre_build_script(variant_name = 'FERMION_QCLI_DEMO', variant_image_id = 'MM'):
    # python build/freertos/eclipse-gcc/Scripts/chip_full_debug_halphy_prebuild.py . FERMION_QCLI_DEMO MM
    # python build/freertos/eclipse-gcc/Scripts/pbl_prebuild.py . FERMION_PBL PBL
    if variant_image_id == 'MM':
        pre_build_script = 'chip_full_debug_halphy_prebuild.py'
    if variant_image_id == 'PBL':
        pre_build_script = 'pbl_prebuild.py'
    if variant_image_id == 'SBL':
        pre_build_script = 'sbl_prebuild.py'
        cmd = [ 'python', 'build/freertos/eclipse-gcc/Scripts/' + pre_build_script, '.', variant_name, variant_image_id ]
    if variant_image_id == 'HOSTED_SBL':
        pre_build_script = 'sbl_prebuild.py'
        cmd = [ 'python', 'build/freertos/eclipse-gcc/Scripts/' + pre_build_script, '.', variant_name, variant_image_id ]
    #cmd = [ 'python', 'tools/host_tools/dev_cfg/' + pre_build_script, '.',
    #    variant_name, variant_image_id,
    #]
    logging.info('Run prebuild.py ....')
    execute_cmd(cmd)

@log_to_file_deco(True, False)
def gen_mib_from_xml():
    # python modules/wifi/config_ini/mib/xml_gen_from_xml.py tools/Target_tools/dev_cfg/export/master_xml.xml > modules/wifi/config_ini/mib/mib.xml
    cmd = [ 'python', 'modules/wifi/config_ini/mib/xml_gen_from_xml.py',
        'tools/Target_tools/dev_cfg/export/master_xml.xml',
    ]
    logging.info('Gen mib ....')
    (out, err, rc) = execute_cmd_with_log(cmd)
    out = out.decode('utf-8').replace('\r', '')
    if (rc != 0):
        logging.warning('mib_gen_from_xml failed')
        sys.exit(-1)
    with open('modules/wifi/config_ini/mib/mib.xml', 'wb') as outp:
        outp.write(out.encode('utf-8'))

def prepare_gn_args(image = 'FERMION'):
    global g_is_sdk_packed
    # prepare args.gn
    is_sdk_str = 'true' if g_is_sdk_packed else 'false'
    logging.warning('is_sdk_packed: %s' % is_sdk_str)
    if image == 'FERMION_WIFI_LIB': #image is not related to any board
        CHIP_VERSION_FERMION = 2
    elif g_val_board_name==SOCKET_BOARD_CHIPV1:
        CHIP_VERSION_FERMION = 1
    elif g_val_board_name in [EVB_V11_HOSTLESS, EVB_V12_HOSTLESS, SOCKET_BOARD_CHIPV2, EVB_V13_HOSTLESS, BOARD_MQM405X, BOARD_MQM405I, BOARD_MQM730X, BOARD_MQM730I, BOARD_CQM730X, BOARD_CQM730I]:
        CHIP_VERSION_FERMION = 2
    else:
        logging.warning('board=%s not supported', g_val_board_name)
        sys.exit(-1)
    logging.warning('Chip version: %d' % CHIP_VERSION_FERMION)
    args_content = []
    args_content.append('image_name="%s"' % (image))
    args_content.append('build_id="%d"' % (int(build_id)))
    args_content.append('is_sdk=%s' % is_sdk_str)
    args_content.append('CHIP_VERSION_FERMION=%d' % CHIP_VERSION_FERMION)
    args_content.append('board_name="%s"'%g_val_board_name)
    if image != 'FERMION_WIFI_LIB':
        with open(gnconfig, 'r') as gncfg:
            args = gncfg.readlines()
            for ln in args:
                ln = ln.split()
                args_content.append('%s' % ln[0])
    with open(gnconfig, 'wb') as outp:
        for ln in args_content:
            outp.write((ln+'\n').encode('utf-8'))

@log_to_file_deco(True)
def execute_gn_build(image = 'FERMION'):
    prepare_gn_args(image)
    #TO add more cases
    if image == 'FERMION_SBL':
        variant_image_id = 'SBL'
        pre_build_script(image, variant_image_id)
    if image == 'FERMION_HOSTED_SBL':
        variant_image_id = 'HOSTED_SBL'
        pre_build_script(image, variant_image_id)
        #logging.warning('image=%s ....' %(image))
    # gn gen
    #gn_args = '--args='
    #gn_args += 'image_name=\"%s\"' % (image)
    #gn_args += ' build_id=\"%s\"' % (build_id)
    #with open(gnconfig, 'r') as gncfg:
    #    args = gncfg.readlines()
    #    for ln in args:
    #        ln = ln.split()
    #        gn_args += ' %s' % ln[0]
    cmd = [ 'gn', 'gen', build_output, ]
    logging.warning('gn gen ....')
    execute_cmd(cmd)

    # gn args. For debug
    cmd = [ 'gn', 'args',  build_output, '--list' ]
    logging.warning('Run gn args --list ....')
    (out, err, rc) = execute_cmd_with_log(cmd)
    out = out.decode('utf-8').replace('\r', '')
    err = err.decode('utf-8').replace('\r', '')
    logging.warning(err)
    with open(os.path.join(log_path,'build-gn-args.log'), 'wb') as outp:
        outp.write(out.encode('utf-8'))
        outp.write(err.encode('utf-8'))
    if (rc != 0):
        sys.exit(-1)

    #cmd = [ 'ninja', '-v', '-d', 'keeprsp', '-C', build_output ]
    #logging.warning('Run ninja ....')
    #(out, err, rc) = execute_cmd_with_log(cmd)
    #out = out.decode('utf-8').replace('\r', '')
    #err = err.decode('utf-8').replace('\r', '')
    #logging.warning(out)
    #logging.warning(err)
    #with open(os.path.join(log_path,'build-ninja.log'), 'wb') as outp:
    #    outp.write(out.encode('utf-8'))
    #    outp.write(err.encode('utf-8'))
    #if (rc != 0):
    #    sys.exit(-1)
    if image == 'FERMION_WIFI_LIB':
        print('build wifi_lib in %s' % image)
        cmd = [ 'ninja', '-d', 'keeprsp', '-C', build_output, '-v', 'wifi_lib', ]
    else:
        cmd = [ 'ninja', '-d', 'keeprsp', '-C', build_output, '-v', 'final_target', ]
    logging.warning('Run ninja ....')
    #with open(os.path.join(log_path,'build-ninja.log'), 'w') as outp:
    execute_cmd(cmd, os.path.join(log_path,'build-ninja.log'))

@log_to_file_deco(True)
def init_matter():
    logging.info("Update Matter submodules")
    cmd = ['python', 'tools/matter/init_matter.py']
    execute_cmd(cmd)

def sign_image(build_output= '',image='FERMION_IOE_QCLI_DEMO'):

    image_type = 'app'
    if image =='FERMION_SBL':
        image_type = 'sbl'
    elif image =='FERMION_HOSTED_SBL':
        image_type = 'hosted_sbl'
    else:
        image_type = 'app'

    build_output = os.path.join(build_output,'bin')
    elf_file = os.path.join(build_output,image + '.elf')
    elf_dir = build_output
    current_path = os.path.dirname(os.path.abspath(__file__))
    sectools_path = os.path.join(current_path, './sectools')
    cmd = [ 'python', os.path.join(sectools_path, 'sectools.py'), 'secimage', '-i', elf_file, '-c', os.path.join(sectools_path, 'config/qcc730/qcc730_secimage.xml'), '-sa', '-g', image_type, '-o', elf_dir, ]
    logging.info('Creating App Signed Image ....')
    logging.info(cmd)
    execute_cmd(cmd)

def start_build(image = 'FERMION_WIFI_LIB', out_dir = default_build_output):
    global build_output
    global dotconfig
    global autoconfig
    global gnconfig
    global log_path

    build_output = os.path.join(out_dir, image)
    if not (main_options.p2p):
        build_output = os.path.join(build_output, 'DEBUG')
    else:
        build_output = os.path.join(build_output, 'DEBUG/p2p')
    if not os.path.exists(build_output):
        os.makedirs(build_output)

    log_path = os.path.join(build_output, 'log')
    full_log = os.path.join(log_path,'build-all.log')
    if os.path.exists(full_log):
        #shutil.rmtree(log_path)
        os.remove(full_log)
    if not os.path.exists(log_path):
        os.makedirs(log_path)
    gnconfig = os.path.join(build_output, 'args.gn')


    if (main_options.clean):
        print('Cleaning %s' % image)
        shutil.rmtree(os.path.join(out_dir, image))
    else:
        # Start here ...
        dotconfig = os.path.join(build_output, '.config')
        autoconfig = os.path.join(build_output, 'autoconf.h')
        if image == 'FERMION_WIFI_LIB':
            gen_dot_conf(image)
            gen_from_xml()
        else:
            gen_bdf_obj()
            gen_dot_conf(image)
            if main_options.menuconfig:
                print('Only Menuconfig')
                execute_cmd('menuconfig Kconfig')
                sys.exit(0)
            gen_auto_conf()
            gen_gn_main_config()
            gen_from_xml()
            gen_mib_from_xml()
        if image == 'FERMION_MATTER_DEMO':
            init_matter()
        execute_gn_build(image)
        # sign image if need
        if main_options.sign:
            if image == 'FERMION_WIFI_LIB':
                pass
            else:
                sign_image(build_output,image)

def setup_env():
    global build_id
    global g_is_sdk_packed
    path_env = os.environ.get('PATH')
    if path_env:
        if (platform.system().lower() != 'windows'):
            path_env = gn_path + ":" + path_env
    else:
        if (platform.system().lower() != 'windows'):
            path_env = gn_path
    if path_env:
        os.environ['PATH'] = path_env

    if not os.path.exists("/local/mnt/workspace/au_build_version.txt"):
        print("Not AU build, try to get version from build_version.txt")
        file_path = os.path.join(os.getcwd(), "build_version.txt")
        if os.path.exists(file_path):
            print("build_version.txt exited, get version from it")
            with open(file_path, "r") as file:
                build_id = file.readline()
        else:
            build_id = default_build_id
    else:
        with open("/local/mnt/workspace/au_build_version.txt", 'r') as file:
            version = file.readline().strip();
            print('au_build_version.txt:{}'.format(version))
            try:
                parts = version.split('.')
                build_id = int(parts[-1])
            except (IndexError, ValueError):
                build_id = default_build_id
            with open('build_version.txt', 'w') as f:
                f.write(str(build_id))
    print('build id: %d' % (int(build_id)))
    if os.path.exists('modules/wifi/bin/libwifi_core.a'):
        g_is_sdk_packed = True
    else:
        g_is_sdk_packed = False

def main():
    global main_options
    global g_val_board_name
    logger = logging.getLogger()
    logger.setLevel(logging.INFO)
    setup_env()
    (main_options, args) = option_parser()
    if main_options.board != None:
        g_val_board_name = main_options.board
        #print('g_val_board_name got from cmd param')
    elif ENV_BOARD_NAME in os.environ:
        g_val_board_name = os.environ[ENV_BOARD_NAME]
        #print('g_val_board_name got from ENV_BOARD_NAME')
    else:
        g_val_board_name = DEFAULT_BOARD_NAME
        #print('g_val_board_name got from DEFAULT_BOARD_NAME')
    os.environ[ENV_BOARD_NAME] = g_val_board_name
    #print('ENV_BOARD_NAME=%s, g_val_board_name=%s'%(ENV_BOARD_NAME, g_val_board_name))
    if main_options.out_dir is None:
        build_output_l = default_build_output
    else:
        build_output_l = main_options.out_dir
    if not main_options.build_all:
        if main_options.image is None:
            print("Error: image not provided!")
            print(image_list)
            sys.exit(-1)
        if main_options.image not in image_list:
            print("Error: image not in list:")
            print(image_list)
            sys.exit(-1)
        start_build(main_options.image, build_output_l)
    else:
        for image in image_list:
            start_build(image, build_output_l)

if __name__ == "__main__":
    main()

