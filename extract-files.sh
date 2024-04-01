#!/bin/bash
#
# Copyright (C) 2016 The CyanogenMod Project
# Copyright (C) 2017-2020 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0

if [ "${BASH_SOURCE[0]}" != "${0}" ]; then
    return
fi

set -e

# Required!
DEVICE=markw
VENDOR=xiaomi

# Load extract_utils and do some sanity checks
MY_DIR="${BASH_SOURCE%/*}"
if [[ ! -d "${MY_DIR}" ]]; then MY_DIR="${PWD}"; fi

ANDROID_ROOT="${MY_DIR}/../../.."

HELPER="${ANDROID_ROOT}/tools/extract-utils/extract_utils.sh"
if [ ! -f "$HELPER" ]; then
    echo "Unable to find helper script at $HELPER"
    exit 1
fi
source "${HELPER}"

# Default to sanitizing the vendor folder before extraction
CLEAN_VENDOR=true

KANG=
SECTION=

while [ "${#}" -gt 0 ]; do
    case "${1}" in
        -n | --no-cleanup )
                CLEAN_VENDOR=false
                ;;
        -k | --kang )
                KANG="--kang"
                ;;
        -s | --section )
                SECTION="${2}"; shift
                CLEAN_VENDOR=false
                ;;
        * )
                SRC="${1}"
                ;;
    esac
    shift
done

if [ -z "${SRC}" ]; then
    SRC="adb"
fi

# Initialize the helper
setup_vendor "${DEVICE}" "${VENDOR}" "${ANDROID_ROOT}" false "${CLEAN_VENDOR}"

extract "${MY_DIR}"/proprietary-files.txt "${SRC}" \
        "${KANG}" --section "${SECTION}"

DEVICE_BLOB_ROOT="${ANDROID_ROOT}"/vendor/"${VENDOR}"/"${DEVICE}"/proprietary

# Dolby
"${PATCHELF} --replace-needed "libstagefright_foundation.so" "libstagefright_foundation-v33.so" "${DEVICE_BLOB_ROOT}"/vendor/lib/libstagefright_soft_ddpdec.so"
"${PATCHELF} --replace-needed "libstagefright_foundation.so" "libstagefright_foundation-v33.so" "${DEVICE_BLOB_ROOT}"/vendor/lib/libstagefright_soft_ac4dec.so"
"${PATCHELF} --replace-needed "libstagefright_foundation.so" "libstagefright_foundation-v33.so" "${DEVICE_BLOB_ROOT}"/vendor/lib/libstagefrightdolby.so"
"${PATCHELF} --replace-needed "libstagefright_foundation.so" "libstagefright_foundation-v33.so" "${DEVICE_BLOB_ROOT}"/vendor/lib64/libstagefright_soft_ddpdec.so"
"${PATCHELF} --replace-needed "libstagefright_foundation.so" "libstagefright_foundation-v33.so" "${DEVICE_BLOB_ROOT}"/vendor/lib64/libdlbdsservice.so"
"${PATCHELF} --replace-needed "libstagefright_foundation.so" "libstagefright_foundation-v33.so" "${DEVICE_BLOB_ROOT}"/vendor/lib64/libstagefright_soft_ac4dec.so"
"${PATCHELF} --replace-needed "libstagefright_foundation.so" "libstagefright_foundation-v33.so" "${DEVICE_BLOB_ROOT}"/vendor/lib64/libstagefrightdolby.so"

# Gnss
sed -i -e '$a\\    capabilities NET_BIND_SERVICE' "${DEVICE_BLOB_ROOT}"/vendor/etc/init/android.hardware.gnss@2.1-service-qti.rc

# RIL
for v in 1.{0..2}; do
    sed -i "s|android.hardware.radio.config@${v}.so|android.hardware.radio.c_shim@${v}.so|g" "${DEVICE_BLOB_ROOT}"/vendor/lib64/libril-qc-hal-qmi.so"
done

"${MY_DIR}/setup-makefiles.sh"
