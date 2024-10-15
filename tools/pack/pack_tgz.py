#===============================================================================
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#===============================================================================
import os
import tarfile
import argparse

parser = argparse.ArgumentParser(description='SDK zip', formatter_class=argparse.RawDescriptionHelpFormatter)
parser.add_argument('--src', required=False, action='store_true', help='Generate src-ioe.tar.gz')
parser.add_argument('--sdk', required=False, action='store_true', help='Generate src-ioe-sdk.tar.gz')
args = parser.parse_args()

def compress_folder(folder_name, output_filename):
  """Compress file.

  Args:
    folder_name: folder.
    output_filename: tar file.
  """
  with tarfile.open(output_filename, 'w:gz') as tar:
    for root, directories, files in os.walk(folder_name):
      for file in files:
        tar.add(os.path.join(root, file))

if args.src==True:
  folder_name = '.'
  output_filename = 'src-ioe.tar.gz'
  print(f'Start to compress folder to {output_filename}')
  compress_folder(folder_name, output_filename)
  print(f'Compressed folder to {output_filename}')

if args.sdk==True:
  folder_name = 'SRC-IOE-SDK'
  output_filename = 'src-ioe-sdk.tar.gz'
  print(f'Start to compress folder to {output_filename}')
  compress_folder(folder_name, output_filename)
  print(f'Compressed folder to {output_filename}')

