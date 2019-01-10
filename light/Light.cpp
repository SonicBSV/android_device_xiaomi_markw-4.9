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

#define LOG_TAG "LightService"

#include "Light.h"

#include <android-base/logging.h>

namespace {
using android::hardware::light::V2_0::LightState;

static constexpr int DEFAULT_MAX_BRIGHTNESS = 255;

static uint32_t rgbToBrightness(const LightState& state) {
    uint32_t color = state.color & 0x00ffffff;
    return ((77 * ((color >> 16) & 0xff)) + (150 * ((color >> 8) & 0xff)) +
            (29 * (color & 0xff))) >> 8;
}

static bool isLit(const LightState& state) {
    return (state.color & 0x00ffffff);
}

static int get_fade_index(int base, int num, int shift_fade)
{
    if (num == 0) return 0;           // we can ignore zeroes
    if ((base % num) != 0) return -1; // when values is not divisible
                                      // we cannot synchronize them
    int division = base / num;

    int max_fade = 6; // 2^6, can be setted to 7
    for (int i = 0; i <= max_fade; i++) {
        if (division == (1 << i)) // values need to be divided with 2^x
            return i + shift_fade; // shift_fade increases default fade value
    }

    return -1;
}

// return one of [0, 16, 32, 64, 128, 256, ...]
static int map_to_grid(int value, int interval)
{
    const int above_divider = 2;
    const int zero_like     = 12;
    int       map_value     = 16;

    if (value < zero_like) return 0;
    while (map_value <= interval) {
        int above = map_value / above_divider;
        if (value <= map_value + above)
            return map_value;
        map_value *= 2;
    }
    return map_value;
}

static void adapt_colors_for_blink(int* red, int* green, int* blue)
{
    int part[] = {(*red), (*green), (*blue)}; // color components
    const int size     = sizeof(part)/sizeof(int); // 3
    const int interval = 256;
    int max, min, base;

    // find max element
    max = part[0];
    for (int i = 0; i < size; i++)
        if (max < part[i]) max = part[i];

    if (max == 0) return; // 0 0 0

    base = 2 * interval;
    for (int i = 0; i < size; i++) {
        part[i] = (interval * part[i]) / max;      // scale to specified interval
        part[i] = map_to_grid(part[i], interval);  // round off values to the grid in specified interval
        if ((part[i] > 0) && (part[i] < base)) base = part[i];
    }

    if (base <= interval) {
        min = (max * base) / interval;
        for (int i = 0; i < size; i++) part[i] = min * (part[i] / base);
    }

    // result output
    *red   = part[0];
    *green = part[1];
    *blue  = part[2];
}

}  // anonymous namespace

