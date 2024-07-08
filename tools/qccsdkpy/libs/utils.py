#===============================================================================
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#===============================================================================

import os
import sys
import subprocess
import io
import re
import shutil
import time

import traceback
import yaml

import cfg_common as CfgCommon

from mylogger import Logger

class Utils(object):
    def __init__(self, project_root, qccsdkpy_dir, enableLogDebug=False):
        self.lineMaxLength = 1024
        self.BITS32MAX = 0xffffffff
        self.CfgCommon = CfgCommon
        self.project_root = project_root
        self.qccsdkpy_dir = qccsdkpy_dir
        self.cfg_interal_path = os.path.join(qccsdkpy_dir, CfgCommon.CFG_INTERNAL)
        self.CfgInternal = self.load_cfg(self.cfg_interal_path)
        qccsdk_base = self.CfgInternal['qccsdk_base']
        if not os.path.isfile(os.path.join(project_root, qccsdk_base['check_path'])):
            print('%s not exist, not a repo or SDK'%os.path.join(project_root, qccsdk_base['check_path']))
            sys.exit(1)
        if os.path.isfile(os.path.join(project_root, qccsdk_base['release_sdk_check_path'])):
            self.is_SDK = True
            self.appdir_maps = self.CfgInternal['appdir_map']['SDK']
            self.board_maps = self.CfgInternal['board_map']['SDK']
            self.regdb_maps = self.CfgInternal['regdb_map']['SDK']
            self.fdt_maps = self.CfgInternal['fdt_map']['SDK']
        else:
            self.is_SDK = False
            self.appdir_maps = self.CfgInternal['appdir_map']['repo']
            self.board_maps = self.CfgInternal['board_map']['repo']
            self.regdb_maps = self.CfgInternal['regdb_map']['repo']
            self.fdt_maps = self.CfgInternal['fdt_map']['repo']
        self.cfg_extenral_path = os.path.join(qccsdkpy_dir, self.CfgInternal['cfg_external'])
        self.sample_cfg_external_path = os.path.join(qccsdkpy_dir, self.CfgInternal['sample_cfg_external'])
        if not os.path.isfile(self.cfg_extenral_path):
            if not os.path.isfile(self.sample_cfg_external_path):
                print('%s not exist'%self.sample_cfg_external_path)
                sys.exit(1)
            fsrc = open(self.sample_cfg_external_path,'r')
            fdst = open(self.cfg_extenral_path,'w')
            shutil.copyfileobj(fsrc, fdst)
            fsrc.close()
            fdst.close()
        self.CfgExternal = self.load_cfg(self.cfg_extenral_path)
        qccsdk = self.CfgExternal['qccsdk']
        self.output_path = os.path.join(project_root, qccsdk['output'])
        self.log_path = os.path.join(self.output_path, qccsdk_base['log_dir'])
        self.build_path = os.path.join(self.output_path, qccsdk['board'])
        logger_set = self.CfgInternal['utils']['logger']
        self.logger = Logger(logdir=self.log_path, stream2file=logger_set['stream2file'], logging2file=logger_set['logging2file'],
            filename_timestamp=logger_set['filename_timestamp'], file_append=logger_set['file_append'], logging_timestamp=logger_set['logging_timestamp'])
        if enableLogDebug==True:
            self.logger.enableLogDebug()
        self.target_appdir_in_flash = self.appdir_in_flash(qccsdk['appdir'])

    def load_cfg(self, config_file):
        if os.path.isfile(config_file)!=True:
            print('%s not exist'%config_file)
            sys.exit(1)
        try:
            with open(config_file) as f:
                #print('%s opened'%config_file)
                config = yaml.load(f, Loader=yaml.FullLoader)
                #print(config)
        except:
            print("Failed to open %s"%config_file)
            sys.exit(1)
        return config

    def is_file_writable(self, path):
        if os.access(path, os.F_OK) and os.access(path, os.W_OK):
            return True
        else:
            return False

    def is_cfg_external_writable(self):
        return self.is_file_writable(self.cfg_extenral_path)

    def cfg_external_write(self, pattern, name, value):
        if self.CfgExternal['qccsdk'][name] == value:
            self.logger.warning('CfgExternal[%s]==%s, skip'%(name, value))
            return
        self.logger.debug('set %s from %s to %s'%(name, self.CfgExternal['qccsdk'][name], value))
        with open(self.cfg_extenral_path, mode='r') as f:
            data = f.read()
            #data = data.replace(pattern, value)
            data = re.sub(pattern+'.*', pattern+value, data)
        with open(self.cfg_extenral_path, mode='w') as f:
            f.write(data)
        self.CfgExternal = self.load_cfg(self.cfg_extenral_path)

    def cur_time(self):
        return time.strftime("[%Y-%m-%d %H:%M:%S]", time.localtime())

    def ENTER(self, strarg=''):
        self.logger.debug('%s %s Started. %s'%(self.cur_time(), os.path.basename(sys.argv[0]), strarg))

    def FAIL(self, strarg=''):
        self.logger.info('%s %s Failed. %s'%(self.cur_time(), os.path.basename(sys.argv[0]), strarg))
        sys.exit(1)

    def SUCCESS(self, strarg=''):
        self.logger.debug('%s %s Succeeded. %s'%(self.cur_time(), os.path.basename(sys.argv[0]), strarg))
        sys.exit(0)

    def warn_not_supported(self, target):
        self.logger.warning('%s is not supported'%target)

    def run_cmd(self, cmd_string, enable=True, check=True, logEnable=True, logBlockMode=True, waitFinish=True):
        ret = False
        readBuff = ''
        call_ret = 0
        try:
            self.logger.debug("Execute cmd as below with check=%s enable=%s logEnable=%s logBlockMode=%s waitFinish=%s:"%(str(check), str(enable), str(logEnable), str(logBlockMode), str(waitFinish)));
            self.logger.info('%s'%cmd_string)
            if (self.CfgInternal['utils']['run_cmd']['run_enable']==True) and (enable==True):
                self.logger.info('+++++++++++++++++++++++++++++++++++++++++++')
                self.logger.reduceFormater()
                if logEnable == True:
                    self.logger.debug('waitFinish=%s'%str(waitFinish))
                    if waitFinish==True:
                        self.logger.debug('run')
                        with subprocess.Popen(cmd_string,shell=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT) as subp:
                            if (waitFinish==True):
                                if logBlockMode==False:
                                    iohandle = io.open(subp.stdout.fileno(), 'rb', closefd=False)
                                while True:
                                    while True:
                                        if logBlockMode == True:
                                            output = subp.stdout.readline().decode(errors='ignore')
                                            readBuff += output
                                            if output=='':
                                                break
                                        else:
                                            output = iohandle.read1(self.lineMaxLength).decode(errors='ignore')
                                            readBuff += output
                                            if len(output) == 0:
                                                break
                                        self.logger.info(output)
                                    call_ret = subp.poll()
                                    if (call_ret != None):
                                        break
                                if logBlockMode==False:
                                    iohandle.close()
                    else:
                        subprocess.Popen(cmd_string,shell=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
                        self.logger.debug('not wait, start and skip')
                else:
                    call_ret = subprocess.call(cmd_string,shell=True)
                self.logger.restoreFormater()
                self.logger.info('-------------------------------------------')
            else:
                self.logger.info('dummy')
        except Exception as e:
            self.logger.info('str(Exception):\t' + str(Exception))
            self.logger.info('str(e):\t\t' + str(e))
            self.logger.info('repr(e):\t' + repr(e))
            self.logger.info('traceback.print_exc():\n%s' % traceback.print_exc())
            self.logger.info('traceback.format_exc():\n%s' % traceback.format_exc())
            self.logger.fail(cmd_string+' exception')
        else:
            if  call_ret != 0:
                result_str = cmd_string+' ret=%d'%call_ret
                if check == True:
                    self.logger.fail(result_str)
                else:
                    self.logger.log.warning(result_str)
            else :
                ret = True
        return (ret, readBuff)

    def get_SDK_by_path(self, sdk_check_path):
        # Determine location of Software Development Kit (SDK)
        SDK = os.getenv("SDK")
        if not SDK:
            # Walk up directories from the current working directory
            curr_dir = os.getcwd()
            self.logger.debug("Initial cwd is " + curr_dir)
            last_dir=""
            while curr_dir != last_dir:
                self.logger.debug("Check for SDK top-level: " + curr_dir)
                tmp_path = os.path.join(curr_dir, sdk_check_path)
                if os.path.isdir(os.path.join(curr_dir, sdk_check_path)):
                    SDK = curr_dir
                    break
                else:
                    self.logger.debug("invalid path: " + tmp_path)
                last_dir = curr_dir
                curr_dir = os.path.dirname(curr_dir)
            if not SDK:
                self.logger.fail("Cannot locate Software Development Kit");
        self.logger.debug("SDK is " + SDK)
        return SDK

    def str2int(self, strs):
        #self.logger.debug(strs)
        strs = strs.strip()
        #self.logger.debug(strs)
        if re.search('^0(x|X)', strs):
            return int(strs, 16)
        else:
            return int(strs)

    def str2intArray(self, strs, split_char=' '):
        #self.logger.debug(strs)
        strArray = strs.split(split_char)
        intArray = []
        for x in strArray:
            #self.logger.debug(x)
            #self.logger.debug(type(x))
            if x=='':
                self.logger.debug('empty str, skip')
                continue
            intArray.append(self.str2int(x))
        return intArray

    def str2bool(self, str):
        if str=='True':
            return True
        elif str=='False':
            return False
        else:
            self.logger.fail('str2bool not support str=%s'%str)

    def ipv4_hex2str(self, ipv4_hex):
        ipv4_str = '%d.%d.%d.%d'%(ipv4_hex&0xff, (ipv4_hex>>8)&0xff, (ipv4_hex>>16)&0xff, (ipv4_hex>>24)&0xff)
        return ipv4_str

    def qassert(self, expr, failMsg=None):
        if expr == False:
            self.logger.fail("qassert failed. " + failMsg)

    def chdir (self, path, printdir=True):
        if os.path.isdir(path)==False:
            self.logger.fail("path=%s is not a valid dir"%path)
        if printdir==True: self.logger.info('cd %s'%path)
        os.chdir(path)

    def copyfile(self, src, dst, printpath=True, create_dir=True, gen_writable=False):
        if os.path.isfile(src) == False:
            self.logger.fail("src=%s is not a valid file"%src)
        if printpath==True:
            self.logger.info('cp %s to %s'%(src, dst))
        if create_dir==True:
            self.make_dir(os.path.dirname(dst))
        if gen_writable==True:
            fsrc = open(src,'r')
            fdst = open(dst,'w')
            shutil.copyfileobj(fsrc, fdst)
            fsrc.close()
            fdst.close()
        else:
            shutil.copyfile(src, dst)

    def copyfile_todir(self, src, dstdir, printpath=True, gen_writable=False):
        filename = os.path.basename(src)
        dst = os.path.join(dstdir, filename)
        self.copyfile(src, dst, printpath, gen_writable)

    def copyfile_samedir(self, src, dst_filename, printpath=True, gen_writable=False):
        dst_dir = os.path.dirname(src)
        dst = os.path.join(dst_dir, dst_filename)
        self.copyfile(src, dst, printpath, gen_writable)

    def rmtree(self, dirpath):
        if os.path.isdir(dirpath):
            self.logger.warning('rmtree dir=%s'%dirpath)
            shutil.rmtree(dirpath)

    def copytree(self, src_dir, dst_dir, del_if_exist=False):
        src_dir_name = os.path.basename(src_dir)
        dst = os.path.join(dst_dir, src_dir_name)
        if del_if_exist==True:
            self.rmtree(dst)
        shutil.copytree(src_dir, dst)

    def fremove(self, filepath):
        if os.path.isfile(filepath):
            self.logger.warning('remove file=%s'%filepath)
            os.remove(filepath)

    def make_dir(self, dirpath, del_if_exist=False):
        make_dirs = True
        if os.path.isdir(dirpath)==True:
            if del_if_exist==True:
                self.rmtree(dirpath)
            else:
                make_dirs = False
        if make_dirs==True:
            os.makedirs(dirpath)

    def get_masks(self, bits_offset, bits_width):
        return (((1<<bits_width) - 1)<<bits_offset)

    def get_mask(self, bit_offset):
        return self.get_masks(bit_offset, 1)

    def get_bits(self, val, bits_offset, bits_width):
        return (val>>bits_offset) & self.get_masks(0, bits_width)

    def get_bit(self, val, bit_offset):
        return self.get_bits(val, bit_offset, 1)

    def clear_bits(self, val, bits_offset, bits_width):
        masks = self.get_masks(bits_offset, bits_width)
        not_masks = ((~masks) & self.BITS32MAX)
        new_val = (val & not_masks)
        return new_val

    def clear_bit(self, val, bit_offset):
        return self.clear_bits(val, bit_offset, 1)

    def set_bits(self, val, bits_offset, bits_width, fieldVal):
        new_val = self.clear_bits(val, bits_offset, bits_width)
        masks = self.get_masks(bits_offset, bits_width)
        new_fieldVal = ((fieldVal << bits_offset) & masks)
        final_val = (new_val | new_fieldVal)
        return final_val

    def setenv_safe (self, key, val):
        if os.getenv(key) == None: os.environ[key]=val

    def setenv_force (self, key, val):
        os.environ[key]=val

    def getenv (self, key):
        if os.getenv(key) == None: return None
        return os.environ[key]

    def python_script_op(self, script_dir=None, script='', cmd=''):
        if script_dir:
            cwd = os.getcwd()
            self.chdir(script_dir)
        if 'env' in self.CfgExternal.keys():
            python_env = self.CfgExternal['env']['python_env']
        else:
            python_env = 'python' #to keep backward compatibility
        (ret, readBuff) = self.run_cmd('%s %s %s'%(python_env, script, cmd))
        if script_dir:
            self.chdir(cwd)
        return (ret, readBuff)

    def python_script_op_local(self, script='', cmd=''):
        ret = False
        readBuff = ''
        script_found = False
        for name in os.listdir(self.qccsdkpy_dir):
            if (name==script):
                script_found = True
                (ret, readBuff) = self.python_script_op(self.qccsdkpy_dir, name, cmd)
        if script_found==False:
            self.logger.warning('%s not found in %s'%(script, self.qccsdkpy_dir))
        return (ret, False)

    def win_kill_exe (self, exename):
        self.logger.debug('win_kill_exe %s'%exename)
        killed = False
        command_result = os.popen('tasklist').readlines()
        for line in command_result:
            ma = re.match('(.*).exe(\s*)(\d*)',str(line),re.I)
            if ma == None:
                continue
            #self.logger.debug('found exe=%s'%ma.group(1))
            if ma.group(1) == exename:
                killcmd = 'taskkill -f -pid '+ str(ma.group(3))
                os.popen(killcmd)
                self.logger.info('killed %s pid=%s'%(exename, ma.group(3)))
                killed = True
        if killed==False:
            self.logger.warning('failed to kill %s since not found'%exename)

    def pscp_up_to_linux(self, sshpwd, sshusername, sship, src, sshdst):
        self.runCmd('pscp -pw %s %s %s@%s:%s'%(sshpwd, src, sshusername, sship, sshdst))

    def pscp_down_from_linux(self, sshpwd, sshusername, sship, sshsrc, dst):
        self.make_dir(os.path.dirname(dst))
        self.runCmd('pscp -pw %s %s@%s:%s %s'%(sshpwd, sshusername, sship, sshsrc, dst))

    def list_remove_all_item(self, a_list, a_item):
        while a_item in a_list:
            a_list.remove(a_item)

    def get_nvm_offset(self, nvm_entry_name):
        nvm_offset = self.CfgInternal['nvm_offset']
        if nvm_entry_name in nvm_offset.keys():
            return nvm_offset[nvm_entry_name]
        else:
            return None

    def appdir_2_image_name (self, appdir):
        if appdir in self.CfgInternal['appdir_map'].keys():
            return self.CfgInternal['appdir_map'][appdir][1]
        else:
            return None

    def appdir_2_nvm_entry_name (self, appdir):
        if appdir in self.CfgInternal['appdir_map'].keys():
            appdir_nvm_entry_name = self.CfgInternal['appdir_map'][appdir][0]
            if (appdir_nvm_entry_name!=None) and ('flash_' in appdir_nvm_entry_name) and self.CfgExternal['qccsdk']['board'] in ['qcc730v2_socket', 'qcc730v2_evb12_hostless']:
                #todo: no flash, then just use rram
                appdir_nvm_entry_name = 'rram_app'
            return appdir_nvm_entry_name
        else:
            return None

    def appdir_in_flash (self, appdir):
        appdir_nvm_entry_name = self.appdir_2_nvm_entry_name(appdir)
        if appdir_nvm_entry_name==None:
            return False
        if ('flash_' in appdir_nvm_entry_name):
            return True
        elif ('rram_' in appdir_nvm_entry_name):
            return False
        else:
            self.logger.warning('appdir=%s appdir_nvm_entry_name=%s are not supported for image Flash'%(appdir, appdir_nvm_entry_name))
            return False

    def appdir_2_image_path (self, appdir):
        image_name = self.appdir_2_image_name(appdir)
        if image_name==None:
            return None
        elif appdir=='intg':
            return None
        else:
            dir_path = os.path.join(self.build_path, image_name, 'DEBUG', 'bin')
            if appdir=='sbl' or self.appdir_in_flash(appdir):
                #generally, elf locates in Flash, bin locates in rram, except sbl as elf in rram
                return os.path.join(dir_path, '%s_HASHED.elf'%image_name)
            elif appdir=='prg':
                # nvm programmer as elf in ram
                return os.path.join(dir_path, '%s.elf'%image_name)
            else:
                return os.path.join(dir_path, '%s.bin'%image_name)

    def get_bdf_info (self):
        board = self.CfgExternal['qccsdk']['board']
        bdf_path = os.path.join(self.project_root, self.board_maps['bdf_dir'], self.board_maps[board])
        bdf_RRAM_offset = self.get_nvm_offset(self.CfgInternal['board_map']['fdt_name'])
        return [bdf_RRAM_offset, bdf_path]

    def get_regdb_info (self):
        regdb_path = os.path.join(self.project_root, self.regdb_maps['regdb_path'])
        regdb_RRAM_offset = self.get_nvm_offset(self.CfgInternal['regdb_map']['fdt_name'])
        return [regdb_RRAM_offset, regdb_path]

    def get_fdt_info (self):
        if (self.target_appdir_in_flash==True):
            fdt_path = os.path.join(self.project_root, self.fdt_maps['fdt_path_app_elf'])
        else:
            fdt_path = os.path.join(self.project_root, self.fdt_maps['fdt_path_app_bin'])
        fdt_offset = self.get_nvm_offset(self.CfgInternal['fdt_map']['nvm_entry_name'])
        return [fdt_offset, fdt_path]

    def assert_board_supported (self, board):
        self.qassert(board in self.board_maps['boards_supported'], '%s as board is not supported in %s'%(board, self.board_maps['boards_supported']))

    def assert_appdir_build_supported (self, appdir):
        self.qassert(appdir in self.appdir_maps['appdirs_build_supported'], '%s as build appdir is not supported in %s'%(appdir, self.appdir_maps['appdirs_build_supported']))

    def assert_appdir_flash_supported (self, appdir):
        self.qassert(appdir in self.appdir_maps['appdirs_flash_supported'], '%s as flash appdir is not supported in %s'%(appdir, self.appdir_maps['appdirs_flash_supported']))

    def assert_appdir_supported (self, appdir):
        self.qassert((appdir in self.appdir_maps['appdirs_build_supported']) or (appdir in (self.appdir_maps['appdirs_flash_supported'])),
            '%s as appdir is not supported in either [%s] or [%s]'%(appdir, self.appdir_maps['appdirs_build_supported'], self.appdir_maps['appdirs_flash_supported']))

    def assert_jtag_supported (self, jtag):
        self.qassert(jtag in self.CfgInternal['flash']['jtags_supported'], '%s as jtag interface is not supported in %s'%(jtag, self.CfgInternal['flash']['jtags_supported']))
