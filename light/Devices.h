/*
 * Copyright (C) 2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "BacklightDevice.h"
#include "LedDevice.h"
#include "RgbLedDevice.h"
#include "IDumpable.h"
#include "Utils.h"

#include <vector>

namespace aidl {
namespace android {
namespace hardware {
namespace light {

class Devices : public IDumpable {
  public:
    Devices();

    bool hasBacklightDevices() const;
    bool hasNotificationDevices() const;

    void setBacklightColor(const rgb& color);
    void setNotificationColor(const rgb& color, LightMode mode, const BlinkConfig& blink);

    void dump(int fd) const override;

  private:
    std::vector<BacklightDevice> mBacklights;
    std::vector<LedDevice> mBacklightLeds;
    std::vector<RgbLedDevice> mRgbNotifications;
};

}  // namespace light
}  // namespace hardware
}  // namespace android
}  // namespace aidl
