#!/usr/bin/env python3

from pathlib import Path


THIS_DIR = Path(__file__).resolve().parent
ANDROID_ROOT = THIS_DIR.parents[2]
PROP_ROOT = ANDROID_ROOT / "vendor/xiaomi/markw/proprietary"
OUT_FILE = THIS_DIR / "proprietary-files.txt"


def collect_files(root: Path) -> list[str]:
    return sorted(
        p.relative_to(root).as_posix()
        for p in root.rglob("*")
        if p.is_file()
    )


def parse_dest_path(entry: str) -> str:
    """
    Extract destination path from a proprietary-files.txt entry.

    Examples:
      vendor/lib/libfoo.so|hash
        -> vendor/lib/libfoo.so

      vendor/lib/libfoo.so;DISABLE_CHECKELF|hash
        -> vendor/lib/libfoo.so

      etc/camera/camera_config.xml:vendor/etc/camera/camera_config.xml
        -> vendor/etc/camera/camera_config.xml

      lib64/hw/fingerprint.default.so:vendor/lib64/hw/fingerprint.fpc.so
        -> vendor/lib64/hw/fingerprint.fpc.so
    """
    core = entry.split("|", 1)[0]

    if ":" in core:
        _, core = core.split(":", 1)

    return core.split(";", 1)[0]


def load_reference_entries(path: Path) -> dict[str, str]:
    """
    Load the current proprietary-files.txt before overwriting it and keep
    exact lines keyed by their destination path.
    """
    if not path.exists():
        return {}

    entries: dict[str, str] = {}

    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()

        if not line or line.startswith("#"):
            continue

        dest = parse_dest_path(line)
        entries[dest] = line

    return entries


# -----------------------------------------------------------------------------
# Fallback exact lines for entries that use src:dest mapping and therefore
# cannot be reconstructed from the on-disk proprietary tree path alone.
# These are only used if the old proprietary-files.txt is missing or does not
# contain the entry anymore.
# -----------------------------------------------------------------------------

FALLBACK_EXACT: dict[str, str] = {
    # Camera config mappings
    "vendor/etc/camera/camera_config.xml":
        "etc/camera/camera_config.xml:vendor/etc/camera/camera_config.xml",
    "vendor/etc/camera/csidtg_camera.xml":
        "etc/camera/csidtg_camera.xml:vendor/etc/camera/csidtg_camera.xml",
    "vendor/etc/camera/csidtg_chromatix.xml":
        "etc/camera/csidtg_chromatix.xml:vendor/etc/camera/csidtg_chromatix.xml",
    "vendor/etc/camera/ofilm_s5k3l8_f3l8yam_chromatix.xml":
        "etc/camera/ofilm_s5k3l8_f3l8yam_chromatix.xml:vendor/etc/camera/ofilm_s5k3l8_f3l8yam_chromatix.xml",
    "vendor/etc/camera/ov5670_f5670bq_chromatix.xml":
        "etc/camera/ov5670_f5670bq_chromatix.xml:vendor/etc/camera/ov5670_f5670bq_chromatix.xml",
    "vendor/etc/camera/qtech_ov5670_f5670bq_chromatix.xml":
        "etc/camera/qtech_ov5670_f5670bq_chromatix.xml:vendor/etc/camera/qtech_ov5670_f5670bq_chromatix.xml",
    "vendor/etc/camera/qtech_s5k3l8_f3l8yam_chromatix.xml":
        "etc/camera/qtech_s5k3l8_f3l8yam_chromatix.xml:vendor/etc/camera/qtech_s5k3l8_f3l8yam_chromatix.xml",
    "vendor/etc/camera/s5k3l8_f3l8yam_chromatix.xml":
        "etc/camera/s5k3l8_f3l8yam_chromatix.xml:vendor/etc/camera/s5k3l8_f3l8yam_chromatix.xml",
    "vendor/etc/camera/s5k5e8_chromatix.xml":
        "etc/camera/s5k5e8_chromatix.xml:vendor/etc/camera/s5k5e8_chromatix.xml",
    "vendor/etc/camera/sunny_s5k3l8_f3l8yam_chromatix.xml":
        "etc/camera/sunny_s5k3l8_f3l8yam_chromatix.xml:vendor/etc/camera/sunny_s5k3l8_f3l8yam_chromatix.xml",

    # Fingerprint mappings
    "vendor/lib64/hw/fingerprint.fpc.so":
        "lib64/hw/fingerprint.default.so:vendor/lib64/hw/fingerprint.fpc.so",
    "vendor/bin/gx_fpd":
        "bin/gx_fpd:vendor/bin/gx_fpd",
    "vendor/lib64/hw/fingerprint.goodix.so":
        "lib64/hw/fingerprint.goodix.so:vendor/lib64/hw/fingerprint.goodix.so",
    "vendor/lib64/hw/gxfingerprint.default.so":
        "lib64/hw/gxfingerprint.default.so:vendor/lib64/hw/gxfingerprint.default.so",
    "vendor/lib64/libfp_client.so":
        "lib64/libfp_client.so:vendor/lib64/libfp_client.so",
    "vendor/lib64/libfpnav.so":
        "lib64/libfpnav.so:vendor/lib64/libfpnav.so",
    "vendor/lib64/libfpservice.so":
        "lib64/libfpservice.so:vendor/lib64/libfpservice.so",
    "vendor/lib64/libcom_fingerprints_service.so":
        "lib64/libcom_fingerprints_service.so:vendor/lib64/libcom_fingerprints_service.so",
}


