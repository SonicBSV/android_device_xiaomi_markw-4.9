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
#include <aidl/android/hardware/power/SessionTag.h>

#include <chrono>

namespace aidl::google::hardware::power::impl::pixel {

// The App Hint Descriptor struct manages information necessary
// to calculate the next uclamp min value from the PID function
// and is separate so that it can be used as a pointer for
// easily passing to the pid function
struct AppHintDesc {
    AppHintDesc(int64_t sessionId, int32_t tgid, int32_t uid, const std::vector<int32_t> &threadIds,
                android::hardware::power::SessionTag tag, std::chrono::nanoseconds pTargetNs)
        : sessionId(sessionId),
          tgid(tgid),
          uid(uid),
          targetNs(pTargetNs),
          thread_ids(threadIds),
          tag(tag),
          pidControlVariable(0),
          is_active(true),
          update_count(0),
          integral_error(0),
          previous_error(0) {}

    std::string toString() const;
    int64_t sessionId{0};
    const int32_t tgid;
    const int32_t uid;
    std::chrono::nanoseconds targetNs;
    std::vector<int32_t> thread_ids;
    android::hardware::power::SessionTag tag;
    int pidControlVariable;
    // status
    std::atomic<bool> is_active;
    // pid
    uint64_t update_count = 0;
    int64_t integral_error = 0;
    int64_t previous_error = 0;
};

}  // namespace aidl::google::hardware::power::impl::pixel
