LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.biometrics.fingerprint@2.1-service.markw
LOCAL_INIT_RC := android.hardware.biometrics.fingerprint@2.1-service.markw.rc
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SRC_FILES := \
    BiometricsFingerprint.cpp \
    service.cpp

LOCAL_SHARED_LIBRARIES := \
    libbinder \
    libcutils \
    liblog \
    libhidlbase \
    libhidltransport \
    libhardware \
    libutils \
    libhwbinder \
    android.hardware.biometrics.fingerprint@2.1

include $(BUILD_EXECUTABLE)

include $(call all-makefiles-under,$(LOCAL_PATH))
