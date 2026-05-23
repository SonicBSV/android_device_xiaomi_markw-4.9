/*
 * Copyright (C) 2021-2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "Utils.h"

#include <android-base/logging.h>
#include <fstream>

namespace aidl {
namespace android {
namespace hardware {
namespace light {

rgb::rgb() : red(0), green(0), blue(0) {}

rgb::rgb(uint32_t color) : red(0), green(0), blue(0) {
    uint8_t a = (color >> 24) & 0xFF;
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;

    if (a > 0 && a < 0xFF) {
        r = static_cast<uint8_t>((static_cast<uint32_t>(r) * a) / 0xFF);
        g = static_cast<uint8_t>((static_cast<uint32_t>(g) * a) / 0xFF);
        b = static_cast<uint8_t>((static_cast<uint32_t>(b) * a) / 0xFF);
    }

    red = r;
    green = g;
    blue = b;
}

bool rgb::isLit() const {
    return red || green || blue;
}

static constexpr uint8_t kRw = 77;
static constexpr uint8_t kGw = 150;
static constexpr uint8_t kBw = 29;

uint8_t rgb::toBrightness() const {
    return static_cast<uint8_t>((kRw * red + kGw * green + kBw * blue) >> 8);
}

uint32_t scaleBrightness(uint8_t brightness, uint32_t maxBrightness) {
    return (static_cast<uint32_t>(brightness) * maxBrightness) / 0xFF;
}

template <typename T>
bool readFromFile(const std::string& path, T& value) {
    std::ifstream file(path);
    if (!file.is_open()) return false;
    file >> value;
    return file.good() || file.eof();
}

template <typename T>
bool writeToFile(const std::string& path, const T& value) {
    std::ofstream file(path);
    if (!file.is_open()) {
        LOG(ERROR) << "Failed to open: " << path;
        return false;
    }
    file << value;
    return !file.fail();
}

template bool readFromFile<uint32_t>(const std::string&, uint32_t&);
template bool writeToFile<uint32_t>(const std::string&, const uint32_t&);
template bool writeToFile<int>(const std::string&, const int&);
template bool writeToFile<std::string>(const std::string&, const std::string&);

}  // namespace light
}  // namespace hardware
}  // namespace android
}  // namespace aidl
