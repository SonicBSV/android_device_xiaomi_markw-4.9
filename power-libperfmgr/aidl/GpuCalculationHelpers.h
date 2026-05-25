/*
 * Copyright (C) 2024 The Android Open Source Project
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

#include <aidl/android/hardware/power/WorkDuration.h>

#include <chrono>

#include "PhysicalQuantityTypes.h"

using aidl::android::hardware::power::WorkDuration;

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

inline double gpu_time_attribution(std::chrono::nanoseconds total, std::chrono::nanoseconds gpu) {
    using std::literals::chrono_literals::operator""ns;
    if (total == 0ns) {
        return 0.0;
    }
    return std::chrono::duration_cast<std::chrono::duration<double, std::nano>>(gpu) / total;
}

Cycles calculate_capacity(WorkDuration observation, std::chrono::nanoseconds target,
                          Frequency gpu_frequency);

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
