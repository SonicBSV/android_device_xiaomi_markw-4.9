#
# Copyright (C) 2021
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# This contains the module build definitions for the hardware-specific
# components for this device.
#
# As much as possible, those components should be built unconditionally,
# with device-specific names to avoid collisions, to avoid device-specific
# bitrot and build breakages. Building a component unconditionally does
# *not* include it on all devices, so it is safe even with hardware-specific
# components.

LOCAL_PATH := $(call my-dir)

ifeq ($(TARGET_DEVICE),markw)

include $(call all-makefiles-under,$(LOCAL_PATH))

include $(CLEAR_VARS)

LOCAL_MODULE := wifi_symlinks
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := FAKE
LOCAL_MODULE_SUFFIX := -timestamp

include $(BUILD_SYSTEM)/base_rules.mk

$(LOCAL_BUILT_MODULE): ACTUAL_INI_FILE := /vendor/etc/wifi/WCNSS_qcom_cfg.ini
$(LOCAL_BUILT_MODULE): WCNSS_INI_SYMLINK := $(TARGET_OUT_VENDOR)/firmware/wlan/prima/WCNSS_qcom_cfg.ini

$(LOCAL_BUILT_MODULE): ACTUAL_BIN_FILE := /mnt/vendor/persist/WCNSS_qcom_wlan_nv.bin
$(LOCAL_BUILT_MODULE): WCNSS_BIN_SYMLINK := $(TARGET_OUT_VENDOR)/firmware/wlan/prima/WCNSS_qcom_wlan_nv.bin

$(LOCAL_BUILT_MODULE): ACTUAL_DAT_FILE := /mnt/vendor/persist/WCNSS_wlan_dictionary.dat
$(LOCAL_BUILT_MODULE): WCNSS_DAT_SYMLINK := $(TARGET_OUT_VENDOR)/firmware/wlan/prima/WCNSS_wlan_dictionary.dat

$(LOCAL_BUILT_MODULE): $(LOCAL_PATH)/Android.mk
$(LOCAL_BUILT_MODULE):
	$(hide) echo "Making symlinks for wifi"
	$(hide) mkdir -p $(dir $@)
	$(hide) mkdir -p $(dir $(WCNSS_INI_SYMLINK))
	$(hide) rm -rf $@
	$(hide) rm -rf $(WCNSS_INI_SYMLINK)
	$(hide) ln -sf $(ACTUAL_INI_FILE) $(WCNSS_INI_SYMLINK)
	$(hide) rm -rf $(WCNSS_BIN_SYMLINK)
	$(hide) ln -sf $(ACTUAL_BIN_FILE) $(WCNSS_BIN_SYMLINK)
	$(hide) rm -rf $(WCNSS_DAT_SYMLINK)
	$(hide) ln -sf $(ACTUAL_DAT_FILE) $(WCNSS_DAT_SYMLINK)
	$(hide) touch $@

include $(call all-makefiles-under,$(LOCAL_PATH))

FIRMWARE_MOUNT_POINT := $(TARGET_OUT_VENDOR)/firmware_mnt
DSP_MOUNT_POINT := $(TARGET_OUT_VENDOR)/dsp
PERSIST_MOUNT_POINT := $(TARGET_ROOT_OUT)/persist

$(FIRMWARE_MOUNT_POINT):
	@echo "Creating $(FIRMWARE_MOUNT_POINT)"
	@mkdir -p $(TARGET_OUT_VENDOR)/firmware_mnt

$(DSP_MOUNT_POINT):
	@echo "Creating $(DSP_MOUNT_POINT)"
	@mkdir -p $(TARGET_OUT_VENDOR)/dsp

$(shell mkdir -p $(TARGET_OUT_VENDOR)/lib/dsp)

$(PERSIST_MOUNT_POINT):
	@echo "Creating $(PERSIST_MOUNT_POINT)"

ALL_DEFAULT_INSTALLED_MODULES += $(FIRMWARE_MOUNT_POINT) $(DSP_MOUNT_POINT) $(PERSIST_MOUNT_POINT)

IMS_LIBS := libimscamera_jni.so libimsmedia_jni.so

IMS_SYMLINKS := $(addprefix $(TARGET_OUT_SYSTEM_EXT_APPS_PRIVILEGED)/ims/lib/arm64/,$(notdir $(IMS_LIBS)))
$(IMS_SYMLINKS): $(LOCAL_INSTALLED_MODULE)
	@echo "IMS lib link: $@"
	@mkdir -p $(dir $@)
	@rm -rf $@
	$(hide) ln -sf /system/system_ext/lib64/$(notdir $@) $@

ALL_DEFAULT_INSTALLED_MODULES += $(IMS_SYMLINKS)

CNE_LIBS := libvndfwk_detect_jni.qti.so

CNE_SYMLINKS := $(addprefix $(TARGET_OUT_VENDOR_APPS)/CneApp/lib/arm64/vendor/lib64/,$(notdir $(CNE_LIBS)))
$(CNE_SYMLINKS): $(LOCAL_INSTALLED_MODULE)
	@echo "CNE lib link: $@"
	@mkdir -p $(dir $@)
	@rm -rf $@
	$(hide) ln -sf /vendor/lib64/$(notdir $@) $@

