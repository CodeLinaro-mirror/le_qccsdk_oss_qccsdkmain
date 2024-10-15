#!/usr/bin/env python3
#===============================================================================
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#===============================================================================
import os
from pack import Pack, PackOpCopyFile, PackOpCopyFolder, PackOpCopySDKFolder

def pack_sdk(build_root_path, ignore_errors=False):
    '''
    Populate the SDK folder.

    :param build_root:    Root of the build. This is the root folder for
                          source paths, relative to the current location.
    :param ignore_errors: Indicates if errors should print warnings instead
                          of raising exceptions.
    '''
    # Initialize the Pack object.
    sdk_paths = {
        'sdk': os.path.normpath(os.path.join(build_root_path, '.', 'SRC-IOE-SDK/qccsdk')),
    }

    # Pack.
    sdk_pack = Pack(build_root_path=build_root_path, sdk_root_path=sdk_paths['sdk'], ignore_errors=ignore_errors, message_prefix='[Pack QCCSDK]')
    sdk_pack_list = []
    sdk_pack_list.append(PackOpCopyFolder(source_path='../qccsdk', dest_path='../qccsdk',include_subfolders=True, folder_exclusion_list=['output']))
    sdk_pack_list.append(PackOpCopyFolder(source_path='../comp', dest_path='../comp',include_subfolders=True,folder_exclusion_list=['wifi']))

    #binary for qcc730v2_evb11_hostless
    sdk_pack_list.append(PackOpCopyFile(source_path='output/wifi_lib/FERMION_WIFI_LIB/DEBUG/lib/libwifi_core.a', dest_path='modules/wifi/bin/libwifi_core.a'))

    sdk_pack.pack(pack_list=sdk_pack_list)

    #record version
    build_version = os.getenv("CRM_BUILDID")
    if build_version == None:
        print('Not CRM build')
    else:
        print('CRM build {}'.format(build_version))
        file_path = os.path.join("./SRC-IOE-SDK/qccsdk", "build_version.txt")
        with open(file_path, "w") as file:
            file.write(build_version)
            print('write build version {} to build_version.txt'.format(build_version))


def main():
    import argparse

    ## Parse the command line arguments.
    parser = argparse.ArgumentParser(description='Packs the QCCSDK.')
    parser.add_argument('-r', '--build-root', default='.', help='Path for the build root.')
    parser.add_argument('-i', '--ignore-errors', default=False, action='store_true', help='Suppress errors')
    parser.add_argument('-I', '--no-ignore-errors', dest='ignore_errors', action='store_false', help='Fail if there is an error (default)')
    args = parser.parse_args()

    print('Packing...')
    pack_sdk(args.build_root, ignore_errors=args.ignore_errors)
    print('Packing complete.')

if __name__ == "__main__":
    main()

