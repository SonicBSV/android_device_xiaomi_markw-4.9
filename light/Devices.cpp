/*
 * Copyright (C) 2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "Devices.h"

#include <android-base/logging.h>

#define LOG_TAG "Devices"

namespace aidl {
namespace android {
namespace hardware {
namespace light {

static const char* kBacklightCandidates[] = { "backlight", "panel0-backlight" };
static const char* kBacklightLedCandidates[] = { "lcd-backlight" };

Devices::Devices() {
    for (const char* name : kBacklightCandidates) {
        BacklightDevice d(name);
        if (d.exists()) {
            LOG(INFO) << "Found backlight device: " << d.getName();
            mBacklights.push_back(d);
        }
    }

    for (const char* name : kBacklightLedCandidates) {
        LedDevice d(name);
        if (d.exists()) {
            LOG(INFO) << "Found backlight LED device: " << d.getName();
            mBacklightLeds.push_back(d);
        }
    }

    {
        LedDevice r("red");
        LedDevice g("green");
        LedDevice b("blue");
        RgbLedDevice rgbDev(r, g, b);
        if (rgbDev.exists()) {
            LOG(INFO) << "Found RGB LED device: red/green/blue";
            mRgbNotifications.push_back(rgbDev);
        }
    }
}

bool Devices::hasBacklightDevices() const {
    return !mBacklights.empty() || !mBacklightLeds.empty();
}

bool Devices::hasNotificationDevices() const {
    return !mRgbNotifications.empty();
}

void Devices::setBacklightColor(const rgb& color) {
    uint8_t br = color.toBrightness();
    for (auto& d : mBacklights) d.setBrightness(br);
    for (auto& d : mBacklightLeds) d.setBrightness(br, LightMode::STATIC);
}

void Devices::setNotificationColor(const rgb& color, LightMode mode, const BlinkConfig& blink) {
    for (auto& d : mRgbNotifications) {
        d.setBrightness(color, mode, blink);
    }
}

void Devices::dump(int fd) const {
    dprintf(fd, "Backlight devices:\n");
    for (const auto& d : mBacklights) { dprintf(fd, "  - "); d.dump(fd); dprintf(fd, "\n"); }

    dprintf(fd, "Backlight LED devices:\n");
    for (const auto& d : mBacklightLeds) { dprintf(fd, "  - "); d.dump(fd); dprintf(fd, "\n"); }

    dprintf(fd, "RGB notification devices:\n");
    for (const auto& d : mRgbNotifications) { dprintf(fd, "  - "); d.dump(fd); dprintf(fd, "\n"); }
}

}  // namespace light
}  // namespace hardware
}  // namespace android
}  // namespace aidl
