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

#include "GpuCalculationHelpers.h"

using std::literals::chrono_literals::operator""ns;
using std::chrono::duration;
using std::chrono::duration_cast;
using std::chrono::nanoseconds;

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

// until std::lerp is available...
template <typename R, typename P>
duration<R, P> lerp(duration<R, P> a, duration<R, P> b, float t) {
    auto const fa = duration_cast<duration<float, std::nano>>(a);
    auto const fb = duration_cast<duration<float, std::nano>>(b);
    return duration_cast<duration<R, P>>(fa + (fb - fa) * t);
}

// In the event that the client reports that the GPU + CPU time is less than
// the total time, expand both CPU and GPU timings so that this constraint
// holds true.
nanoseconds sanitize_timings(nanoseconds total, nanoseconds cpu, nanoseconds gpu) {
    auto const accounted_portion = cpu + gpu;
    auto const unaccounted_portion = total - accounted_portion;
    if (unaccounted_portion > 0ns) {
        auto const cpu_portion = duration_cast<duration<float, std::nano>>(cpu) / accounted_portion;
        gpu = lerp(gpu, gpu + unaccounted_portion, (1.0 - cpu_portion));
    }
    return gpu;
}

inline bool subtotal_timings_invalid(WorkDuration const &observation) {
    return observation.durationNanos < observation.gpuDurationNanos ||
           observation.durationNanos < observation.cpuDurationNanos;
}

Cycles calculate_capacity(WorkDuration observation, nanoseconds target, Frequency gpu_frequency) {
    auto const total = nanoseconds(observation.durationNanos);

    auto const overrun = duration_cast<duration<float, std::nano>>(total - target);
    if (overrun < 0ns || subtotal_timings_invalid(observation)) {
        return Cycles(0);
    }

    auto const gpu = sanitize_timings(total, nanoseconds(observation.cpuDurationNanos),
                                      nanoseconds(observation.gpuDurationNanos));

    auto const gpu_time_attribution_pct = gpu_time_attribution(total, gpu);
    auto const gpu_delta = overrun * gpu_time_attribution_pct;
    return gpu_frequency * gpu_delta;
}

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
