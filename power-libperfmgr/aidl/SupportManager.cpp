/*
 * Copyright 2024 The Android Open Source Project
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

#include "SupportManager.h"

#include <perfmgr/HintManager.h>

#include <bitset>
#include <map>
#include <type_traits>

#include "AdpfTypes.h"

namespace aidl::google::hardware::power::impl::pixel {
using ::android::perfmgr::HintManager;

template <class E>
using SupportList = std::initializer_list<std::pair<const E, int32_t>>;

#define ASSERT_CAPACITY(Enum, list)                                               \
    static_assert(list.size() >= enum_size<Enum>(),                               \
                  "Enum " #Enum " is missing entries in SupportManager.");        \
    static_assert(std::is_same<const Enum, decltype(list.begin()->first)>::value, \
                  "Mismatched types on " #list);

// clang-format off
constexpr SupportList<Mode> kModeEarliestVersion {
  {Mode::DOUBLE_TAP_TO_WAKE, 1},
  {Mode::LOW_POWER, 1},
  {Mode::SUSTAINED_PERFORMANCE, 1},
  {Mode::FIXED_PERFORMANCE, 1},
  {Mode::VR, 1},
  {Mode::LAUNCH, 1},
  {Mode::EXPENSIVE_RENDERING, 1},
  {Mode::INTERACTIVE, 1},
  {Mode::DEVICE_IDLE, 1},
  {Mode::DISPLAY_INACTIVE, 1},
  {Mode::AUDIO_STREAMING_LOW_LATENCY, 1},
  {Mode::CAMERA_STREAMING_SECURE, 1},
  {Mode::CAMERA_STREAMING_LOW, 1},
  {Mode::CAMERA_STREAMING_MID, 1},
  {Mode::CAMERA_STREAMING_HIGH, 1},
  {Mode::GAME, 3},
  {Mode::GAME_LOADING, 3},
  {Mode::DISPLAY_CHANGE, 5},
  {Mode::AUTOMOTIVE_PROJECTION, 5},
};

constexpr SupportList<Boost> kBoostEarliestVersion = {
  {Boost::INTERACTION, 1},
  {Boost::DISPLAY_UPDATE_IMMINENT, 1},
  {Boost::ML_ACC, 1},
  {Boost::AUDIO_LAUNCH, 1},
  {Boost::CAMERA_LAUNCH, 1},
  {Boost::CAMERA_SHOT, 1},
};

constexpr SupportList<SessionHint> kSessionHintEarliestVersion = {
  {SessionHint::CPU_LOAD_UP, 4},
  {SessionHint::CPU_LOAD_DOWN, 4},
  {SessionHint::CPU_LOAD_RESET, 4},
  {SessionHint::CPU_LOAD_RESUME, 4},
  {SessionHint::POWER_EFFICIENCY, 4},
  {SessionHint::GPU_LOAD_UP, 5},
  {SessionHint::GPU_LOAD_DOWN, 5},
  {SessionHint::GPU_LOAD_RESET, 5},
  {SessionHint::CPU_LOAD_SPIKE, 6},
  {SessionHint::GPU_LOAD_SPIKE, 6},
};

constexpr SupportList<SessionMode> kSessionModeEarliestVersion = {
  {SessionMode::POWER_EFFICIENCY, 5},
  {SessionMode::GRAPHICS_PIPELINE, 6},
  {SessionMode::AUTO_CPU, 6},
  {SessionMode::AUTO_GPU, 6},
};

constexpr SupportList<SessionTag> kSessionTagEarliestVersion {
  {SessionTag::OTHER, 5},
  {SessionTag::SURFACEFLINGER, 5},
  {SessionTag::HWUI, 5},
  {SessionTag::GAME, 5},
  {SessionTag::APP, 5},
  {SessionTag::SYSUI, 6},
};
// clang-format on

// Make it so that this refuses to build if you add enums but don't define them here
ASSERT_CAPACITY(Mode, kModeEarliestVersion);
ASSERT_CAPACITY(Boost, kBoostEarliestVersion);
ASSERT_CAPACITY(SessionHint, kSessionHintEarliestVersion);
ASSERT_CAPACITY(SessionMode, kSessionModeEarliestVersion);
ASSERT_CAPACITY(SessionTag, kSessionTagEarliestVersion);

std::map<Mode, int32_t> kModeEarliestVersionMap = kModeEarliestVersion;
std::map<Boost, int32_t> kBoostEarliestVersionMap = kBoostEarliestVersion;
std::map<SessionHint, int32_t> kSessionHintEarliestVersionMap = kSessionHintEarliestVersion;
std::map<SessionMode, int32_t> kSessionModeEarliestVersionMap = kSessionModeEarliestVersion;
std::map<SessionTag, int32_t> kSessionTagEarliestVersionMap = kSessionTagEarliestVersion;

SupportInfo SupportManager::makeSupportInfo() {
    SupportInfo out;
    out.usesSessions = HintManager::GetInstance()->IsAdpfSupported();

    // Assume all are unsupported
    std::bitset<64> modeBits(0);
    std::bitset<64> boostBits(0);
    std::bitset<64> sessionHintBits(0);
    std::bitset<64> sessionModeBits(0);
    std::bitset<64> sessionTagBits(0);

    for (auto &&mode : ndk::enum_range<Mode>()) {
        modeBits[static_cast<int>(mode)] = modeSupported(mode);
    }
    for (auto &&boost : ndk::enum_range<Boost>()) {
        boostBits[static_cast<int>(boost)] = boostSupported(boost);
    }

    out.modes = static_cast<int64_t>(modeBits.to_ullong());
    out.boosts = static_cast<int64_t>(boostBits.to_ullong());

    // Don't check session-specific items if they aren't supported
    if (!out.usesSessions) {
        return out;
    }

    for (auto &&sessionHint : ndk::enum_range<SessionHint>()) {
        sessionHintBits[static_cast<int>(sessionHint)] = sessionHintSupported(sessionHint);
    }
    for (auto &&sessionMode : ndk::enum_range<SessionMode>()) {
        sessionModeBits[static_cast<int>(sessionMode)] = sessionModeSupported(sessionMode);
    }
    for (auto &&sessionTag : ndk::enum_range<SessionTag>()) {
        sessionTagBits[static_cast<int>(sessionTag)] = sessionTagSupported(sessionTag);
    }

    out.sessionHints = static_cast<int64_t>(sessionHintBits.to_ullong());
    out.sessionModes = static_cast<int64_t>(sessionModeBits.to_ullong());
    out.sessionTags = static_cast<int64_t>(sessionTagBits.to_ullong());

    out.compositionData = {
            .isSupported = false,
            .disableGpuFences = false,
            .maxBatchSize = 1,
            .alwaysBatch = false,
    };
    out.headroom = {
        .isCpuSupported = false,
        .isGpuSupported = false,
        .cpuMinIntervalMillis = 0,
        .gpuMinIntervalMillis = 0,
    };

    return out;
}

bool SupportManager::modeSupported(Mode type) {
    auto it = kModeEarliestVersionMap.find(type);
    if (it == kModeEarliestVersionMap.end() || IPower::version < it->second) {
        return false;
    }

    if (type == Mode::DOUBLE_TAP_TO_WAKE) {
        return true;
    }
    
    bool supported = HintManager::GetInstance()->IsHintSupported(toString(type));
    // LOW_POWER handled insides PowerHAL specifically
    if (type == Mode::LOW_POWER) {
        return true;
    }
    if (!supported && HintManager::GetInstance()->IsAdpfProfileSupported(toString(type))) {
        return true;
    }
    return supported;
}

bool SupportManager::boostSupported(Boost type) {
    auto it = kBoostEarliestVersionMap.find(type);
    if (it == kBoostEarliestVersionMap.end() || IPower::version < it->second) {
        return false;
    }
    bool supported = HintManager::GetInstance()->IsHintSupported(toString(type));
    if (!supported && HintManager::GetInstance()->IsAdpfProfileSupported(toString(type))) {
        return true;
    }
    return supported;
}

bool SupportManager::sessionHintSupported(SessionHint type) {
    auto it = kSessionHintEarliestVersionMap.find(type);
    if (it == kSessionHintEarliestVersionMap.end() || IPower::version < it->second) {
        return false;
    }
    switch (type) {
        case SessionHint::POWER_EFFICIENCY:
            return false;
        default:
            return true;
    }
}

bool SupportManager::sessionModeSupported(SessionMode type) {
    auto it = kSessionModeEarliestVersionMap.find(type);
    if (it == kSessionModeEarliestVersionMap.end() || IPower::version < it->second) {
        return false;
    }
    switch (type) {
        case SessionMode::POWER_EFFICIENCY:
            return false;
        case SessionMode::GRAPHICS_PIPELINE:
            return false;
        default:
            return true;
    }
}

bool SupportManager::sessionTagSupported(SessionTag type) {
    auto it = kSessionTagEarliestVersionMap.find(type);
    if (it == kSessionTagEarliestVersionMap.end() || IPower::version < it->second) {
        return false;
    }
    return true;
}

}  // namespace aidl::google::hardware::power::impl::pixel
