LOCAL_PATH := $(call my-dir)

ifneq ($(filter markw,$(TARGET_DEVICE)),)

include $(call all-makefiles-under,$(LOCAL_PATH))

include $(CLEAR_VARS)

LOCAL_MODULE := wifi_symlinks
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := FAKE
LOCAL_MODULE_SUFFIX := -timestamp

include $(BUILD_SYSTEM)/base_rules.mk

$(LOCAL_BUILT_MODULE): ACTUAL_INI_FILE := /data/misc/wifi/WCNSS_qcom_cfg.ini
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
BT_FIRMWARE_MOUNT_POINT := $(TARGET_OUT_VENDOR)/bt_firmware
DSP_MOUNT_POINT := $(TARGET_OUT_VENDOR)/dsp
PERSIST_MOUNT_POINT := $(TARGET_ROOT_OUT)/persist
ALL_DEFAULT_INSTALLED_MODULES += $(FIRMWARE_MOUNT_POINT) \
				 $(BT_FIRMWARE_MOUNT_POINT) \
				 $(DSP_MOUNT_POINT) \
				 $(PERSIST_MOUNT_POINT)
$(FIRMWARE_MOUNT_POINT):
	@echo "Creating $(FIRMWARE_MOUNT_POINT)"
	@mkdir -p $(TARGET_OUT_VENDOR)/firmware_mnt
ifneq ($(TARGET_MOUNT_POINTS_SYMLINKS),false)
	@ln -sf /vendor/firmware_mnt $(TARGET_ROOT_OUT)/firmware
endif

$(BT_FIRMWARE_MOUNT_POINT):
	@echo "Creating $(BT_FIRMWARE_MOUNT_POINT)"
	@mkdir -p $(TARGET_OUT_VENDOR)/bt_firmware
ifneq ($(TARGET_MOUNT_POINTS_SYMLINKS),false)
	@ln -sf /vendor/bt_firmware $(TARGET_ROOT_OUT)/bt_firmware
endif

$(DSP_MOUNT_POINT):
	@echo "Creating $(DSP_MOUNT_POINT)"
	@mkdir -p $(TARGET_OUT_VENDOR)/dsp
ifneq ($(TARGET_MOUNT_POINTS_SYMLINKS),false)
	@ln -sf /vendor/dsp $(TARGET_ROOT_OUT)/dsp
endif

$(PERSIST_MOUNT_POINT):
	@echo "Creating $(PERSIST_MOUNT_POINT)"
ifneq ($(TARGET_MOUNT_POINTS_SYMLINKS),false)
	@ln -sf /mnt/vendor/persist $(TARGET_ROOT_OUT)/persist
endif

IMS_LIBS := libimscamera_jni.so libimsmedia_jni.so
IMS_SYMLINKS := $(addprefix $(TARGET_OUT_APPS_PRIVILEGED)/ims/lib/arm64/,$(notdir $(IMS_LIBS)))
$(IMS_SYMLINKS): $(LOCAL_INSTALLED_MODULE)
	@echo "IMS lib link: $@"
	@mkdir -p $(dir $@)
	@rm -rf $@
	$(hide) ln -sf /system/lib64/$(notdir $@) $@

ALL_DEFAULT_INSTALLED_MODULES += $(IMS_SYMLINKS)

BT_FIRMWARE := apbtfw10.tlv apbtfw11.tlv apnv10.bin apnv11.bin crbtfw11.tlv crbtfw20.tlv crbtfw21.tlv crnv11.bin crnv20.bin crnv21.bin
BT_FIRMWARE_SYMLINKS := $(addprefix $(TARGET_OUT_VENDOR)/firmware/,$(notdir $(BT_FIRMWARE)))
$(BT_FIRMWARE_SYMLINKS): $(LOCAL_INSTALLED_MODULE)
	@echo "Creating BT firmware symlink: $@"
	@mkdir -p $(dir $@)
	@rm -rf $@
	$(hide) ln -sf /bt_firmware/image/$(notdir $@) $@

ALL_DEFAULT_INSTALLED_MODULES += $(BT_FIRMWARE_SYMLINKS)

#########################################################################
# Create Folder Structure
#########################################################################
TARGET_OUT_FIRMWARE="/vendor/firmware_mnt"

$(shell rm -rf $(TARGET_OUT_VENDOR)/rfs/)

