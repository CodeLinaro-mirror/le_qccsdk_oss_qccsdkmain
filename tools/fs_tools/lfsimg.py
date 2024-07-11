#Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
#SPDX-License-Identifier: BSD-3-Clause-Clear

import os
import argparse
import subprocess
from littlefs import LittleFS

def add_files_to_littlefs(fs, directory):
  # Check the operating system
  if os.name == 'nt':  # Windows
    separator = '/'  # Use forward slash for LittleFS
  else:
    separator = os.path.sep  # Use system-specific separator otherwise

  for root, dirs, files in os.walk(directory):
    # Loop through subdirectories first
    for subdir in dirs:
      # Construct subdirectory path within LittleFS
      subdirectory_path = os.path.join(root[len(directory):], subdir).replace("\\", separator)
      # Create the subdirectory in LittleFS
      fs.mkdir(subdirectory_path)
    
    for file_name in files:
      file_path = os.path.join(root, file_name)
      with open(file_path, 'rb') as file:
        content = file.read()
        # Construct the file path within LittleFS
        littlefs_file_path = os.path.join(root[len(directory):], file_name).replace("\\", separator)
        with fs.open(littlefs_file_path, 'wb') as fh:
          fh.write(content)




def create_lfs_image(source_path, image_file='lfsimg.bin', block_size=4096, block_count=16, verbose=True):
    if not os.path.exists(source_path):
        print(f"Error: Source path '{source_path}' does not exist.")
        return

    if os.path.exists(image_file):
        print(f"Warning: Image file '{image_file}' already exists and will be overwritten.")

    fs = LittleFS(block_size=block_size, block_count=block_count)
    add_files_to_littlefs(fs, source_path)
    
    with open(image_file, 'wb') as fh:
        fh.write(fs.context.buffer)

    print(f"LittleFS image created successfully: {image_file}")


def list_lfs_image(image_file, block_size=4096, block_count=16, verbose=True):
    if not os.path.exists(image_file):
        print(f"Error: Image file '{image_file}' does not exist.")
        return

    block_count=int(os.path.getsize(image_file)/block_size)
    cmd = ['littlefs_list', '-b', str(block_size), '-c', str(block_count), '-i', image_file]

    try:
        subprocess.run(cmd, check=True)
    except subprocess.CalledProcessError as e:
        print(f"Error: Failed to list LittleFS image: {e}")

def is_lfs_image_file(file_path):
    try:
        with open(file_path, 'rb') as file:
            # LittleFS image files typically start with a specific magic number
            file.seek(4)
            magic_number = file.read(12)
            #if magic_number == b'\xF7\xF0\xF7\xF0'[::-1]:  # Little-endian magic number
            #if magic_number == b'\x04\x00\x00\x00\xF0\x0F\xFF\xF7':
            if magic_number == b'\xF0\x0F\xFF\xF7\x6C\x69\x74\x74\x6C\x65\x66\x73':  # Little-endian magic number
                #print("YES")
                return True
            else:
                #print("NOT")
                return False
    except IOError:
        # Unable to open the file
        print("Error: Unable to open or read the file")
        return False

def dump_first_16_bytes(file_path):
    try:
        with open(file_path, 'rb') as file:
            # Read the first 16 bytes
            first_16_bytes = file.read(16)
            # Convert bytes to hexadecimal representation
            hex_dump = ' '.join([format(byte, '02x') for byte in first_16_bytes])
            print(hex_dump)
    except IOError:
        print("Error: Unable to open or read the file")
    except Exception as e:
        print(f"Error: An unexpected error occurred: {e}")

# Call the function with the file path
#dump_first_16_bytes('lfsimg.bin')




if __name__ == "__main__":    
    parser = argparse.ArgumentParser(description='Tool to generate LFS images from a source folder or list files in a LittleFS image')
    parser.add_argument('-s', '--source', type=str,
                    help='''Source path.
                    If provided, create a LittleFS image from the source folder.
                    If not provided, the image file(-f) must be provided for parsing.'''
                    )
    parser.add_argument('-f', '--file', type=str, help="Image file name. If creating an image and not provided, defaults to 'lfsimg.bin'.")
    parser.add_argument('--img_size', type=int, default=65536, help='Size of the LFS image (defaults to 65536(64K))')
    parser.add_argument('--block_size', type=int, default=4096, help='Block size of the LFS image (defaults to 4096)')
    
    #parser.add_argument('-v', '--verbose', action='store_true', help='Verbose')

    args = parser.parse_args()
    block_count = int(args.img_size/args.block_size)
    
    if args.source:
        if args.file is None:
            args.file = 'lfsimg.bin'
        create_lfs_image(args.source, args.file, args.block_size, block_count)
    elif args.file:
        #dump_first_16_bytes(args.file)
        if is_lfs_image_file(args.file):          
            list_lfs_image(args.file, args.block_size, block_count)
        else:
            print(f"Error: {args.file} is not valid LittleFS image.")
    else:
        parser.print_help()
