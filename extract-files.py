#!/usr/bin/env -S PYTHONPATH=../../../tools/extract-utils python3
#
# SPDX-FileCopyrightText: 2024 The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#

from extract_utils.fixups_blob import (
    blob_fixup,
    blob_fixups_user_type,
)
from extract_utils.fixups_lib import (
    lib_fixup_remove,
    lib_fixups,
    lib_fixups_user_type,
)
from extract_utils.main import (
    ExtractUtils,
    ExtractUtilsModule,
)

namespace_imports = [
    'device/xiaomi/markw',
    'device/xiaomi/markw/qcom-caf',
    'vendor/qcom/opensource/dataservices',
]

def lib_fixup_vendor_suffix(lib: str, partition: str, *args, **kwargs):
    return f'{lib}_{partition}' if partition == 'vendor' else None
lib_fixups: lib_fixups_user_type = {
    **lib_fixups,
    (
        'com.qualcomm.qti.dpm.api@1.0',
        'vendor.qti.imsrtpservice@3.0',
    ): lib_fixup_vendor_suffix,
}

# Define the blob fixups
blob_fixups: blob_fixups_user_type = {
    # Camera - - libstdc++.so' -> 'libstdc++_vendor.so
    ('vendor/lib/libts_detected_face_hal.so', 'vendor/lib/libts_face_beautify_hal.so', 'vendor/lib/libubifocus.so', 'vendor/lib/libseemore.so', 'vendor/lib/liboptizoom.so', 'vendor/lib/libchromaflash.so'): blob_fixup()
        .replace_needed('libstdc++.so', 'libstdc++_vendor.so'),
    # Camera - uneeded
    'vendor/lib/libmmcamera_tuning.so': blob_fixup()
        .remove_needed('libmm-qcamera.so'),
    # Dolby
    ('vendor/lib64/libdlbdsservice.so', 'vendor/lib/libstagefright_soft_ddpdec.so'): blob_fixup()
        .replace_needed('libstagefright_foundation.so', 'libstagefright_foundation-v33.so'),
    # Fingerprint - shims & uneeded
    'vendor/bin/gx_fpd': blob_fixup()
        .remove_needed('libunwind.so')
        .remove_needed('libbacktrace.so')
        .add_needed('gx_fpd_shim.so')
        .add_needed('fakelogprint.so'),
    ('vendor/lib64/hw/fingerprint.goodix.so', 'vendor/lib64/hw/gxfingerprint.default.so'): blob_fixup()
        .add_needed('fakelogprint.so'),
    # Fingerprint - liblog dep.
    ('vendor/lib64/libfp_client.so', 'vendor/lib64/libfpservice.so'): blob_fixup()
        .add_needed('liblog.so'),
    # Fingerprint - fix Unresolved symbol: _ZN7android22checkCallingPermissionERKNS_8String16E.
    'vendor/lib64/libfpservice.so': blob_fixup()
        .add_needed('libshims_binder.so'),
    # Fingerprint - libstdc++.so' -> 'libstdc++_vendor.so
    ('vendor/lib64/libfp_client.so', 'vendor/lib64/libfpservice.so', 'vendor/lib64/libfpnav.so', 'vendor/lib64/hw/fingerprint.goodix.so', 'vendor/lib64/hw/gxfingerprint.default.so'): blob_fixup()
        .replace_needed('libstdc++.so', 'libstdc++_vendor.so'),
}  # fmt: skip

# Define the module
module = ExtractUtilsModule(
    'markw',
    'xiaomi',
    blob_fixups=blob_fixups,
    lib_fixups=lib_fixups,
    namespace_imports=namespace_imports,
)

if __name__ == '__main__':
    utils = ExtractUtils.device(module)
    utils.run()
