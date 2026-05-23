/*
 * Copyright (C) 2021-2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "Devices.h"

#include <aidl/android/hardware/light/BnLights.h>
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

using ::aidl::android::hardware::light::FlashMode;
using ::aidl::android::hardware::light::HwLight;
using ::aidl::android::hardware::light::HwLightState;
using ::aidl::android::hardware::light::LightType;

namespace aidl {
namespace android {
namespace hardware {
namespace light {

class Lights : public BnLights {
  public:
    Lights();
    ~Lights() override;

    ndk::ScopedAStatus getLights(std::vector<HwLight>* lights) override;
    ndk::ScopedAStatus setLightState(int32_t id, const HwLightState& state) override;

    binder_status_t dump(int fd, const char** args, uint32_t numArgs) override;

  private:
    void updateCompositeLedLocked();
    void blinkWorker();

    Devices mDevices;
    std::vector<HwLight> mLights;

    std::mutex mMutex;
    int64_t mLastApplyMs;

    HwLightState mLastBattery{};
    HwLightState mLastNotification{};
    HwLightState mLastAttention{};

    // Software blink
    std::thread mBlinkThread;
    std::atomic<bool> mBlinkRunning{false};

    bool mBlinkEnabled = false;
    int32_t mBlinkOnMs = 0;
    int32_t mBlinkOffMs = 0;
    rgb mBlinkColor{};
};

}  // namespace light
}  // namespace hardware
}  // namespace android
}  // namespace aidl
