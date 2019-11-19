# Inherit from those products. Most specific first.
$(call inherit-product, $(SRC_TARGET_DIR)/product/core_64_bit.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base_telephony.mk)
$(call inherit-product, vendor/aosp/config/common_full_phone.mk)

# Inherit from markw device
$(call inherit-product, $(LOCAL_PATH)/device.mk)

# Inherit some common stuff
TARGET_BOOT_ANIMATION_RES := 1080

# Device identifier. This must come after all inclusions
BOARD_VENDOR := Xiaomi
TARGET_VENDOR := Xiaomi
PRODUCT_BRAND := Xiaomi
PRODUCT_DEVICE := markw
PRODUCT_NAME := aosp_markw
PRODUCT_MANUFACTURER := Xiaomi
PRODUCT_MODEL := Redmi 4 Prime

PRODUCT_GMS_CLIENTID_BASE := android-xiaomi
PRODUCT_BUILD_PROP_OVERRIDES += \
    PRIVATE_BUILD_DESC="markw-user 6.0.1 MMB29M V10.2.2.0.MBECNXM release-keys"

# Set BUILD_FINGERPRINT variable to be picked up by both system and vendor build.prop
BUILD_FINGERPRINT := "Xiaomi/markw/markw:6.0.1/MMB29M/V10.2.2.0.MBECNXM:user/release-keys"
