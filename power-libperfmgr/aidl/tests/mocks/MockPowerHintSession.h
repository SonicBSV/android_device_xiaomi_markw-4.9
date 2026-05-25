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

#pragma once

#include <aidl/android/hardware/power/BnPowerHintSession.h>
#include <aidl/android/hardware/power/SessionMode.h>
#include <aidl/android/hardware/power/SessionTag.h>
#include <gmock/gmock.h>

namespace aidl::google::hardware::power::mock::pixel {

class MockPowerHintSession {
  public:
    MockPowerHintSession() = default;
    ~MockPowerHintSession() = default;

    MOCK_METHOD(ndk::ScopedAStatus, close, ());
    MOCK_METHOD(ndk::ScopedAStatus, pause, ());
    MOCK_METHOD(ndk::ScopedAStatus, resume, ());
    MOCK_METHOD(ndk::ScopedAStatus, updateTargetWorkDuration, (int64_t targetDurationNanos));
    MOCK_METHOD(ndk::ScopedAStatus, reportActualWorkDuration,
                (const std::vector<android::hardware::power::WorkDuration> &actualDurations));
    MOCK_METHOD(ndk::ScopedAStatus, sendHint, (android::hardware::power::SessionHint hint));
    MOCK_METHOD(ndk::ScopedAStatus, setMode,
                (android::hardware::power::SessionMode mode, bool enabled));
    MOCK_METHOD(ndk::ScopedAStatus, setThreads, (const std::vector<int32_t> &threadIds));
    MOCK_METHOD(ndk::ScopedAStatus, getSessionConfig,
                (android::hardware::power::SessionConfig * _aidl_return));

    MOCK_METHOD(bool, isModeSet, (android::hardware::power::SessionMode mode), (const));
    MOCK_METHOD(void, dumpToStream, (std::ostream & stream));
    MOCK_METHOD(android::hardware::power::SessionTag, getSessionTag, (), (const));
};

}  // namespace aidl::google::hardware::power::mock::pixel
