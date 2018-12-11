#
# Copyright (C) 2018 The LineageOS Project
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

# Release name
PRODUCT_RELEASE_NAME := markw

# Inherit build target
$(call inherit-product, build/target/product/embedded.mk)

# Inherit from those products. Most specific first
$(call inherit-product, $(SRC_TARGET_DIR)/product/core_64_bit.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base_telephony.mk)

# Inherit some common Dot OS stuff
$(call inherit-product, vendor/dot/config/common.mk)

# Inherit from whyred device
$(call inherit-product, device/xiaomi/markw/device.mk)

# Device identifier. This must come after all inclusions
PRODUCT_NAME := dot_markw
PRODUCT_DEVICE := markw
PRODUCT_MANUFACTURER := Xiaomi
PRODUCT_BRAND := Xiaomi
PRODUCT_FULL_TREBLE_OVERRIDE := true

TARGET_VENDOR_PRODUCT_NAME := markw
TARGET_VENDOR_DEVICE_NAME := markw
PRODUCT_GMS_CLIENTID_BASE := android-xiaomi
PRODUCT_BUILD_PROP_OVERRIDES += \
    PRIVATE_BUILD_DESC="markw-user 6.0.1 MMB29M V9.6.2.0.MBECNFD release-keys" \
    TARGET_DEVICE="markw" \
    DEVICE_MAINTAINERS="DarkAngelGR"
    
# Set BUILD_FINGERPRINT variable to be picked up by both system and vendor build.prop
BUILD_FINGERPRINT := Xiaomi/markw/markw:6.0.1/MMB29M/V9.6.2.0.MBECNFD:user/release-keys