# -----------------------------------------------------------------------------
# Fallback suffixes/attributes for entries that can be reconstructed safely.
# Exact old lines from proprietary-files.txt still have priority over these.
# -----------------------------------------------------------------------------

FALLBACK_SUFFIXES: dict[str, str] = {
    # App requirements
    "vendor/app/CneApp/CneApp.apk":
        ";REQUIRED=CneApp.libvndfwk_detect_jni.qti_symlink",

    # EGL symlinks
    "vendor/lib/egl/libEGL_adreno.so":
        ";SYMLINK=vendor/lib/libEGL_adreno.so",
    "vendor/lib/egl/libGLESv2_adreno.so":
        ";SYMLINK=vendor/lib/libGLESv2_adreno.so",
    "vendor/lib64/egl/libEGL_adreno.so":
        ";SYMLINK=vendor/lib64/libEGL_adreno.so",
    "vendor/lib64/egl/libGLESv2_adreno.so":
        ";SYMLINK=vendor/lib64/libGLESv2_adreno.so",

    # IMS JNI symlinks
    "system_ext/lib64/libimscamera_jni.so":
        ";SYMLINK=system_ext/priv-app/ims/lib/arm64/libimscamera_jni.so",
    "system_ext/lib64/libimsmedia_jni.so":
        ";SYMLINK=system_ext/priv-app/ims/lib/arm64/libimsmedia_jni.so",

    # Explicit MODULE_SUFFIX from the original markw list
    "vendor/lib64/com.qualcomm.qti.dpm.api@1.0.so":
        ";MODULE_SUFFIX=_vendor",
    "vendor/lib64/vendor.qti.imsrtpservice@3.0.so":
        ";MODULE_SUFFIX=_vendor",

    # Explicit non-chromatix DISABLE_CHECKELF entries from the original list
    "vendor/lib/libmmcamera_faceproc.so":
        ";DISABLE_CHECKELF",
    "vendor/lib/libmmcamera_faceproc2.so":
        ";DISABLE_CHECKELF",
    "vendor/lib/libmmcamera_s5k3l8_eeprom.so":
        ";DISABLE_CHECKELF",
    "vendor/lib/libmmcamera_ov5670_eeprom.so":
        ";DISABLE_CHECKELF",
    "vendor/lib/libmmcamera_s5k5e8_eeprom.so":
        ";DISABLE_CHECKELF",
    "vendor/lib/libmmcamera_trueportrait_lib.so":
        ";DISABLE_CHECKELF",
    "vendor/lib/libmmcamera_tuning.so":
        ";DISABLE_CHECKELF",
}


def fallback_entry_for(path: str) -> str | None:
    """
    Build a safe fallback line if the old proprietary-files.txt does not
    contain this file.

    Priority:
      1) exact src:dest mappings
      2) exact suffix overrides
      3) heuristics for markw camera chromatix blobs
      4) plain path
    """
    if path in FALLBACK_EXACT:
        return FALLBACK_EXACT[path]

    if path in FALLBACK_SUFFIXES:
        return path + FALLBACK_SUFFIXES[path]

    name = Path(path).name

    # All libchromatix_* entries in the provided markw list are DISABLE_CHECKELF
    if path.startswith("vendor/lib/libchromatix_") and path.endswith(".so"):
        return f"{path};DISABLE_CHECKELF"

    # Keep plain path otherwise
    return path


def build_entry(path: str, reference_entries: dict[str, str]) -> str:
    """
    Prefer exact original line from the previous proprietary-files.txt.
    This preserves hashes, DISABLE_CHECKELF, SYMLINKs, MODULE_SUFFIX, mappings,
    and any future hand-written flags.
    """
    if path in reference_entries:
        return reference_entries[path]

    return fallback_entry_for(path)


# -----------------------------------------------------------------------------
# Grouping order for Xiaomi markw
# -----------------------------------------------------------------------------

