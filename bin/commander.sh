#!/bin/bash
#############################
#
# 3DSTool Commander v0.9.2
#   by Qu Chao
#
# ---------------------------
#
# Installation:
#   Put this script into the same folder as 3dstool,
# or just edit $EXE_PATH in the CONFIGS section below.
#
# Usage:
#   % extract path_to_3ds_rom path_to_xorpad_dir
#
#############################

############################
# CONFIGS
############################
EXE_PATH="./3dstool"

############################
# DEFAULTS
############################
CCI_PATH="./cci" # todo: to use rom name as the folder name
PART_LIST=("Main" "Manual" "DownloadPlay" "Partition4" "Partition5" "Partition6" "Partition7" "UpdateData")

############################
# MAIN
############################
# funcs
existence_pattern_check(){
    [ -n "$1" ] && [ -e "$1" ] && ( [ "${2:-file}" = "file" ] && ( [ -f "$1" ] || [ -L "$1" ] ) || [ -d "$1" ] ) && [ -r "$1" ]
}

# check if 3dstool is existing & execuable
if [ -z "${EXE_PATH}" ]; then
    EXE_PATH="$(cd `dirname $0`; pwd)/3dstool"
fi
if ! existence_pattern_check "${EXE_PATH}"; then
    echo -e "3DSTool isn't found"
    exit 1
elif [ ! -x "${EXE_PATH}" ]; then
    echo -e "3DSTool isn't excutable!"
    exit 1
fi

# input: rom file path
if ! existence_pattern_check "$1"; then
    echo -e "No 3ds rom path specified!"
    exit 1
fi

# input: xorpads dir path
if ! existence_pattern_check "$2" "dir"; then
    echo -e "No xorpads' dir path specified!"
    exit 1
else
    cd "$2" # make the pattern_check trick avaiable
fi

# update the save path
CCI_PATH="`dirname "$1"`/${CCI_PATH/.\//}"
if existence_pattern_check "${CCI_PATH}" "dir"; then
    echo -e "A folder is located at \"${CCI_PATH}\" already!"
    exit 1
fi

# extract NCSD header
echo -e "- Extracting NCSD header..."
mkdir -p "${CCI_PATH}"
"$EXE_PATH" "-xvf" "$1" "--header" "${CCI_PATH}/ncsdheader.bin"
echo -e "- Done!\n"

# check xorpads
for ((part_idx=0; part_idx<${#PART_LIST[@]}; part_idx++)); do
    # shortcuts
    part_name="${PART_LIST[part_idx]}"

    # filter non-existing xorpads by pattern
    romfs_xorpad_pattern=*.${part_name}.romfs.xorpad
    if ! existence_pattern_check ${romfs_xorpad_pattern}; then
        echo -e "Skipped, no xorpad for the ${part_name} partition"
        continue
    fi

    # partition vars
    part_type="cfa"
    part_img="${CCI_PATH}/${part_name}.bin"
    part_dir="${part_img/.bin/}"
    romfs_bin="${part_dir}/romfs.bin"
    romfs_dir="${romfs_bin/.bin/}"

    # command args
    cmd_args=("$EXE_PATH" "-xvtf" "${part_type}" "${part_img}" "--header" "${part_dir}/ncchheader.bin" "--romfs" "${romfs_bin}" "--romfs-xor" "${2}/`ls ${romfs_xorpad_pattern}`")

    # extract partition images
    echo -e "- Extracting the ${part_name} partition..."
    "$EXE_PATH" "-xvt${part_idx}f" "cci" "${part_img}" "$1"
    echo -e "- Done!\n"

    # decrypt & extract data from partition images
    echo -e "- Decrypting & Extracting data from the ${part_name} partition..."
    mkdir -p "${part_dir}"
    # for the 'Main' partition only
    if [ ${part_idx} -eq 0 ]; then
        # vars
        exefs_bin="${part_dir}/exefs.bin"
        exefs_dir="${exefs_bin/.bin/}"

        # extra arguments, todo: check if there's a logo
        cmd_args+=("--logoregion" "${part_dir}/logo.bcma.lz" "--plainregion" "${part_dir}/plain.bin" "--extendedheader" "${part_dir}/exh.bin" "--extendedheader-xor" "${2}/`ls ${romfs_xorpad_pattern/romfs/exheader}`" "--exefs" "${exefs_bin}" "--exefs-xor" "${2}/`ls ${romfs_xorpad_pattern/romfs/exefs_norm}`")

        # for 7.x Crypto only
        exefs_7x_xorpad_pattern=${romfs_xorpad_pattern/romfs/exefs_7x}
        if existence_pattern_check ${exefs_7x_xorpad_pattern}; then
            cmd_args+=("--exefs-top-xor" "${2}/`ls ${exefs_7x_xorpad_pattern}`")
        fi

        # update the partition type
        cmd_args[2]="cxi"
    fi
    "${cmd_args[@]}"
    rm "${part_img}"
    echo -e "- Done!\n"

    # extract data from exefs, for the 'Main' partition only
    if [ ${part_idx} -eq 0 ]; then
        if existence_pattern_check "${exefs_bin}"; then
            echo -e "- Extracting data from exefs of the ${part_name} partition..."
            mkdir -p "${exefs_dir}"
            # todo: check if it's LZ77 compressed
            "$EXE_PATH" "-xvtfu" "exefs" "${exefs_bin}" "--header" "${exefs_dir}/exefsheader.bin" "--exefs-dir" "${exefs_dir}"
            rm "${exefs_bin}"
            echo -e "- Done!\n"
        fi

        # vars
        banner_ext=".bnr"
        banner_bin="${exefs_dir}/banner${banner_ext}"
        banner_dir="${banner_bin/${banner_ext}/}"

        # extract data from banner of the exefs
        if existence_pattern_check "${banner_bin}"; then
            echo -e "- Extracting data from banner fo the ${part_name} partition..."
            mkdir -p "${banner_dir}"
            "$EXE_PATH" "-xvtf" "banner" "${banner_bin}" "--banner-dir" "${banner_dir}"
            rm "${banner_bin}"
            echo -e "- Done!\n"
        fi
    fi

    # extract data from romfs
    if existence_pattern_check "${romfs_bin}"; then
        echo -e "- Extracting data from romfs of the ${part_name} partition..."
        mkdir -p "${part_dir}/romfs"
        "$EXE_PATH" "-xvtf" "romfs" "${romfs_bin}" "--romfs-dir" "${romfs_dir}"
        rm "${romfs_bin}"
        echo -e "- Done!\n"
    fi

    # happy ending
    echo -e "All resources were extracted to \"${CCI_PATH}\"!"
done
