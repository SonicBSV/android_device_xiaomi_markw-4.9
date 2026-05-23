/*
 * Copyright (C) 2021-2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "Lights.h"

#include <android-base/logging.h>
#include <chrono>

namespace aidl {
namespace android {
namespace hardware {
namespace light {

static inline int64_t nowMs() {
    auto n = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(n.time_since_epoch()).count();
}

static constexpr int64_t kDebounceMs = 50;

#define AutoLight(t) HwLight{ .id = static_cast<int32_t>(t), .ordinal = 0, .type = (t) }

Lights::Lights() : mLastApplyMs(0) {
    if (mDevices.hasBacklightDevices()) {
        mLights.push_back(AutoLight(LightType::BACKLIGHT));
    }

    if (mDevices.hasNotificationDevices()) {
        mLights.push_back(AutoLight(LightType::BATTERY));
        mLights.push_back(AutoLight(LightType::NOTIFICATIONS));
        mLights.push_back(AutoLight(LightType::ATTENTION));
    }

    mBlinkRunning.store(true);
    mBlinkThread = std::thread([this]() { this->blinkWorker(); });

    LOG(INFO) << "Lights HAL init, lights=" << mLights.size();
}

Lights::~Lights() {
    mBlinkRunning.store(false);
    if (mBlinkThread.joinable()) mBlinkThread.join();
}

ndk::ScopedAStatus Lights::getLights(std::vector<HwLight>* _aidl_return) {
    *_aidl_return = mLights;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Lights::setLightState(int32_t id, const HwLightState& state) {
    const LightType type = static_cast<LightType>(id);

    if (type == LightType::BACKLIGHT) {
        mDevices.setBacklightColor(rgb(state.color));
        return ndk::ScopedAStatus::ok();
    }

    std::lock_guard<std::mutex> lk(mMutex);

    switch (type) {
        case LightType::BATTERY:
            mLastBattery = state;
            break;
        case LightType::NOTIFICATIONS:
            mLastNotification = state;
            break;
        case LightType::ATTENTION:
            mLastAttention = state;
            break;
        default:
            return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }

    updateCompositeLedLocked();
    return ndk::ScopedAStatus::ok();
}

void Lights::updateCompositeLedLocked() {
    const bool allOff = ((mLastBattery.color & 0x00FFFFFF) == 0) &&
                        ((mLastNotification.color & 0x00FFFFFF) == 0) &&
                        ((mLastAttention.color & 0x00FFFFFF) == 0);

    const int64_t t = nowMs();
    if (!allOff && (t - mLastApplyMs) < kDebounceMs) {
        return;
    }
    mLastApplyMs = t;

    HwLightState chosen{};
    if ((mLastNotification.color & 0x00FFFFFF) != 0) {
        chosen = mLastNotification;
    } else if ((mLastAttention.color & 0x00FFFFFF) != 0) {
        chosen = mLastAttention;
    } else if ((mLastBattery.color & 0x00FFFFFF) != 0) {
        chosen = mLastBattery;
    } else {
        chosen = HwLightState{};
    }

    rgb c(chosen.color);

    bool wantBlink = (chosen.flashMode == FlashMode::TIMED || 
                      chosen.flashMode == FlashMode::HARDWARE) &&
                     ((chosen.color & 0x00FFFFFF) != 0);

    int32_t onMs = chosen.flashOnMs;
    int32_t offMs = chosen.flashOffMs;

    if (wantBlink) {
        if (onMs < 50) onMs = 1000;
        if (offMs < 50) offMs = 1000;
    }

    mBlinkEnabled = wantBlink;
    mBlinkColor = c;
    mBlinkOnMs = onMs;
    mBlinkOffMs = offMs;

    if (!mBlinkEnabled) {
        mDevices.setNotificationColor(c, LightMode::STATIC, BlinkConfig{});
    }

    LOG(INFO) << "Apply LED: rgb=(" << int(c.red) << "," << int(c.green) << "," << int(c.blue)
              << ") swBlink=" << (mBlinkEnabled ? "yes" : "no")
              << " on=" << mBlinkOnMs << " off=" << mBlinkOffMs;
}

void Lights::blinkWorker() {
    bool phaseOn = false;
    int32_t lastOn = 0;
    int32_t lastOff = 0;
    rgb lastColor;

    while (mBlinkRunning.load()) {
        bool enabled;
        int32_t onMs, offMs;
        rgb color;

        {
            std::lock_guard<std::mutex> lk(mMutex);
            enabled = mBlinkEnabled;
            onMs = mBlinkOnMs;
            offMs = mBlinkOffMs;
            color = mBlinkColor;
        }

        if (!enabled) {
            phaseOn = false;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        if (onMs != lastOn || offMs != lastOff ||
            color.red != lastColor.red || 
            color.green != lastColor.green || 
            color.blue != lastColor.blue) {
            phaseOn = true;
            lastOn = onMs;
            lastOff = offMs;
            lastColor = color;
        }

        if (phaseOn) {
            mDevices.setNotificationColor(color, LightMode::STATIC, BlinkConfig{});
            std::this_thread::sleep_for(std::chrono::milliseconds(onMs));
        } else {
            mDevices.setNotificationColor(rgb(0x00000000), LightMode::STATIC, BlinkConfig{});
            std::this_thread::sleep_for(std::chrono::milliseconds(offMs));
        }

        phaseOn = !phaseOn;
    }
}

binder_status_t Lights::dump(int fd, const char** /*args*/, uint32_t /*numArgs*/) {
    dprintf(fd, "AIDL Lights HAL (markw)\n\n");
    dprintf(fd, "Registered lights:\n");
    for (const auto& l : mLights) {
        dprintf(fd, "  - id=%d type=%d\n", l.id, static_cast<int>(l.type));
    }
    dprintf(fd, "\nDevices:\n");
    mDevices.dump(fd);
    dprintf(fd, "\n");
    dprintf(fd, "Blink:\n");
    dprintf(fd, "  enabled=%d on=%d off=%d rgb=(%d,%d,%d)\n",
            mBlinkEnabled ? 1 : 0, mBlinkOnMs, mBlinkOffMs,
            int(mBlinkColor.red), int(mBlinkColor.green), int(mBlinkColor.blue));
    return STATUS_OK;
}

}  // namespace light
}  // namespace hardware
}  // namespace android
}  // namespace aidl
