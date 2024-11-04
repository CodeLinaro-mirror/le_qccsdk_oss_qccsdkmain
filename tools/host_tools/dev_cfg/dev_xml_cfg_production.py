#!/usr/bin/env python3
#===============================================================================
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#===============================================================================

import os
import sys


def append_files(input_f, output_f):
    file = open(input_f, "r")
    data = file.read()
    file.close()

    fout = open(output_f, "a")
    fout.write(data)
    fout.close()


def append_text(text, input_f):
    fout = open(input_f, "a")
    fout.write(text)
    fout.close()


if len(sys.argv) >= 2:
    source_root_path = sys.argv[1]
else:
    source_root_path = r"../../../.."


dev_cfg_export_path = os.path.join(
    source_root_path + r"/tools/Target_tools/dev_cfg/export"
)
os.chdir(dev_cfg_export_path)

if os.path.exists(r"all_xml_modules_to_cat.xml"):
    os.remove(r"all_xml_modules_to_cat.xml")

os.system("python xml_combine_production.py ?.xml | tee all_xml_modules_to_cat.xml")

if os.path.exists("master_xml.xml"):
    os.remove("master_xml.xml")

append_files("first_part_to_cat.xml", "master_xml.xml")
append_files("all_xml_modules_to_cat.xml", "master_xml.xml")
append_files("last_part_to_cat.xml", "master_xml.xml")

if os.path.exists(r"../inc/master_xml.xml"):
    os.remove(r"../inc/master_xml.xml")

append_files("master_xml.xml", r"../inc/master_xml.xml")

# code to generate header files
input_f = "master_xml.xml"
byte_seq_input = r"../inc/byte_sequence.xml"
output_f = "Names.h"
output2_f = "Names2.h"
byte_enum_output = "byte_enum.h"
byte_struct_output = "byte_struct.h"
output_list1 = []
output_list2 = []
count = 1
count1 = 1

if os.path.exists(output_f):
    os.remove(output_f)
if os.path.exists(output2_f):
    os.remove(output2_f)
if os.path.exists(byte_enum_output):
    os.remove(byte_enum_output)
if os.path.exists(byte_struct_output):
    os.remove(byte_struct_output)

# remove system generated files, if they already exist
if os.path.exists(r"../inc/nt_devcfg_structure.h"):
    os.remove(r"../inc/nt_devcfg_structure.h")
if os.path.exists(r"../../../../core/common/nt_devcfg.h"):
    os.remove(r"../../../../core/common/nt_devcfg.h")
if os.path.exists(r"../../../../core/common/nt_devcfg_byte_seq.h"):
    os.remove(r"../../../../core/common/nt_devcfg_byte_seq.h")
if os.path.exists(
    r"../../../../tools/Target_tools/dev_cfg/inc/nt_devcfg_byte_seq_structure.h"
):
    os.remove(
        r"../../../../tools/Target_tools/dev_cfg/inc/nt_devcfg_byte_seq_structure.h"
    )

append_text("/** System Generated File\n", "Names.h")
append_text("/** System Generated File\n", "Names2.h")
append_text("/** System Generated File\n", "byte_enum.h")
append_text("/** System Generated File\n", "byte_struct.h")

append_text("*  Don't Change Manually */\n", "Names.h")
append_text("*  Don't Change Manually */\n", "Names2.h")
append_text("*  Don't Change Manually */\n", "byte_enum.h")
append_text("*  Don't Change Manually */\n", "byte_struct.h")

append_text("#ifndef CORE_DEV_CFG_EXPORT_NT_DEVCFG_H_\n", "Names.h")
append_text("#define CORE_DEV_CFG_EXPORT_NT_DEVCFG_H_\n", "Names.h")
append_text("typedef enum nt_devcfg_id_s \n", "Names.h")
append_text("{\n", "Names.h")

append_text("#ifndef CORE_COMMON_NT_DEVCFG_BYTE_SEQ_H_\n", "byte_enum.h")
append_text("#define CORE_COMMON_NT_DEVCFG_BYTE_SEQ_H_\n", "byte_enum.h")
append_text("typedef enum nt_devcfg_byte_seq_id_s \n", "byte_enum.h")
append_text("{\n", "byte_enum.h")

append_text("#ifndef CORE_DEV_CFG_INC_NT_DEVCFG_STRUCTURE_H_\n", "Names2.h")
append_text("#define CORE_DEV_CFG_INC_NT_DEVCFG_STRUCTURE_H_\n", "Names2.h")
append_text('#include "nt_devcfg_types.h" \n', "Names2.h")
append_text('#include "nt_devcfg_def.h"\n', "Names2.h")
append_text("void nt_devcfg_parse(); \n", "Names2.h")
append_text("typedef struct nt_devcfg_structure_s \n", "Names2.h")
append_text("{\n", "Names2.h")

