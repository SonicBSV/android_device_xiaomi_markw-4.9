/*
 * Copyright (C) 2021-2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "IDumpable.h"
#include "Utils.h"

#include <cstdint>
#include <string>

namespace aidl {
namespace android {
namespace hardware {
namespace light {

class LedDevice : public IDumpable {
  public:
    LedDevice() = delete;
    explicit LedDevice(const std::string& name);

    std::string getName() const;

    bool exists() const;
    bool supportsBreath() const;
    bool supportsOnOffMs() const;

    bool setBrightness(uint8_t value, LightMode mode = LightMode::STATIC,
                       const BlinkConfig& blink = {});

    bool setRawBrightness(uint8_t value);
    bool setBreathEnabled(bool enabled);
    bool setBreathTiming(const BlinkConfig& blink);

    void dump(int fd) const override;

  private:
    std::string mName;
    std::string mBasePath;
    std::string mBreathNode;
    uint32_t mMaxBrightness;
    bool mHasOnOffMs;
};

}  // namespace light
}  // namespace hardware
}  // namespace android
}  // namespace aidl
