LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE             := custom_charger
LOCAL_MODULE_OWNER       := xiaomi
LOCAL_MODULE_PATH        := $(TARGET_ROOT_OUT_SYSTEM_BIN)
LOCAL_UNSTRIPPED_PATH    := $(TARGET_ROOT_OUT_SYSTEM_BIN_UNSTRIPPED)
LOCAL_SRC_FILES          := custom_charger
LOCAL_MODULE_TAGS        := optional
LOCAL_MODULE_CLASS       := EXECUTABLES
include $(BUILD_PREBUILT)

define _add-charger-image
include $$(CLEAR_VARS)
LOCAL_MODULE          := custom_charger_res_images_$(notdir $(1))
LOCAL_MODULE_STEM     := $(notdir $(1))
LOCAL_MODULE_OWNER    := xiaomi
LOCAL_MODULE_PATH     := $$(TARGET_ROOT_OUT)/res/images/charger
LOCAL_MODULE_TAGS     := optional
_img_modules          += $$(LOCAL_MODULE)
LOCAL_SRC_FILES := $1
LOCAL_MODULE_CLASS    := ETC
include $$(BUILD_PREBUILT)
endef

_img_modules :=
IMAGES_DIR := res/images/charger
_images :=
$(foreach _img, $(call find-subdir-subdir-files, "$(IMAGES_DIR)", "*.png"), \
  $(eval $(call _add-charger-image,$(_img))))

include $(CLEAR_VARS)
LOCAL_MODULE := custom_charger_res_images
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := $(_img_modules)
include $(BUILD_PHONY_PACKAGE)

_add-charger-image :=
_img_modules :=
