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

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

/**
 * Put jank frames into buckets. The "jank" evaluation is reusing the session records jank
 * evaluation logic while here only counts the frames over 17ms. Though the current jank
 * evaluation is not exactly right for every frame at the moment, it can still provide a
 * a good sense of session's jank status. When we have more precise timeline from platform side
 * the jank evaluation logic could be updated.
 */
struct FrameBuckets {
  public:
    int64_t totalNumOfFrames{0};      // This includes jank frames and normal frames.
    int64_t numOfFrames17to25ms{0};   // Jank frames over 1 120Hz Vsync interval(8.333ms)
    int64_t numOfFrames25to34ms{0};   // Jank frames over 2 120Hz Vsync interval(16.667ms)
    int64_t numOfFrames34to67ms{0};   // Jank frames over 3 to 6 120Hz Vsync intervals.
    int64_t numOfFrames67to100ms{0};  // Jank frames between 10 Hz and 15 Hz
    int64_t numOfFramesOver100ms{0};  // Jank frames below 10 Hz.

    std::string toString() const {
        std::stringstream ss;
        ss << "JankFramesInBuckets: ";
        if (totalNumOfFrames <= 0) {
            ss << "0%-0%-0%-0%-0%-0";
            return ss.str();
        }

        ss << (numOfFrames17to25ms * 10000 / totalNumOfFrames / 100.0) << "%";
        if (numOfFrames17to25ms > 0) {
            ss << "(" << numOfFrames17to25ms << ")";
        }

        appendSingleBucketStr(numOfFrames25to34ms, totalNumOfFrames, ss);
        appendSingleBucketStr(numOfFrames34to67ms, totalNumOfFrames, ss);
        appendSingleBucketStr(numOfFrames67to100ms, totalNumOfFrames, ss);
        appendSingleBucketStr(numOfFramesOver100ms, totalNumOfFrames, ss);

        ss << "-" << totalNumOfFrames;
        return ss.str();
    }

    void addUpNewFrames(const FrameBuckets &newFrames) {
        totalNumOfFrames += newFrames.totalNumOfFrames;
        numOfFrames17to25ms += newFrames.numOfFrames17to25ms;
        numOfFrames25to34ms += newFrames.numOfFrames25to34ms;
        numOfFrames34to67ms += newFrames.numOfFrames34to67ms;
        numOfFrames67to100ms += newFrames.numOfFrames67to100ms;
        numOfFramesOver100ms += newFrames.numOfFramesOver100ms;
    }

  private:
    void appendSingleBucketStr(int64_t singleBucketFrames, int64_t totalFrames,
                               std::stringstream &ss) const {
        ss << "-" << (singleBucketFrames * 10000 / totalFrames / 100.0) << "%";
        if (singleBucketFrames > 0) {
            ss << "(" << singleBucketFrames << ")";
        }
    }
};

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
