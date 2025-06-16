#===============================================================================
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#===============================================================================

import logging
import os
import shutil
import subprocess
import sys 

cur_dir = os.getcwd()
script_path = os.path.dirname(os.path.abspath(__file__))
project_root = os.path.join(script_path, '../../')
matter_root = os.path.join(project_root, '../comp/matter')
port_root = os.path.join(project_root, 'demo/matter_demo/port')

def set_log():
    logger = logging.getLogger()
    logger.setLevel(logging.INFO)
    streamer = logging.StreamHandler()
    logger.addHandler(streamer)

def copy_to(src, dst):
    if os.path.isdir(src):
        if os.path.exists(dst):
            shutil.rmtree(dst)
        shutil.copytree(src, dst, symlinks=True)
    elif os.path.isfile(src):
        if os.path.exists(dst):
            os.remove(dst)
        shutil.copy(src, dst)

def copy_matter_files(app_type):
    src_path = os.path.abspath(os.path.join(port_root, "build_overrides/fermion.gni"))
    dst_path = os.path.abspath(os.path.join(matter_root, "build_overrides/fermion.gni"))
    copy_to(src_path, dst_path)

    src_path = os.path.abspath(os.path.join(port_root, "examples/build_overrides/fermion.gni"))
    dst_path = os.path.abspath(os.path.join(matter_root, "examples/build_overrides/fermion.gni"))
    copy_to(src_path, dst_path)

    src_path = os.path.abspath(os.path.join(port_root, "examples", app_type, "fermion"))
    dst_path = os.path.abspath(os.path.join(matter_root, "examples", app_type, "fermion"))
    copy_to(src_path, dst_path)

    src_path = os.path.abspath(os.path.join(port_root, "src/platform/fermion"))
    dst_path = os.path.abspath(os.path.join(matter_root, "src/platform/fermion"))
    copy_to(src_path, dst_path)

def apply_patches():
    patch_path = os.path.abspath(os.path.join(port_root, "patch"))
    os.chdir(matter_root)
    cmd = "git reset --hard"
    logging.info(cmd)
    subprocess.run(cmd.split())
    for patches in os.listdir(patch_path):
        patch=os.path.join(patch_path, patches)
        cmd = "git apply " + patch
        logging.info(cmd)
        subprocess.run(cmd.split())
    os.chdir(cur_dir)

def build_matter(app_type, include_dir):
    example_root = os.path.join(matter_root, 'examples', app_type, 'fermion') 
    os.chdir(example_root)
    cmd = "gn gen output --args=\"fermion_output=\\\"{}\\\"\"".format(include_dir)
    logging.info(cmd)
    os.system(cmd)

    if (app_type == 'lighting-app'):
        cmd = "ninja -C output libLight"
    elif (app_type == 'light-switch-app'):
        cmd = "ninja -C output libSwitch"
    else:
        sys.exit(-1)
    logging.info(cmd)
    subprocess.run(cmd.split())

    os.chdir(cur_dir)
    
def main():
    set_log()
    app_type = sys.argv[1]
    include_dir = sys.argv[2]
    if app_type not in ['lighting-app', 'light-switch-app']:
        logging.info("Supported demo: lighting-app, light-switch-app")
        sys.exit(-1)

    if not os.path.exists(matter_root):
        #todo
        logging.info("Error: Matter SDK doesn't exist")
        logging.info("Please clone Matter SDK before building Matter demo")
        sys.exit(-1)
    else:
        copy_matter_files(app_type)        
        apply_patches()
        build_matter(app_type, include_dir)

if __name__ == "__main__":
    main()