GROUPS: list[tuple[str, list[str]]] = [
    ("# Product - Apps", ["product/app/"]),
    ("# Product - Permissions", ["product/etc/permissions/"]),
    ("# Product - ETC", ["product/etc/"]),

    ("# System Ext - Apps", ["system_ext/app/"]),
    ("# System Ext - Priv Apps", ["system_ext/priv-app/"]),
    ("# System Ext - Permissions", ["system_ext/etc/permissions/"]),
    ("# System Ext - Sysconfig", ["system_ext/etc/sysconfig/"]),
    ("# System Ext - ETC", ["system_ext/etc/"]),
    ("# System Ext - Framework", ["system_ext/framework/"]),
    ("# System Ext - Lib", ["system_ext/lib/"]),
    ("# System Ext - Lib64", ["system_ext/lib64/"]),

    ("# System - Permissions", ["etc/permissions/"]),
    ("# System - Sysconfig", ["etc/sysconfig/"]),
    ("# System - ETC", ["etc/"]),

    ("# Vendor - Apps", ["vendor/app/"]),
    ("# Vendor - HAL Binaries", ["vendor/bin/hw/"]),
    ("# Vendor - Binaries", ["vendor/bin/"]),

    ("# Vendor - ACDB Data", ["vendor/etc/acdbdata/"]),
    ("# Vendor - Camera Configs", ["vendor/etc/camera/"]),
    ("# Vendor - Data Configs", ["vendor/etc/data/"]),
    ("# Vendor - Default Permissions", ["vendor/etc/default-permissions/"]),
    ("# Vendor - Init Scripts", ["vendor/etc/init/"]),
    ("# Vendor - Perf Configs", ["vendor/etc/perf/"]),
    ("# Vendor - Seccomp Policies", ["vendor/etc/seccomp_policy/"]),
    ("# Vendor - VINTF", ["vendor/etc/vintf/"]),
    ("# Vendor - ETC", ["vendor/etc/"]),

    ("# Vendor - Firmware", ["vendor/firmware/"]),

    ("# Vendor - EGL Libraries (32-bit)", ["vendor/lib/egl/"]),
    ("# Vendor - HAL Libraries (32-bit)", ["vendor/lib/hw/"]),
    ("# Vendor - ADSP Libraries (32-bit)", ["vendor/lib/rfsa/adsp/"]),
    ("# Vendor - SoundFX Libraries (32-bit)", ["vendor/lib/soundfx/"]),
    ("# Vendor - Libraries (32-bit)", ["vendor/lib/"]),

    ("# Vendor - EGL Libraries (64-bit)", ["vendor/lib64/egl/"]),
    ("# Vendor - HAL Libraries (64-bit)", ["vendor/lib64/hw/"]),
    ("# Vendor - SoundFX Libraries (64-bit)", ["vendor/lib64/soundfx/"]),
    ("# Vendor - Libraries (64-bit)", ["vendor/lib64/"]),

    ("# Vendor - Radio Database Upgrades", ["vendor/radio/qcril_database/upgrade/"]),
    ("# Vendor - Radio Database", ["vendor/radio/qcril_database/"]),
]


def main() -> None:
    if not PROP_ROOT.exists():
        raise SystemExit(f"Missing proprietary dir: {PROP_ROOT}")

    reference_entries = load_reference_entries(OUT_FILE)
    files = collect_files(PROP_ROOT)

    assigned: set[str] = set()
    output: list[str] = [
        "# Proprietary files for Xiaomi markw",
        "# Autogenerated from vendor/xiaomi/markw/proprietary",
        "# Existing lines from proprietary-files.txt are preserved exactly when possible",
        "# No automatic MODULE_SUFFIX rewriting is performed",
        "",
    ]

    for header, prefixes in GROUPS:
        group_files = [
            f for f in files
            if f not in assigned and any(f.startswith(pref) for pref in prefixes)
        ]

        if not group_files:
            continue

        output.append(header)
        for f in group_files:
            output.append(build_entry(f, reference_entries))
            assigned.add(f)
        output.append("")

    remaining = [f for f in files if f not in assigned]
    if remaining:
        output.append("# Ungrouped")
        for f in remaining:
            output.append(build_entry(f, reference_entries))
        output.append("")

    OUT_FILE.write_text("\n".join(output).rstrip() + "\n", encoding="utf-8")

    print(f"Wrote: {OUT_FILE}")
    print(f"Files: {len(files)}")
    print(f"Preserved exact old entries: {sum(1 for f in files if f in reference_entries)}")
    if not reference_entries:
        print("Warning: previous proprietary-files.txt was not found, only fallback rules were used.")
    if remaining:
        print(f"Ungrouped: {len(remaining)}")


if __name__ == "__main__":
    main()
