
import os
import sys
import re

build_version_file = r"sbl_version_build.h"
build_count_file = r"sbl_version_count.txt"

# increments the build count and generates the new build version file
def increment_build_count(build_version_file, build_count_file):
    # read the existing build count for SBL build
    try:
        with open(build_count_file) as count_file:
            build_count = int(count_file.read().strip())
    except Exception as e:
        print(e)
        print("SBL Build count file does not exist")
        print("Creating new sbl build count file")

        # extract the build count from the build_version_file file
        with open(build_version_file, "r") as version_file:
            file_data = version_file.read()

            build_count = re.search(".*SBL_VER_COUNT(.*)", file_data)
            build_count = int(build_count.group(1).strip())

        # create a new build count file
        with open(build_count_file, "w+") as count_file:
            count_file.write(str(build_count))

    # increment the build count
    build_count += 1

    new_version_file = r"""
#ifndef SBL_VERSION_BUILD
#define SBL_VERSION_BUILD
#define SBL_VER_MAJOR 0
#define SBL_VER_MINOR 1
#define SBL_VER_COUNT {}
#endif
""".format(
        build_count
    )

    # write the new version number to sbl build version file
    with open(build_version_file, "w+") as version_file:
        version_file.write(new_version_file)

    # write the new build count number to the sbl count file
    with open(build_count_file, "w+") as count_file:
        count_file.write(str(build_count))

def append_text(text, input_f):
    fout = open(input_f, "a")
    fout.write(text)
    fout.close()


if len(sys.argv) >= 3:
    source_root_path = sys.argv[1]
    variant_image_id = sys.argv[3]

if variant_image_id == "SBL":
    sbl_common_path = source_root_path + r"/bootloader/SBL/common"
    os.chdir(sbl_common_path)
    print("Check if " + build_version_file + " file already exists")
    if os.path.exists(build_version_file):
        print(build_version_file + " file already exists, will increment the build count number")
        increment_build_count(build_version_file, build_count_file);
    else:
        print("Creating "+ build_version_file)
        append_text("#ifndef SBL_VERSION_BUILD \n", build_version_file)
        append_text("#define SBL_VERSION_BUILD \n", build_version_file)
        append_text("#define SBL_VER_MAJOR 0 \n", build_version_file)
        append_text("#define SBL_VER_MINOR 1 \n", build_version_file)
        append_text("#define SBL_VER_COUNT 1 \n", build_version_file)
        append_text("#endif \n", build_version_file)
        print(build_count_file + " does not exist, will create a new file")
        append_text("1", build_count_file)
else:
    print("variant_image_id is not correct")
