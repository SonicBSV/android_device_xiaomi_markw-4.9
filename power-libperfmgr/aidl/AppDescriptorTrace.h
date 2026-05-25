/*
 * Copyright 2023 The Android Open Source Project
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

#include <aidl/android/hardware/power/SessionMode.h>
#include <android-base/stringprintf.h>

#include <string>

#include "AdpfTypes.h"

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

// The App Hint Descriptor struct manages information necessary
// to calculate the next uclamp min value from the PID function
// and is separate so that it can be used as a pointer for
// easily passing to the pid function
struct AppDescriptorTrace {
    AppDescriptorTrace(const std::string &idString) {
        using ::android::base::StringPrintf;
        trace_pid_err = StringPrintf("adpf.%s-%s", idString.c_str(), "pid.err");
        trace_pid_integral = StringPrintf("adpf.%s-%s", idString.c_str(), "pid.integral");
        trace_pid_derivative = StringPrintf("adpf.%s-%s", idString.c_str(), "pid.derivative");
        trace_pid_pOut = StringPrintf("adpf.%s-%s", idString.c_str(), "pid.pOut");
        trace_pid_iOut = StringPrintf("adpf.%s-%s", idString.c_str(), "pid.iOut");
        trace_pid_dOut = StringPrintf("adpf.%s-%s", idString.c_str(), "pid.dOut");
        trace_pid_output = StringPrintf("adpf.%s-%s", idString.c_str(), "pid.output");
        trace_target = StringPrintf("adpf.%s-%s", idString.c_str(), "target");
        trace_active = StringPrintf("adpf.%s-%s", idString.c_str(), "active");
        trace_add_threads = StringPrintf("adpf.%s-%s", idString.c_str(), "add_threads");
        trace_actl_last = StringPrintf("adpf.%s-%s", idString.c_str(), "act_last");
        trace_min = StringPrintf("adpf.%s-%s", idString.c_str(), "min");
        trace_batch_size = StringPrintf("adpf.%s-%s", idString.c_str(), "batch_size");
        trace_hint_count = StringPrintf("adpf.%s-%s", idString.c_str(), "hint_count");
        trace_hint_overtime = StringPrintf("adpf.%s-%s", idString.c_str(), "hint_overtime");
        trace_is_first_frame = StringPrintf("adpf.%s-%s", idString.c_str(), "is_first_frame");
        // traces for heuristic boost
        trace_avg_duration = StringPrintf("adpf.%s-%s", idString.c_str(), "hboost.avgDuration");
        trace_hboost_janky_level =
                StringPrintf("adpf.%s-%s", idString.c_str(), "hboost.jankyLevel");
        trace_low_frame_rate =
                StringPrintf("adpf.%s-%s", idString.c_str(), "hboost.isLowFrameRate");
        trace_max_duration = StringPrintf("adpf.%s-%s", idString.c_str(), "hboost.maxDuration");
        trace_missed_cycles =
                StringPrintf("adpf.%s-%s", idString.c_str(), "hboost.numOfMissedCycles");
        trace_uclamp_min_ceiling =
                StringPrintf("adpf.%s-%s", idString.c_str(), "hboost.uclampMinCeiling");
        trace_uclamp_min_floor =
                StringPrintf("adpf.%s-%s", idString.c_str(), "hboost.uclampMinFloor");
        trace_hboost_pid_pu = StringPrintf("adpf.%s-%s", idString.c_str(), "hboost.uclampPidPu");

        for (size_t i = 0; i < trace_modes.size(); ++i) {
            trace_modes[i] = StringPrintf(
                    "adpf.%s-%s_mode", idString.c_str(),
                    toString(static_cast<aidl::android::hardware::power::SessionMode>(i)).c_str());
        }
        for (size_t i = 0; i < trace_votes.size(); ++i) {
            trace_votes[i] = StringPrintf("adpf.%s-vote.%s", idString.c_str(),
                                          AdpfVoteTypeToStr(static_cast<AdpfVoteType>(i)));
        }
        trace_cpu_duration = StringPrintf("adpf.%s-%s", idString.c_str(), "cpu_duration");
        trace_gpu_duration = StringPrintf("adpf.%s-%s", idString.c_str(), "gpu_duration");
        trace_gpu_capacity = StringPrintf("adpf.%s-%s", idString.c_str(), "gpu_capacity");
        trace_game_mode_fps = "adpf.sf.gameModeFPS";
        trace_game_mode_fps_jitters = "adpf.sf.gameModeFPSJitters";
    }

    // Trace values
    std::string trace_pid_err;
    std::string trace_pid_integral;
    std::string trace_pid_derivative;
    std::string trace_pid_pOut;
    std::string trace_pid_iOut;
    std::string trace_pid_dOut;
    std::string trace_pid_output;
    std::string trace_target;
    std::string trace_active;
    std::string trace_add_threads;
    std::string trace_actl_last;
    std::string trace_min;
    std::string trace_batch_size;
    std::string trace_hint_count;
    std::string trace_hint_overtime;
    std::string trace_is_first_frame;
    // traces for heuristic boost
    std::string trace_avg_duration;
    std::string trace_hboost_janky_level;
    std::string trace_hboost_pid_pu;
    std::string trace_low_frame_rate;
    std::string trace_max_duration;
    std::string trace_missed_cycles;
    std::string trace_uclamp_min_ceiling;
    std::string trace_uclamp_min_floor;

    std::array<std::string, enum_size<aidl::android::hardware::power::SessionMode>()> trace_modes;
    std::array<std::string, static_cast<int32_t>(AdpfVoteType::VOTE_TYPE_SIZE)> trace_votes;
    std::string trace_cpu_duration;
    std::string trace_gpu_duration;
    std::string trace_gpu_capacity;
    std::string trace_game_mode_fps;
    std::string trace_game_mode_fps_jitters;
};

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