ALL_DEFAULT_INSTALLED_MODULES += $(CNE_SYMLINKS)

RFS_MSM_ADSP_SYMLINKS := $(TARGET_OUT_VENDOR)/rfs/msm/adsp/
$(RFS_MSM_ADSP_SYMLINKS): $(LOCAL_INSTALLED_MODULE)
	@echo "Creating RFS MSM ADSP folder structure: $@"
	@rm -rf $@/*
	@mkdir -p $(dir $@)/readonly/vendor
	$(hide) ln -sf /data/vendor/tombstones/rfs/lpass $@/ramdumps
	$(hide) ln -sf /mnt/vendor/persist/rfs/msm/adsp $@/readwrite
	$(hide) ln -sf /mnt/vendor/persist/rfs/shared $@/shared
	$(hide) ln -sf /mnt/vendor/persist/hlos_rfs/shared $@/hlos
	$(hide) ln -sf /vendor/firmware_mnt $@/readonly/firmware
	$(hide) ln -sf /vendor/firmware $@/readonly/vendor/firmware

RFS_MSM_MPSS_SYMLINKS := $(TARGET_OUT_VENDOR)/rfs/msm/mpss/
$(RFS_MSM_MPSS_SYMLINKS): $(LOCAL_INSTALLED_MODULE)
	@echo "Creating RFS MSM MPSS folder structure: $@"
	@rm -rf $@/*
	@mkdir -p $(dir $@)/readonly/vendor
	$(hide) ln -sf /data/vendor/tombstones/rfs/modem $@/ramdumps
	$(hide) ln -sf /mnt/vendor/persist/rfs/msm/mpss $@/readwrite
	$(hide) ln -sf /mnt/vendor/persist/rfs/shared $@/shared
	$(hide) ln -sf /mnt/vendor/persist/hlos_rfs/shared $@/hlos
	$(hide) ln -sf /vendor/firmware_mnt $@/readonly/firmware
	$(hide) ln -sf /vendor/firmware $@/readonly/vendor/firmware

RFS_MSM_SLPI_SYMLINKS := $(TARGET_OUT_VENDOR)/rfs/msm/slpi/
$(RFS_MSM_SLPI_SYMLINKS): $(LOCAL_INSTALLED_MODULE)
	@echo "Creating RFS MSM SLPI folder structure: $@"
	@rm -rf $@/*
	@mkdir -p $(dir $@)/readonly/vendor
	$(hide) ln -sf /data/vendor/tombstones/rfs/slpi $@/ramdumps
	$(hide) ln -sf /mnt/vendor/persist/rfs/msm/slpi $@/readwrite
	$(hide) ln -sf /mnt/vendor/persist/rfs/shared $@/shared
	$(hide) ln -sf /mnt/vendor/persist/hlos_rfs/shared $@/hlos
	$(hide) ln -sf /vendor/firmware_mnt $@/readonly/firmware
	$(hide) ln -sf /vendor/firmware $@/readonly/vendor/firmware

ALL_DEFAULT_INSTALLED_MODULES += $(RFS_MSM_ADSP_SYMLINKS) $(RFS_MSM_MPSS_SYMLINKS) $(RFS_MSM_SLPI_SYMLINKS)

#########################################################################
# capabilityconfigstore
#########################################################################

$(shell mkdir -p $(TARGET_OUT_VENDOR)/etc/configstore)

$(shell mkdir -p $(PRODUCT_OUT)/vendor/lib/egl && pushd $(PRODUCT_OUT)/vendor/lib > /dev/null && ln -s egl/libEGL_adreno.so libEGL_adreno.so && popd > /dev/null)

$(shell mkdir -p $(PRODUCT_OUT)/vendor/lib/egl && pushd $(PRODUCT_OUT)/vendor/lib > /dev/null && ln -s egl/libq3dtools_adreno.so libq3dtools_adreno.so && popd > /dev/null)

$(shell mkdir -p $(PRODUCT_OUT)/vendor/lib/egl && pushd $(PRODUCT_OUT)/vendor/lib > /dev/null && ln -s egl/libGLESv2_adreno.so libGLESv2_adreno.so && popd > /dev/null)

$(shell mkdir -p $(PRODUCT_OUT)/vendor/lib64/egl && pushd $(PRODUCT_OUT)/vendor/lib64 > /dev/null && ln -s egl/libEGL_adreno.so libEGL_adreno.so && popd > /dev/null)

$(shell mkdir -p $(PRODUCT_OUT)/vendor/lib64/egl && pushd $(PRODUCT_OUT)/vendor/lib64 > /dev/null && ln -s egl/libq3dtools_adreno.so libq3dtools_adreno.so && popd > /dev/null)

$(shell mkdir -p $(PRODUCT_OUT)/vendor/lib64/egl && pushd $(PRODUCT_OUT)/vendor/lib64 > /dev/null && ln -s egl/libGLESv2_adreno.so libGLESv2_adreno.so && popd > /dev/null)

$(shell cp -rf $(LOCAL_PATH)/vendor/bin $(PRODUCT_OUT)/vendor)

endif