#To be enabled when prepopulation support is needed for the read_write folder
# $(shell rm -rf  $(TARGET_OUT_DATA)/rfs/)
# $(shell mkdir -p $(TARGET_OUT_DATA)/rfs/msm/mpss/)
# $(shell mkdir -p $(TARGET_OUT_DATA)/rfs/msm/adsp/)
# $(shell mkdir -p $(TARGET_OUT_DATA)/rfs/msm/slpi/)
# $(shell mkdir -p $(TARGET_OUT_DATA)/rfs/msm/cdsp/)
# $(shell mkdir -p $(TARGET_OUT_DATA)/rfs/mdm/mpss/)
# $(shell mkdir -p $(TARGET_OUT_DATA)/rfs/mdm/adsp/)
# $(shell mkdir -p $(TARGET_OUT_DATA)/rfs/mdm/slpi/)
# $(shell mkdir -p $(TARGET_OUT_DATA)/rfs/mdm/cdsp/)

#########################################################################
# MSM Folders
#########################################################################
$(shell mkdir -p $(TARGET_OUT_VENDOR)/rfs/msm/mpss/readonly/vendor)
$(shell mkdir -p $(TARGET_OUT_VENDOR)/rfs/msm/adsp/readonly/vendor)
$(shell mkdir -p $(TARGET_OUT_VENDOR)/rfs/msm/slpi/readonly/vendor)
$(shell mkdir -p $(TARGET_OUT_VENDOR)/rfs/msm/cdsp/readonly/vendor)

$(shell ln -s /data/vendor/tombstones/rfs/modem $(TARGET_OUT_VENDOR)/rfs/msm/mpss/ramdumps)
$(shell ln -s /mnt/vendor/persist/rfs/msm/mpss $(TARGET_OUT_VENDOR)/rfs/msm/mpss/readwrite)
$(shell ln -s /mnt/vendor/persist/rfs/shared $(TARGET_OUT_VENDOR)/rfs/msm/mpss/shared)
$(shell ln -s /mnt/vendor/persist/hlos_rfs/shared $(TARGET_OUT_VENDOR)/rfs/msm/mpss/hlos)
$(shell ln -s $(TARGET_OUT_FIRMWARE) $(TARGET_OUT_VENDOR)/rfs/msm/mpss/readonly/firmware)
$(shell ln -s /vendor/firmware $(TARGET_OUT_VENDOR)/rfs/msm/mpss/readonly/vendor/firmware)

$(shell ln -s /data/vendor/tombstones/rfs/lpass $(TARGET_OUT_VENDOR)/rfs/msm/adsp/ramdumps)
$(shell ln -s /mnt/vendor/persist/rfs/msm/adsp $(TARGET_OUT_VENDOR)/rfs/msm/adsp/readwrite)
$(shell ln -s /mnt/vendor/persist/rfs/shared $(TARGET_OUT_VENDOR)/rfs/msm/adsp/shared)
$(shell ln -s /mnt/vendor/persist/hlos_rfs/shared $(TARGET_OUT_VENDOR)/rfs/msm/adsp/hlos)
$(shell ln -s $(TARGET_OUT_FIRMWARE) $(TARGET_OUT_VENDOR)/rfs/msm/adsp/readonly/firmware)
$(shell ln -s /vendor/firmware $(TARGET_OUT_VENDOR)/rfs/msm/adsp/readonly/vendor/firmware)

$(shell ln -s /data/vendor/tombstones/rfs/slpi $(TARGET_OUT_VENDOR)/rfs/msm/slpi/ramdumps)
$(shell ln -s /mnt/vendor/persist/rfs/msm/slpi $(TARGET_OUT_VENDOR)/rfs/msm/slpi/readwrite)
$(shell ln -s /mnt/vendor/persist/rfs/shared $(TARGET_OUT_VENDOR)/rfs/msm/slpi/shared)
$(shell ln -s /mnt/vendor/persist/hlos_rfs/shared $(TARGET_OUT_VENDOR)/rfs/msm/slpi/hlos)
$(shell ln -s $(TARGET_OUT_FIRMWARE) $(TARGET_OUT_VENDOR)/rfs/msm/slpi/readonly/firmware)
$(shell ln -s /vendor/firmware $(TARGET_OUT_VENDOR)/rfs/msm/slpi/readonly/vendor/firmware)