namespace android {
namespace hardware {
namespace light {
namespace V2_0 {
namespace implementation {

Light::Light(std::pair<std::ofstream, uint32_t>&& lcd_backlight,
             uint32_t old_led_driver,
             std::ofstream&& red_led, std::ofstream&& green_led, std::ofstream&& blue_led,
             std::ofstream&& red_blink, std::ofstream&& green_blink, std::ofstream&& blue_blink,
             std::ofstream&& red_breath, std::ofstream&& green_breath, std::ofstream&& blue_breath)
    : mLcdBacklight(std::move(lcd_backlight)),
      mOldLedDriver(old_led_driver),
      mRedLed(std::move(red_led)),
      mGreenLed(std::move(green_led)),
      mBlueLed(std::move(blue_led)),
      mRedBlink(std::move(red_blink)),
      mGreenBlink(std::move(green_blink)),
      mBlueBlink(std::move(blue_blink)),
      mRedBreath(std::move(red_breath)),
      mGreenBreath(std::move(green_breath)),
      mBlueBreath(std::move(blue_breath)) {
    auto attnFn(std::bind(&Light::setAttentionLight, this, std::placeholders::_1));
    auto backlightFn(std::bind(&Light::setLcdBacklight, this, std::placeholders::_1));
    auto batteryFn(std::bind(&Light::setBatteryLight, this, std::placeholders::_1));
    auto buttonsFn(std::bind(&Light::setButtonsBacklight, this, std::placeholders::_1));
    auto notifFn(std::bind(&Light::setNotificationLight, this, std::placeholders::_1));
    mLights.emplace(std::make_pair(Type::ATTENTION, attnFn));
    mLights.emplace(std::make_pair(Type::BACKLIGHT, backlightFn));
    mLights.emplace(std::make_pair(Type::BATTERY, batteryFn));
    mLights.emplace(std::make_pair(Type::BUTTONS, buttonsFn));
    mLights.emplace(std::make_pair(Type::NOTIFICATIONS, notifFn));
}

// Methods from ::android::hardware::light::V2_0::ILight follow.
Return<Status> Light::setLight(Type type, const LightState& state) {
    auto it = mLights.find(type);

    if (it == mLights.end()) {
        return Status::LIGHT_NOT_SUPPORTED;
    }

    it->second(state);

    return Status::SUCCESS;
}

Return<void> Light::getSupportedTypes(getSupportedTypes_cb _hidl_cb) {
    std::vector<Type> types;

    for (auto const& light : mLights) {
        types.push_back(light.first);
    }

    _hidl_cb(types);

    return Void();
}

void Light::setAttentionLight(const LightState& state) {
    std::lock_guard<std::mutex> lock(mLock);
    mAttentionState = state;
    setSpeakerBatteryLightLocked();
}

void Light::setLcdBacklight(const LightState& state) {
    std::lock_guard<std::mutex> lock(mLock);

    uint32_t brightness = rgbToBrightness(state);

    // If max panel brightness is not the default (255),
    // apply linear scaling across the accepted range.
    if (mLcdBacklight.second != DEFAULT_MAX_BRIGHTNESS) {
        int old_brightness = brightness;
        brightness = brightness * mLcdBacklight.second / DEFAULT_MAX_BRIGHTNESS;
        LOG(VERBOSE) << "scaling brightness " << old_brightness << " => " << brightness;
    }

    mLcdBacklight.first << brightness << std::endl;
}

void Light::setButtonsBacklight(const LightState& state) {
    // We have no buttons light management, so do nothing.
    // This function required to shut up warnings about missing functionality.
    (void)state;
}

void Light::setBatteryLight(const LightState& state) {
    std::lock_guard<std::mutex> lock(mLock);
    mBatteryState = state;
    setSpeakerBatteryLightLocked();
}

void Light::setNotificationLight(const LightState& state) {
    std::lock_guard<std::mutex> lock(mLock);
    mNotificationState = state;
    setSpeakerBatteryLightLocked();
}

void Light::setSpeakerBatteryLightLocked() {
    if (isLit(mNotificationState)) {
        setSpeakerLightLocked(mNotificationState);
    } else if (isLit(mAttentionState)) {
        setSpeakerLightLocked(mAttentionState);
    } else if (isLit(mBatteryState)) {
        setSpeakerLightLocked(mBatteryState);
    } else {
        // Lights off
        LOG(VERBOSE) << "Light::setSpeakerBatteryLightLocked: Lights off";
        mRedLed     << 0 << std::endl;
        mGreenLed   << 0 << std::endl;
        mBlueLed    << 0 << std::endl;
        mRedBlink   << 0 << std::endl;
        mGreenBlink << 0 << std::endl;
        mBlueBlink  << 0 << std::endl;
    }
}

void Light::setSpeakerLightLocked(const LightState& state) {
    int red, green, blue, blink, max;
    int red_fade, green_fade, blue_fade;
    int onMs, offMs;
    uint32_t colorRGB = state.color;

    switch (state.flashMode) {
        case Flash::TIMED:
            onMs = state.flashOnMs;
            offMs = state.flashOffMs;
            break;
        case Flash::NONE:
        default:
            onMs = 0;
            offMs = 0;
            break;
    }

    red = (colorRGB >> 16) & 0xff;
    green = (colorRGB >> 8) & 0xff;
    blue = colorRGB & 0xff;
    blink = onMs > 0 && offMs > 0;

    LOG(VERBOSE) << "Light::setSpeakerLightLocked:"
                 << " r " << red << ", g " << green << ", b " << blue
                 << ", blink " << blink;

    // Disable all blinking to start
    mRedLed   << 0 << std::endl;
    mGreenLed << 0 << std::endl;
    mBlueLed  << 0 << std::endl;

    if (mOldLedDriver) {
        /* Old LED driver */

        /* first brightness, then blink, otherwise blinking will be disabled */
        mRedLed   << red   << std::endl;
        mGreenLed << green << std::endl;
        mBlueLed  << blue  << std::endl;

        if (blink) {
            mRedBlink   << (red   ? 1 : 0) << std::endl;
            mGreenBlink << (green ? 1 : 0) << std::endl;
            mBlueBlink  << (blue  ? 1 : 0) << std::endl;
        }
    } else {
        /* New LED driver */
        if (blink) {
            adapt_colors_for_blink(&red, &green, &blue);

            // In our case, use the settings in the driver led range values
            // to do: refactor intervals
            if (onMs < 1000)
                onMs = 1000;
            else if (onMs > 6000)
                onMs = 6000;

            if (offMs < 1000)
                offMs = 1000;
            else if (offMs > 7000)
                offMs = 7000;

            // Indexes [0: 0.13, 1:0.26, 2: 0.52, 3:1.04, 4: 2.08, 5: 4.16, 6: 8.32, 7: 16.64]
            // max fade grid     256     128      64      32       16       8        4
            // to do: match max fade element to grid
            max = (red > green) ? ((red > blue) ? red : blue) : ((green > blue) ? green : blue);
            red_fade   = get_fade_index(max, red,   1);
            green_fade = get_fade_index(max, green, 1);
            blue_fade  = get_fade_index(max, blue,  1);

            LOG(VERBOSE) << "Light::setSpeakerLightLocked:"
                         << " rf " << red_fade << ", gf " << green_fade << ", bf " << blue_fade;

            if (red_fade == -1 || green_fade == -1 || blue_fade == -1) { // to do: remove
                LOG(VERBOSE) << "Light::setSpeakerLightLocked: LED ERROR";
                red = green = blue = 127;
                red_fade = green_fade = blue_fade = 2; // 0.52 => 0.26 for 127 color
            }
        } else {
            onMs = 3000; offMs = 4000;
            red_fade = green_fade = blue_fade = 2; // 0.52 => 0.26 for 127 color
        }

        // first configure fade & blinking than setup brightness */
        mRedBreath   << red_fade   << " " << (int)(onMs/1000) << " " << red_fade   << " " << (int)(offMs/1000) << std::endl;
        mGreenBreath << green_fade << " " << (int)(onMs/1000) << " " << green_fade << " " << (int)(offMs/1000) << std::endl;
        mBlueBreath  << blue_fade  << " " << (int)(onMs/1000) << " " << blue_fade  << " " << (int)(offMs/1000) << std::endl;

        mRedBlink   << (blink && red   ? 1 : 0) << std::endl;
        mGreenBlink << (blink && green ? 1 : 0) << std::endl;
        mBlueBlink  << (blink && blue  ? 1 : 0) << std::endl;

        mRedLed   << red   << std::endl;
        mGreenLed << green << std::endl;
        mBlueLed  << blue  << std::endl;
    }
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace light
}  // namespace hardware
}  // namespace android
