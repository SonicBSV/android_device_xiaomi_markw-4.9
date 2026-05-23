/*
 * Copyright (C) 2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "RgbLedDevice.h"

#include <android-base/logging.h>

#define LOG_TAG "RgbLedDevice"

namespace aidl {
namespace android {
namespace hardware {
namespace light {

RgbLedDevice::RgbLedDevice(LedDevice r, LedDevice g, LedDevice b)
    : mR(r), mG(g), mB(b),
      mHasR(r.exists()), mHasG(g.exists()), mHasB(b.exists()) {}

bool RgbLedDevice::exists() const {
    return mHasR || mHasG || mHasB;
}

bool RgbLedDevice::supportsBreath() const {
    if (mHasR && !mR.supportsBreath()) return false;
    if (mHasG && !mG.supportsBreath()) return false;
    if (mHasB && !mB.supportsBreath()) return false;
    return true;
}

bool RgbLedDevice::setBrightness(const rgb& color, LightMode mode, const BlinkConfig& blink) {
    // Force STATIC to avoid rainbow effect on AW2013
    (void)mode;
    (void)blink;

    bool ok = true;

    // Disable breath on all channels
    if (mHasR) ok &= mR.setBreathEnabled(false);
    if (mHasG) ok &= mG.setBreathEnabled(false);
    if (mHasB) ok &= mB.setBreathEnabled(false);

    // Set brightness (STATIC only)
    if (mHasR) ok &= mR.setRawBrightness(color.red);
    if (mHasG) ok &= mG.setRawBrightness(color.green);
    if (mHasB) ok &= mB.setRawBrightness(color.blue);

    return ok;
}

void RgbLedDevice::dump(int fd) const {
    dprintf(fd, "exists=%d, supportsBreath=%d\n", exists(), supportsBreath());
    if (mHasR) { dprintf(fd, "  R: "); mR.dump(fd); dprintf(fd, "\n"); }
    if (mHasG) { dprintf(fd, "  G: "); mG.dump(fd); dprintf(fd, "\n"); }
    if (mHasB) { dprintf(fd, "  B: "); mB.dump(fd); dprintf(fd, "\n"); }
}

}  // namespace light
}  // namespace hardware
}  // namespace android
}  // namespace aidl
