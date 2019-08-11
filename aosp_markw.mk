#
# Copyright (C) 2016 The CyanogenMod Project
#               2017 The LineageOS Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Inherit from those products. Most specific first.
$(call inherit-product, $(SRC_TARGET_DIR)/product/core_64_bit.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base_telephony.mk)

# Inherit some common Evolution X stuff.
TARGET_BOOT_ANIMATION_RES := 1080
TARGET_BOOT_ANIMATION_RES_EVO := true
TARGET_GAPPS_ARCH := arm64
$(call inherit-product, vendor/aosp/config/common_full_phone.mk)

# Inherit from markw device
$(call inherit-product, $(LOCAL_PATH)/device.mk)

# Device identifier. This must come after all inclusions
PRODUCT_BRAND := Xiaomi
PRODUCT_NAME := aosp_markw
BOARD_VENDOR := Xiaomi
PRODUCT_MANUFACTURER := Xiaomi
PRODUCT_DEVICE := markw
PRODUCT_MODEL := Redmi 4 Prime

PRODUCT_GMS_CLIENTID_BASE := android-xiaomi

PRODUCT_BUILD_PROP_OVERRIDES += \
    PRIVATE_BUILD_DESC="walleye-user 9 PQ3A.190505.001 5373320 release-keys" \
    TARGET_DEVICE="markw" \
    DEVICE_MAINTAINERS="SonicBSV"
    
# Set BUILD_FINGERPRINT variable to be picked up by both system and vendor build.prop
BUILD_FINGERPRINT := google/walleye/walleye:9/PQ3A.190505.001/5373320:user/release-keys
