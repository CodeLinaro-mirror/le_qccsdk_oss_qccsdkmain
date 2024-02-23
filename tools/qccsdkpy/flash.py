import os
import sys
import re
import argparse
import time

cur_dir = os.getcwd()
qccsdkpy_dir = cur_dir
project_root = os.path.dirname(os.path.dirname(qccsdkpy_dir))
qccsdkpy_lib_dir = os.path.join(qccsdkpy_dir, 'libs')

sys.path.append(qccsdkpy_dir)
sys.path.append(qccsdkpy_lib_dir)

from utils import Utils

parser = argparse.ArgumentParser(description='flash', formatter_class=argparse.RawDescriptionHelpFormatter)
parser.add_argument('--flash', required=False, action='store_true', help='flash images')
parser.add_argument('--verify', required=False, action='store_true', help='flash images and verify. NOT SUPPORT YET')
parser.add_argument('--reset', required=False, action='store_true', help='reset board after flash')
parser.add_argument('--erase', required=False, action='store_true', help='erase all RRAM and Flash')
parser.add_argument('--dump', required=False, action='store_true', help='dump all RRAM and Flash. NOT SUPPORT YET')
parser.add_argument('--debug', '--verbose', '-v', required=False, action='store_true', help='enalbe output debug info on console')
args = parser.parse_args()

cUtils = Utils(project_root, qccsdkpy_dir, enableLogDebug=args.debug)
Logger = cUtils.logger
#set param should be included in qccsdk
qccsdk = cUtils.CfgExternal['qccsdk']
flash = qccsdk['flash']
exreset = qccsdk['exreset']
appdir = qccsdk['appdir']
board = qccsdk['board']
jtag = qccsdk['jtag']

cUtils.ENTER()

#by default, no parameter
if len(sys.argv)==1:
    default_action = qccsdk['flash']['default_action']
    if default_action in cUtils.CfgInternal['flash']['actions_supported']:
        if default_action=='erase':
            args.erase = True
        elif default_action=='flash':
            args.flash = True
        else:
            Logger.warn_not_supported(default_action)
            cUtils.SUCCESS('no parameter, skip')
    else:
        cUtils.SUCCESS('no parameter, skip')

cUtils.assert_appdir_flash_supported(appdir)
cUtils.assert_board_supported(board)
cUtils.assert_jtag_supported(jtag)
flash_internal = cUtils.CfgInternal['flash']

cUtils.qassert(flash['interface']=='jtag', 'flash[interface] only supoort jtag')
cUtils.qassert(flash['tool']=='python', 'flash[tool] should be python not exe')

nvm_prg_path = os.path.join(project_root, cUtils.CfgInternal['qccsdk_base']['nvm_prg_path'])
nvm_prg_dir = os.path.dirname(nvm_prg_path)
nvm_prg_name = os.path.basename(nvm_prg_path)
cmd_interface_name = flash_internal[jtag]
CMD_RRAM = 'rram'
CMD_FLASH = 'flash'
CMD_OTP = 'otp'

nvmcmd_intf =  ' %s -s %s '%(nvm_prg_name, cmd_interface_name)
Flash_cmd = nvmcmd_intf + ' --nvm-name %s '%CMD_FLASH
RRAM_cmd = nvmcmd_intf + ' --nvm-name %s '%CMD_RRAM
reset_cmd = nvmcmd_intf + ' --reset '

def burn_nvm_appdir (cur_appdir):
    app_nvm_entry_name = cUtils.appdir_2_nvm_entry_name(cur_appdir)
    image_offset = cUtils.get_nvm_offset(app_nvm_entry_name)
    image_path = cUtils.appdir_2_image_path(cur_appdir)
    if cUtils.appdir_in_flash(cur_appdir)==True:
        real_cmd = Flash_cmd
    else:
        real_cmd = RRAM_cmd
    cUtils.python_script_op(script=' %s -b 0x%x -f %s'%(real_cmd, image_offset, image_path))

def burn_sblB ():
    image_offset = cUtils.get_nvm_offset('rram_sblB')
    image_path = cUtils.appdir_2_image_path('sbl')
    cUtils.python_script_op(script=' %s -b 0x%x -f %s'%(RRAM_cmd, image_offset, image_path))

def power_flip ():
    power_py = exreset['power_py']
    dir_name = os.path.dirname(power_py)
    base_name = os.path.basename(power_py)
    cUtils.python_script_op(dir_name, base_name, '')

def power_reset ():
    #require init state is ON
    #dut power down
    power_flip()
    Logger.info('delay=%s(s) before power up'%exreset['power_down_up_delay_s'])
    time.sleep(exreset['power_down_up_delay_s'])
    #dut power up
    power_flip()

if exreset['enable']==True:
    power_reset()

cUtils.chdir(nvm_prg_dir)

if args.erase==True:
    cUtils.python_script_op(script=' %s --chip-erase'%Flash_cmd)
    cUtils.python_script_op(script=' %s --partial-erase --begin-address 0x%x --size 0x%x '
        %(RRAM_cmd, flash_internal['erase_rram'][0], flash_internal['erase_rram'][1]-flash_internal['erase_rram'][0]))

if args.flash==True:
    #burn_nvm_appdir('pbl')
    [fdt_offset, fdt_path] = cUtils.get_fdt_info()
    cUtils.python_script_op(script=' %s -b 0x%x -f %s'%(RRAM_cmd, fdt_offset, fdt_path))
    burn_nvm_appdir('sbl')
    burn_sblB()
    #[bdf_RRAM_offset, bdf_path] = cUtils.get_bdf_info()
    #[regdb_RRAM_offset, regdb_path] = cUtils.get_regdb_info()
    #cUtils.python_script_op(script=' %s -b 0x%x -f %s'%(RRAM_cmd, bdf_RRAM_offset, bdf_path))
    #cUtils.python_script_op(script=' %s -b 0x%x -f %s'%(RRAM_cmd, regdb_RRAM_offset, regdb_path))
    burn_nvm_appdir(appdir)

if (args.reset == True):
    cUtils.python_script_op(script=reset_cmd)

cUtils.chdir(cur_dir)

if exreset['enable']==True:
    power_reset()

cUtils.SUCCESS()