append_text(
    "#ifndef CORE_DEV_CFG_INC_NT_DEVCFG_BYTE_SEQ_STRUCTURE_H_\n", "byte_struct.h"
)
append_text(
    "#define CORE_DEV_CFG_INC_NT_DEVCFG_BYTE_SEQ_STRUCTURE_H_\n", "byte_struct.h"
)
append_text('#include "nt_devcfg_def.h" \n', "byte_struct.h")
append_text('#include "nt_devcfg_types.h" \n', "byte_struct.h")
append_text("void nt_devcfg_byte_seq_parse();\n", "byte_struct.h")
append_text("typedef struct nt_devcfg_byte_seq_structure_s\n", "byte_struct.h")
append_text("{\n", "byte_struct.h")

fhand = open(input_f)
for line in fhand:
    line = line.strip()

    split_line = line.split()

    if len(split_line) > 2:
        if split_line[2].split("=")[0] == "id_name":
            if split_line[2].split("=")[1][-1] == '"':
                flag = split_line[2].split("=")[1][1:-1]
            else:
                flag = split_line[2].split("=")[1][1:]
            text = "{} = {},\n".format(flag, count)

            if flag not in output_list1:
                count += 1
                output_list1.append(flag)
                append_text(text, output_f)

            text2 = "uint32 {};\n".format(flag)

            if text2 not in output_list2:
                output_list2.append(text2)
                append_text(text2, output2_f)

append_text("}nt_devcfg_id_t; \n", "Names.h")
append_text(
    "void* nt_devcfg_get_config(int enum_id);        // callback function for unit value \n",
    "Names.h",
)
append_text(
    "void* nt_devcfg_ascii_config(int enum_id);        // callback function for ascii value \n",
    "Names.h",
)
append_text("#endif /* CORE_DEV_CFG_EXPORT_NT_DEVCFG_H_ */ \n", "Names.h")

if os.path.exists(r"../../../../core/common/nt_devcfg.h"):
    os.remove(r"../../../../core/common/nt_devcfg.h")
append_files("Names.h", r"../../../../core/common/nt_devcfg.h")

append_text("}nt_devcfg_structure_t; \n", "Names2.h")
append_text("#endif /* CORE_DEV_CFG_INC_NT_DEVCFG_STRUCTURE_H_ */\n", "Names2.h")

if os.path.exists(r"../inc/nt_devcfg_structure.h"):
    os.remove(r"../inc/nt_devcfg_structure.h")
append_files("Names2.h", r"../inc/nt_devcfg_structure.h")

byte_hand = open(byte_seq_input)
for byte_line in byte_hand:
    split_line = byte_line.strip().split()

    if len(split_line) > 2:
        if split_line[2].split("=")[0] == "id_name":
            count1 += 1
            flag = split_line[2].split("=")[1][1:-1]
            append_text("{} = {},\n".format(flag, count1), byte_enum_output)
            append_text("uint32 {};\n".format(flag), byte_struct_output)

append_text("}nt_devcfg_byte_seq_id_t; \n", "byte_enum.h")
append_text(
    "void* nt_devcfg_byte_seq_config(int enum_id);         // callback function for byte sequence value\n",
    "byte_enum.h",
)
append_text("#endif /* CORE_COMMON_NT_DEVCFG_BYTE_SEQ_H_ */\n", "byte_enum.h")

if os.path.exists(r"../../../../core/common/nt_devcfg_byte_seq.h"):
    os.remove(r"../../../../core/common/nt_devcfg_byte_seq.h")
append_files("byte_enum.h", r"../../../../core/common/nt_devcfg_byte_seq.h")

append_text("}nt_devcfg_byte_seq_structure_t;\n", "byte_struct.h")
append_text(
    "#endif /* CORE_DEV_CFG_INC_NT_DEVCFG_BYTE_SEQ_STRUCTURE_H_ */\n", "byte_struct.h"
)

if os.path.exists(
    r"../../../../tools/Target_tools/dev_cfg/inc/nt_devcfg_byte_seq_structure.h"
):
    os.remove(
        r"../../../../tools/Target_tools/dev_cfg/inc/nt_devcfg_byte_seq_structure.h"
    )
append_files(
    "byte_struct.h",
    r"../../../../tools/Target_tools/dev_cfg/inc/nt_devcfg_byte_seq_structure.h",
)

exit()
