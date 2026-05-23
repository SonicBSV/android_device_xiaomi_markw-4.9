/*
 * Copyright (C) 2022-2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <string>
#include "IDumpable.h"

namespace aidl {
namespace android {
namespace hardware {
namespace light {

class BacklightDevice : public IDumpable {
  public:
    BacklightDevice() = delete;
    explicit BacklightDevice(const std::string& name);

    std::string getName() const;
    bool exists() const;
    bool setBrightness(uint8_t value);

    void dump(int fd) const override;

  private:
    std::string mName;
    std::string mBasePath;
    uint32_t mMaxBrightness;
};

}  // namespace light
}  // namespace hardware
}  // namespace android
}  // namespace aidl
