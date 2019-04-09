LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(call all-java-files-under, src)
LOCAL_PACKAGE_NAME := AdvancedControls
LOCAL_CERTIFICATE := platform
LOCAL_PRIVILEGED_MODULE := true
LOCAL_PROGUARD_ENABLED := disabled
LOCAL_DEX_PREOPT := false
LOCAL_AAPT_FLAGS := --auto-add-overlay
LOCAL_RESOURCE_DIR := $(LOCAL_PATH)/res
LOCAL_PRIVATE_PLATFORM_APIS := true

LOCAL_STATIC_ANDROID_LIBRARIES := android-support-v7-appcompat
LOCAL_STATIC_ANDROID_LIBRARIES += android-support-v4

LOCAL_RESOURCE_DIR += prebuilts/sdk/current/support/v7/appcompat/res

package_resource_overlays := $(strip \
    $(wildcard $(foreach dir, $(PRODUCT_PACKAGE_OVERLAYS), \
      $(addprefix $(dir)/, packages/apps/AdvancedControls/res))) \
    $(wildcard $(foreach dir, $(DEVICE_PACKAGE_OVERLAYS), \
      $(addprefix $(dir)/, packages/apps/AdvancedControls/res))))

LOCAL_RESOURCE_DIR := $(package_resource_overlays) $(LOCAL_RESOURCE_DIR)
LOCAL_AAPT_FLAGS += --extra-packages android.support.v7.appcompat

include $(BUILD_PACKAGE)

include $(call all-makefiles-under,$(LOCAL_PATH))