$(shell ln -s /data/vendor/tombstones/rfs/cdsp $(TARGET_OUT_VENDOR)/rfs/msm/cdsp/ramdumps)
$(shell ln -s /mnt/vendor/persist/rfs/msm/cdsp $(TARGET_OUT_VENDOR)/rfs/msm/cdsp/readwrite)
$(shell ln -s /mnt/vendor/persist/rfs/shared $(TARGET_OUT_VENDOR)/rfs/msm/cdsp/shared)
$(shell ln -s /mnt/vendor/persist/hlos_rfs/shared $(TARGET_OUT_VENDOR)/rfs/msm/cdsp/hlos)
$(shell ln -s $(TARGET_OUT_FIRMWARE) $(TARGET_OUT_VENDOR)/rfs/msm/cdsp/readonly/firmware)
$(shell ln -s /vendor/firmware $(TARGET_OUT_VENDOR)/rfs/msm/cdsp/readonly/vendor/firmware)
#########################################################################
# MDM Folders
#########################################################################
$(shell mkdir -p $(TARGET_OUT_VENDOR)/rfs/mdm/mpss/readonly/vendor)
$(shell mkdir -p $(TARGET_OUT_VENDOR)/rfs/mdm/adsp/readonly/vendor)
$(shell mkdir -p $(TARGET_OUT_VENDOR)/rfs/mdm/tn/readonly)
$(shell mkdir -p $(TARGET_OUT_VENDOR)/rfs/mdm/slpi/readonly)
$(shell mkdir -p $(TARGET_OUT_VENDOR)/rfs/mdm/cdsp/readonly)


$(shell ln -s /data/vendor/tombstones/rfs/modem $(TARGET_OUT_VENDOR)/rfs/mdm/mpss/ramdumps)
$(shell ln -s /mnt/vendor/persist/rfs/mdm/mpss $(TARGET_OUT_VENDOR)/rfs/mdm/mpss/readwrite)
$(shell ln -s /mnt/vendor/persist/rfs/shared $(TARGET_OUT_VENDOR)/rfs/mdm/mpss/shared)
$(shell ln -s /mnt/vendor/persist/hlos_rfs/shared $(TARGET_OUT_VENDOR)/rfs/mdm/mpss/hlos)
$(shell ln -s $(TARGET_OUT_FIRMWARE) $(TARGET_OUT_VENDOR)/rfs/mdm/mpss/readonly/firmware)
$(shell ln -s /vendor/firmware $(TARGET_OUT_VENDOR)/rfs/mdm/mpss/readonly/vendor/firmware)

$(shell ln -s /data/vendor/tombstones/rfs/lpass $(TARGET_OUT_VENDOR)/rfs/mdm/adsp/ramdumps)
$(shell ln -s /mnt/vendor/persist/rfs/mdm/adsp $(TARGET_OUT_VENDOR)/rfs/mdm/adsp/readwrite)
$(shell ln -s /mnt/vendor/persist/rfs/shared $(TARGET_OUT_VENDOR)/rfs/mdm/adsp/shared)
$(shell ln -s /mnt/vendor/persist/hlos_rfs/shared $(TARGET_OUT_VENDOR)/rfs/mdm/adsp/hlos)
$(shell ln -s $(TARGET_OUT_FIRMWARE) $(TARGET_OUT_VENDOR)/rfs/mdm/adsp/readonly/firmware)
$(shell ln -s /vendor/firmware $(TARGET_OUT_VENDOR)/rfs/mdm/adsp/readonly/vendor/firmware)

$(shell ln -s /data/vendor/tombstones/rfs/slpi $(TARGET_OUT_VENDOR)/rfs/mdm/slpi/ramdumps)
$(shell ln -s /mnt/vendor/persist/rfs/mdm/slpi $(TARGET_OUT_VENDOR)/rfs/mdm/slpi/readwrite)
$(shell ln -s /mnt/vendor/persist/rfs/shared $(TARGET_OUT_VENDOR)/rfs/mdm/slpi/shared)
$(shell ln -s /mnt/vendor/persist/hlos_rfs/shared $(TARGET_OUT_VENDOR)/rfs/mdm/slpi/hlos)
$(shell ln -s $(TARGET_OUT_FIRMWARE) $(TARGET_OUT_VENDOR)/rfs/mdm/slpi/readonly/firmware)
$(shell ln -s /vendor/firmware $(TARGET_OUT_VENDOR)/rfs/mdm/slpi/readonly/vendor/firmware)

