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

#include <aidl/android/hardware/power/WorkDuration.h>

#include <deque>
#include <optional>
#include <vector>

#include "SessionMetrics.h"

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

using aidl::android::hardware::power::WorkDuration;
class SessionRecords {
  public:
    struct CycleRecord {
        int32_t startIntervalUs{0};
        int32_t totalDurationUs{0};
        bool isMissedCycle{false};
        bool isFPSJitter{false};
    };

  public:
    SessionRecords(const int32_t maxNumOfRecords, const double jankCheckTimeFactor);
    ~SessionRecords() = default;

    void addReportedDurations(const std::vector<WorkDuration> &actualDurationsNs,
                              int64_t targetDurationNs, FrameBuckets &newFramesInBuckets,
                              bool computeFPSJitters = false);
    std::optional<int32_t> getMaxDuration();
    std::optional<int32_t> getAvgDuration();
    int32_t getNumOfRecords();
    int32_t getNumOfMissedCycles();
    bool isLowFrameRate(int32_t fpsLowRateThreshold);
    void resetRecords();
    // It will only return valid value when the computeFPSJitters is enabled while
    // calling addReportedDurations. It's mainly for game mode FPS monitoring.
    int32_t getLatestFPS() const;
    int32_t getNumOfFPSJitters() const;

  private:
    void updateFrameBuckets(int32_t frameDurationUs, bool isJankFrame,
                            FrameBuckets &framesInBuckets);

    const int32_t kMaxNumOfRecords;
    const double kJankCheckTimeFactor;
    std::vector<CycleRecord> mRecords;
    // A descending order queue to store the records' indexes.
    // It is for detecting the maximum duration.
    std::deque<int32_t> mRecordsIndQueue;
    int32_t mAvgDurationUs{0};
    int64_t mLastStartTimeNs{0};
    int32_t mLatestRecordIndex{-1};
    int32_t mNumOfMissedCycles{0};
    int32_t mNumOfFrames{0};
    int64_t mSumOfDurationsUs{0};

    // Compute the sum of start interval for the last few frames.
    // It can be beneficial for computing the FPS jitters.
    int32_t mLatestStartIntervalSumUs{0};
    int32_t mNumOfFrameFPSJitters{0};
    int32_t mAddedFramesForFPSCheck{0};
};

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
