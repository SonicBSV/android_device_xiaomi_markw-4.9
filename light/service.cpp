/*
 * Copyright (C) 2018 The LineageOS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "android.hardware.light@2.0-service.xiaomi_markw"

#include <android-base/logging.h>
#include <hidl/HidlTransportSupport.h>
#include <utils/Errors.h>

#include "Light.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

// libhwbinder:
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;

// Generated HIDL files
using android::hardware::light::V2_0::ILight;
using android::hardware::light::V2_0::implementation::Light;

const static std::string kLcdBacklightPath = "/sys/class/leds/lcd-backlight/brightness";
const static std::string kLcdMaxBacklightPath = "/sys/class/leds/lcd-backlight/max_brightness";
const static std::string kRedLedPath = "/sys/class/leds/red/brightness";
const static std::string kGreenLedPath = "/sys/class/leds/green/brightness";
const static std::string kBlueLedPath = "/sys/class/leds/blue/brightness";
const static std::string kRedBlinkPath = "/sys/class/leds/red/blink";
const static std::string kGreenBlinkPath = "/sys/class/leds/green/blink";
const static std::string kBlueBlinkPath = "/sys/class/leds/blue/blink";
const static std::string kRedBreathPath = "/sys/class/leds/red/led_time";
const static std::string kGreenBreathPath = "/sys/class/leds/green/led_time";
const static std::string kBlueBreathPath = "/sys/class/leds/blue/led_time";

int main() {
    uint32_t old_led_driver;
    uint32_t lcdMaxBrightness = 255;

    std::ofstream lcdBacklight(kLcdBacklightPath);
    if (!lcdBacklight) {
        LOG(ERROR) << "Failed to open " << kLcdBacklightPath << ", error=" << errno
                   << " (" << strerror(errno) << ")";
        return -errno;
    }

    std::ifstream lcdMaxBacklight(kLcdMaxBacklightPath);
    if (!lcdMaxBacklight) {
        LOG(ERROR) << "Failed to open " << kLcdMaxBacklightPath << ", error=" << errno
                   << " (" << strerror(errno) << ")";
        return -errno;
    } else {
        lcdMaxBacklight >> lcdMaxBrightness;
    }

    std::ofstream redLed(kRedLedPath);
    if (!redLed) {
        LOG(ERROR) << "Failed to open " << kRedLedPath << ", error=" << errno
                   << " (" << strerror(errno) << ")";
        return -errno;
    }

    std::ofstream greenLed(kGreenLedPath);
    if (!greenLed) {
        LOG(ERROR) << "Failed to open " << kGreenLedPath << ", error=" << errno
                   << " (" << strerror(errno) << ")";
        return -errno;
    }

    std::ofstream blueLed(kBlueLedPath);
    if (!blueLed) {
        LOG(ERROR) << "Failed to open " << kBlueLedPath << ", error=" << errno
                   << " (" << strerror(errno) << ")";
        return -errno;
    }

    std::ofstream redBlink(kRedBlinkPath);
    if (!redBlink) {
        LOG(ERROR) << "Failed to open " << kRedBlinkPath << ", error=" << errno
                   << " (" << strerror(errno) << ")";
        return -errno;
    }

    std::ofstream greenBlink(kGreenBlinkPath);
    if (!greenBlink) {
        LOG(ERROR) << "Failed to open " << kGreenBlinkPath << ", error=" << errno
                   << " (" << strerror(errno) << ")";
        return -errno;
    }

    std::ofstream blueBlink(kBlueBlinkPath);
    if (!blueBlink) {
        LOG(ERROR) << "Failed to open " << kBlueBlinkPath << ", error=" << errno
                   << " (" << strerror(errno) << ")";
        return -errno;
    }

    std::ofstream redBreath;
    std::ofstream greenBreath;
    std::ofstream blueBreath;

    struct stat st;
    if (stat(kRedBreathPath.c_str(), &st) != 0){
        switch(errno) {
            case ENOENT:
            case EACCES:
                /* Old LED driver */
                old_led_driver = 1;
                LOG(INFO) << "Light: Use Old LED driver";
                break;
            default:
                LOG(ERROR) << "Failed to stat " << kRedBreathPath << ", error=" << errno
                           << " (" << strerror(errno) << ")";
                return -errno;
        }
    } else {
        /* New LED driver */
        old_led_driver = 0;
        LOG(INFO) << "Light: Use New LED driver";

        redBreath.open(kRedBreathPath);
        if (!redBreath) {
            LOG(ERROR) << "Failed to stat " << kRedBreathPath << ", error=" << errno
                       << " (" << strerror(errno) << ")";
            return -errno;
        }

        greenBreath.open(kGreenBreathPath);
        if (!greenBreath) {
            LOG(ERROR) << "Failed to open " << kGreenBreathPath << ", error=" << errno
                       << " (" << strerror(errno) << ")";
            return -errno;
        }

        blueBreath.open(kBlueBreathPath);
        if (!blueBreath) {
            LOG(ERROR) << "Failed to open " << kBlueBreathPath << ", error=" << errno
                       << " (" << strerror(errno) << ")";
            return -errno;
        }
    }

    android::sp<ILight> service = new Light(
            {std::move(lcdBacklight), lcdMaxBrightness},
            old_led_driver,
            std::move(redLed), std::move(greenLed), std::move(blueLed),
            std::move(redBlink), std::move(greenBlink), std::move(blueBlink),
            std::move(redBreath), std::move(greenBreath), std::move(blueBreath));

    configureRpcThreadpool(1, true);

    android::status_t status = service->registerAsService();

    if (status != android::OK) {
        LOG(ERROR) << "Cannot register Light HAL service";
        return 1;
    }

    LOG(INFO) << "Light HAL Ready.";
    joinRpcThreadpool();
    // Under normal cases, execution will not reach this line.
    LOG(ERROR) << "Light HAL failed to join thread pool.";
    return 1;
}