$(shell ln -s /data/vendor/tombstones/rfs/cdsp $(TARGET_OUT_VENDOR)/rfs/mdm/cdsp/ramdumps)
$(shell ln -s /mnt/vendor/persist/rfs/mdm/cdsp $(TARGET_OUT_VENDOR)/rfs/mdm/cdsp/readwrite)
$(shell ln -s /mnt/vendor/persist/rfs/shared $(TARGET_OUT_VENDOR)/rfs/mdm/cdsp/shared)
$(shell ln -s /mnt/vendor/persist/hlos_rfs/shared $(TARGET_OUT_VENDOR)/rfs/mdm/cdsp/hlos)
$(shell ln -s $(TARGET_OUT_FIRMWARE) $(TARGET_OUT_VENDOR)/rfs/mdm/cdsp/readonly/firmware)
$(shell ln -s /vendor/firmware $(TARGET_OUT_VENDOR)/rfs/mdm/cdsp/readonly/vendor/firmware)

$(shell ln -s /data/vendor/tombstones/rfs/tn $(TARGET_OUT_VENDOR)/rfs/mdm/tn/ramdumps)
$(shell ln -s /mnt/vendor/persist/rfs/mdm/tn $(TARGET_OUT_VENDOR)/rfs/mdm/tn/readwrite)
$(shell ln -s /mnt/vendor/persist/rfs/shared $(TARGET_OUT_VENDOR)/rfs/mdm/tn/shared)
$(shell ln -s /mnt/vendor/persist/hlos_rfs/shared $(TARGET_OUT_VENDOR)/rfs/mdm/tn/hlos)
$(shell ln -s $(TARGET_OUT_FIRMWARE) $(TARGET_OUT_VENDOR)/rfs/mdm/tn/readonly/firmware)
$(shell ln -s /vendor/firmware $(TARGET_OUT_VENDOR)/rfs/mdm/tn/readonly/vendor/firmware)

#########################################################################
# APQ Folders
#########################################################################
$(shell mkdir -p $(TARGET_OUT_VENDOR)/rfs/apq/gnss/readonly/vendor)

$(shell ln -s /data/vendor/tombstones/rfs/modem $(TARGET_OUT_VENDOR)/rfs/apq/gnss/ramdumps)
$(shell ln -s /mnt/vendor/persist/rfs/apq/gnss $(TARGET_OUT_VENDOR)/rfs/apq/gnss/readwrite)
$(shell ln -s /mnt/vendor/persist/rfs/shared $(TARGET_OUT_VENDOR)/rfs/apq/gnss/shared)
$(shell ln -s /mnt/vendor/persist/hlos_rfs/shared $(TARGET_OUT_VENDOR)/rfs/apq/gnss/hlos)
$(shell ln -s $(TARGET_OUT_FIRMWARE) $(TARGET_OUT_VENDOR)/rfs/apq/gnss/readonly/firmware)
$(shell ln -s /vendor/firmware $(TARGET_OUT_VENDOR)/rfs/apq/gnss/readonly/vendor/firmware)

$(shell mkdir -p $(PRODUCT_OUT)/vendor/lib/egl && pushd $(PRODUCT_OUT)/vendor/lib > /dev/null && ln -s egl/libEGL_adreno.so libEGL_adreno.so && popd > /dev/null)

$(shell mkdir -p $(PRODUCT_OUT)/vendor/lib/egl && pushd $(PRODUCT_OUT)/vendor/lib > /dev/null && ln -s egl/libq3dtools_adreno.so libq3dtools_adreno.so && popd > /dev/null)

$(shell mkdir -p $(PRODUCT_OUT)/vendor/lib/egl && pushd $(PRODUCT_OUT)/vendor/lib > /dev/null && ln -s egl/libGLESv2_adreno.so libGLESv2_adreno.so && popd > /dev/null)

$(shell mkdir -p $(PRODUCT_OUT)/vendor/lib64/egl && pushd $(PRODUCT_OUT)/vendor/lib64 > /dev/null && ln -s egl/libEGL_adreno.so libEGL_adreno.so && popd > /dev/null)

$(shell mkdir -p $(PRODUCT_OUT)/vendor/lib64/egl && pushd $(PRODUCT_OUT)/vendor/lib64 > /dev/null && ln -s egl/libq3dtools_adreno.so libq3dtools_adreno.so && popd > /dev/null)

$(shell mkdir -p $(PRODUCT_OUT)/vendor/lib64/egl && pushd $(PRODUCT_OUT)/vendor/lib64 > /dev/null && ln -s egl/libGLESv2_adreno.so libGLESv2_adreno.so && popd > /dev/null)

$(shell cp -rf $(LOCAL_PATH)/vendor/bin $(PRODUCT_OUT)/vendor)

endif
