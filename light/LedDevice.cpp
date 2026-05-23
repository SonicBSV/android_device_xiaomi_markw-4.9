/*
 * Copyright (C) 2021-2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "LedDevice.h"

#include <android-base/logging.h>
#include <fstream>
#include <sstream>

#define LOG_TAG "LedDevice"

namespace aidl {
namespace android {
namespace hardware {
namespace light {

static const std::string kBaseLedsPath = "/sys/class/leds/";
static const uint32_t kDefaultMaxBrightness = 255;

static const std::string kBrightnessNode = "brightness";
static const std::string kMaxBrightnessNode = "max_brightness";
static const std::string kOnOffMsNode = "on_off_ms";

static const char* kBreathCandidates[] = { "breath", "blink" };

LedDevice::LedDevice(const std::string& name)
    : mName(name),
      mBasePath(kBaseLedsPath + name + "/"),
      mMaxBrightness(kDefaultMaxBrightness),
      mHasOnOffMs(false) {
    readFromFile(mBasePath + kMaxBrightnessNode, mMaxBrightness);

    for (const char* n : kBreathCandidates) {
        if (std::ifstream(mBasePath + n).good()) {
            mBreathNode = n;
            break;
        }
    }

    mHasOnOffMs = std::ifstream(mBasePath + kOnOffMsNode).good();
}

std::string LedDevice::getName() const {
    return mName;
}

bool LedDevice::exists() const {
    return std::ifstream(mBasePath + kBrightnessNode).good();
}

bool LedDevice::supportsBreath() const {
    return !mBreathNode.empty();
}

bool LedDevice::supportsOnOffMs() const {
    return mHasOnOffMs;
}

bool LedDevice::setBrightness(uint8_t value, LightMode mode, const BlinkConfig& blink) {
    if (supportsBreath()) {
        (void)writeToFile(mBasePath + mBreathNode, 0);
    }

    if (!writeToFile(mBasePath + kBrightnessNode, scaleBrightness(value, mMaxBrightness))) {
        return false;
    }

    if (mode == LightMode::BREATH) {
        if (!supportsBreath()) return true;

        if (mHasOnOffMs && blink.valid()) {
            std::stringstream ss;
            ss << blink.onMs << " " << blink.offMs;
            (void)writeToFile(mBasePath + kOnOffMsNode, ss.str());
        }

        return writeToFile(mBasePath + mBreathNode, value > 0 ? 1 : 0);
    }

    return true;
}

bool LedDevice::setRawBrightness(uint8_t value) {
    return writeToFile(mBasePath + kBrightnessNode, scaleBrightness(value, mMaxBrightness));
}

bool LedDevice::setBreathEnabled(bool enabled) {
    if (!supportsBreath()) return true;
    return writeToFile(mBasePath + mBreathNode, enabled ? 1 : 0);
}

bool LedDevice::setBreathTiming(const BlinkConfig& blink) {
    if (!mHasOnOffMs || !blink.valid()) return true;

    std::stringstream ss;
    ss << blink.onMs << " " << blink.offMs;
    return writeToFile(mBasePath + kOnOffMsNode, ss.str());
}

void LedDevice::dump(int fd) const {
    dprintf(fd, "Name: %s, exists: %d, base: %s, max: %u, breathNode: %s, on_off_ms: %d",
            mName.c_str(), exists(), mBasePath.c_str(), mMaxBrightness,
            mBreathNode.c_str(), mHasOnOffMs);
}

}  // namespace light
}  // namespace hardware
}  // namespace android
}  // namespace aidl
