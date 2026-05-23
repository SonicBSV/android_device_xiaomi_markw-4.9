/*
 * Copyright (C) 2021-2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <string>

namespace aidl {
namespace android {
namespace hardware {
namespace light {

enum class LightMode : uint8_t {
    STATIC = 0,
    BREATH = 1,
};

struct BlinkConfig {
    int32_t onMs = 0;
    int32_t offMs = 0;
    bool valid() const { return onMs > 0 && offMs > 0; }
};

struct rgb {
    uint8_t red;
    uint8_t green;
    uint8_t blue;

    rgb();
    explicit rgb(uint32_t color);
    bool isLit() const;
    uint8_t toBrightness() const;
};

uint32_t scaleBrightness(uint8_t brightness, uint32_t maxBrightness);

template <typename T>
bool readFromFile(const std::string& path, T& value);

template <typename T>
bool writeToFile(const std::string& path, const T& value);

}  // namespace light
}  // namespace hardware
}  // namespace android
}  // namespace aidl
