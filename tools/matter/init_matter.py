#===============================================================================
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#===============================================================================

import os
import subprocess
import sys

cur_dir = os.getcwd()
script_path = os.path.dirname(os.path.abspath(__file__))
project_root = os.path.join(script_path, '../../')
matter_root = os.path.join(project_root, '../comp/matter')

submodule_path = [
    'third_party/pigweed/repo',
    'third_party/nlassert/repo',
    'third_party/nlio/repo',
]

def main():
    if not os.path.exists(matter_root):
        print("Error: Matter SDK doesn't exist")
        print("Please clone Matter SDK before building Matter demo")
        sys.exit(-1)

    os.chdir(matter_root)

    file = os.path.join(matter_root, 'build_overrides', 'pigweed_environment.gni')
    with open(file, 'w') as f:
        pass
        
    for path in submodule_path:
        file = os.path.join(matter_root, path, '.git')
        if not os.path.exists(file):
            try:
                subprocess.run(['git', 'submodule', 'update', '--init', path], check=True)
            except Exception:
                print("submodule update failed")

    os.chdir(cur_dir)

if __name__ == "__main__":
    main()
