#!/usr/bin/env python3
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
    sdk_pack_list.append(PackOpCopyFile(source_path='notice.txt', dest_path='notice.txt'))
    sdk_pack_list.append(PackOpCopyFile(source_path='Kconfig', dest_path='Kconfig'))
    sdk_pack_list.append(PackOpCopyFile(source_path='qccsdk.py', dest_path='qccsdk.py'))
    sdk_pack_list.append(PackOpCopyFolder(source_path='arch', dest_path='arch',include_subfolders=True, file_exclusion_list=['README_DEVICE.txt']))
    sdk_pack_list.append(PackOpCopyFolder(source_path='boards', dest_path='boards',include_subfolders=True))
    sdk_pack_list.append(PackOpCopyFolder(source_path='build', dest_path='build',include_subfolders=True,folder_exclusion_list=['FERMION_QCLI_DEMO', 'pre_built_binaries', '.settings', 'FERMION', 'FERMION_FTM', 'FERMION_PBL', 'FERMION_SBL', 'FERMION_HELLO_WORLD', 'FERMION_POSIX_DEMO', 'FERMION_NVM_PROGRAMMER']))
    sdk_pack_list.append(PackOpCopyFolder(source_path='bootloader/SBL', dest_path='bootloader/SBL',include_subfolders=True, file_exclusion_list=['src/pbl_patch.c']))
    sdk_pack_list.append(PackOpCopyFolder(source_path='config', dest_path='config',include_subfolders=True))
    sdk_pack_list.append(PackOpCopyFolder(source_path='demo', dest_path='demo',include_subfolders=True))
    sdk_pack_list.append(PackOpCopyFolder(source_path='drivers', dest_path='drivers',include_subfolders=True))
    sdk_pack_list.append(PackOpCopyFolder(source_path='hal', dest_path='hal',include_subfolders=True, file_exclusion_list=['makefile.mk']))
    sdk_pack_list.append(PackOpCopyFolder(source_path='include', dest_path='include',include_subfolders=True))
    sdk_pack_list.append(PackOpCopyFolder(source_path='kernel', dest_path='kernel',include_subfolders=True))
    sdk_pack_list.append(PackOpCopyFolder(source_path='lib', dest_path='lib',include_subfolders=True, file_exclusion_list=['makefile.mk']))
    sdk_pack_list.append(PackOpCopyFolder(source_path='modules', dest_path='modules',include_subfolders=True,folder_exclusion_list=['wifi']))
    sdk_pack_list.append(PackOpCopyFolder(source_path='modules/wifi', dest_path='modules/wifi/inc',include_subfolders=True, file_exclusion_list=['BUILD.gn'], extension_inclusion_list=['.h'], flatten_subfolders=True))
    sdk_pack_list.append(PackOpCopyFolder(source_path='modules/core/qcc_wifi', dest_path='modules/core/qcc_wifi',include_subfolders=True))
    sdk_pack_list.append(PackOpCopyFolder(source_path='modules/wifi/config_ini', dest_path='modules/wifi/config_ini',include_subfolders=True))
    sdk_pack_list.append(PackOpCopyFolder(source_path='os', dest_path='os',include_subfolders=True,folder_exclusion_list=['demos', 'abstractions', 'c_sdk','freertos_plus','pkcs11', 'ports','aws_demos', 'Neutrino', 'uart_cli'], file_exclusion_list=['Osal.h', 'Timer.h', '.travis.yml', 'README_DEVICE.txt']))
    sdk_pack_list.append(PackOpCopyFolder(source_path='soc', dest_path='soc',include_subfolders=True))
    sdk_pack_list.append(PackOpCopyFolder(source_path='subsys', dest_path='subsys',include_subfolders=True))
    sdk_pack_list.append(PackOpCopyFolder(source_path='tools', dest_path='tools',include_subfolders=True,folder_exclusion_list=['pack','nvm_programmer','test_tlv20', 'simv', 'build_infrastructure', 'Performance_tools', 'test_infrastructure', 'trace32','tcp_ip_agent_app','arudino', 'FermionApp', 'FermionFlasher', 'FermWinApp', 'QcmbrSPI','software-update','TestTunnelBridge', 'WFA-sigma', 'rcli-host', 'fdi_tool', ' __pycache__', ' pack', 'storage'], file_exclusion_list=['.launches.zip', 'neutrino_recovery.zip']))
    sdk_pack_list.append(PackOpCopyFolder(source_path='tools/nvm_programmer', dest_path='tools/nvm_programmer',include_subfolders=True))
    sdk_pack_list.append(PackOpCopyFolder(source_path='sectools', dest_path='sectools',include_subfolders=True))
    sdk_pack_list.append(PackOpCopyFolder(source_path='../comp', dest_path='../comp',include_subfolders=True,folder_exclusion_list=['wifi']))
    sdk_pack_list.append(PackOpCopyFolder(source_path='../release', dest_path='../release',include_subfolders=True))

    #binary for qcc730v2_evb11_hostless
    sdk_pack_list.append(PackOpCopyFile(source_path='output/wifi_lib/FERMION_WIFI_LIB/DEBUG/lib/libwifi_core.a', dest_path='modules/wifi/bin/libwifi_core.a'))
    sdk_pack_list.append(PackOpCopyFile(source_path='output/qcc730v2_evb11_hostless/FERMION_FTM/DEBUG/bin/FERMION_FTM.bin', dest_path='modules/wifi/qcc730/core/bin/ftm/qcc730v2_evb11_hostless/FERMION_FTM.bin'))
    sdk_pack_list.append(PackOpCopyFile(source_path='output/qcc730v2_evb11_hostless/FERMION_FTM/DEBUG/bin/FERMION_FTM_STRIPPED.elf', dest_path='modules/wifi/qcc730/core/bin/ftm/qcc730v2_evb11_hostless/FERMION_FTM_STRIPPED.elf'))

    #binary for qcc730v2_evb13_hostless
    sdk_pack_list.append(PackOpCopyFile(source_path='output/wifi_lib/FERMION_WIFI_LIB/DEBUG/lib/libwifi_core.a', dest_path='modules/wifi/qcc730/core/lib/qcc730v2_evb13_hostless/libwifi_core.a'))
    sdk_pack_list.append(PackOpCopyFile(source_path='output/qcc730v2_evb13_hostless/FERMION_FTM/DEBUG/bin/FERMION_FTM.bin', dest_path='modules/wifi/qcc730/core/bin/ftm/qcc730v2_evb13_hostless/FERMION_FTM.bin'))
    sdk_pack_list.append(PackOpCopyFile(source_path='output/qcc730v2_evb13_hostless/FERMION_FTM/DEBUG/bin/FERMION_FTM_STRIPPED.elf', dest_path='modules/wifi/qcc730/core/bin/ftm/qcc730v2_evb13_hostless/FERMION_FTM_STRIPPED.elf'))

    #bin
    sdk_pack_list.append(PackOpCopyFile(source_path='modules/wifi/bin/regdb.bin', dest_path='modules/wifi/bin/regdb.bin'))
    sdk_pack_list.append(PackOpCopyFile(source_path='modules/wifi/bin/bdwlan01.bin', dest_path='modules/wifi/bin/bdwlan01.bin'))
    sdk_pack_list.append(PackOpCopyFile(source_path='modules/wifi/bin/bdwlan03.bin', dest_path='modules/wifi/bin/bdwlan03.bin'))


    #core/wifi Kconfig files
    sdk_pack_list.append(PackOpCopyFile(source_path='modules/Kconfig', dest_path='modules/Kconfig'))
    sdk_pack_list.append(PackOpCopyFile(source_path='modules/wifi/Kconfig', dest_path='modules/wifi/Kconfig'))
    sdk_pack_list.append(PackOpCopyFile(source_path='modules/wifi/Kconfig.lib', dest_path='modules/wifi/Kconfig.lib'))
    sdk_pack_list.append(PackOpCopyFile(source_path='modules/wifi/Kconfig.rtt', dest_path='modules/wifi/Kconfig.rtt'))
    sdk_pack_list.append(PackOpCopyFile(source_path='modules/wifi/Kconfig.wapp', dest_path='modules/wifi/Kconfig.wapp'))
    sdk_pack_list.append(PackOpCopyFile(source_path='modules/core/system/Kconfig', dest_path='modules/core/system/Kconfig'))
    sdk_pack_list.append(PackOpCopyFile(source_path='modules/core/system/sys_src/Kconfig', dest_path='modules/core/system/sys_src/Kconfig'))

    #gn files
    sdk_pack_list.append(PackOpCopyFile(source_path='BUILD.gn', dest_path='BUILD.gn'))
    sdk_pack_list.append(PackOpCopyFile(source_path='.gn', dest_path='.gn'))
    sdk_pack_list.append(PackOpCopyFile(source_path='build.py', dest_path='build.py'))
    sdk_pack_list.append(PackOpCopyFile(source_path='configs_decl.gni', dest_path='configs_decl.gni'))

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

