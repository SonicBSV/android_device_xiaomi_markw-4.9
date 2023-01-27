LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := RemovePackages
LOCAL_MODULE_CLASS := APPS
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := /dev/null
LOCAL_CERTIFICATE := PRESIGNED
LOCAL_OVERRIDES_PACKAGES := Camera2 arcore BetterBugStub Chrome GoogleCamera GoogleFeedback GooglePrintRecommendationService GoogleTTS Papers SafetyHubPrebuilt talkback Velvet Maps Drive PrebuiltGmail YouTube SoundAmplifierPrebuilt
LOCAL_UNINSTALLABLE_MODULE := true
include $(BUILD_PREBUILT)
