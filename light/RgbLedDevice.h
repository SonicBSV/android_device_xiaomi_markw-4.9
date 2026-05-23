/*
 * Copyright (C) 2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "IDumpable.h"
#include "LedDevice.h"
#include "Utils.h"

namespace aidl {
namespace android {
namespace hardware {
namespace light {

class RgbLedDevice : public IDumpable {
  public:
    RgbLedDevice() = delete;
    RgbLedDevice(LedDevice r, LedDevice g, LedDevice b);

    bool exists() const;
    bool supportsBreath() const;

    bool setBrightness(const rgb& color, LightMode mode, const BlinkConfig& blink);

    void dump(int fd) const override;

  private:
    LedDevice mR;
    LedDevice mG;
    LedDevice mB;

    bool mHasR;
    bool mHasG;
    bool mHasB;
};

}  // namespace light
}  // namespace hardware
}  // namespace android
}  // namespace aidl
