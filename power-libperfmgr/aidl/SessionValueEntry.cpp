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

#include "SessionValueEntry.h"

#include <sstream>

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

std::ostream &SessionValueEntry::dump(std::ostream &os) const {
    auto timeNow = std::chrono::steady_clock::now();
    os << "ID.Min.Act(" << idString;
    if (votes) {
        UclampRange uclampRange;
        votes->getUclampRange(uclampRange, timeNow);
        os << ", " << uclampRange.uclampMin;
        os << "-" << uclampRange.uclampMax;
    } else {
        os << ", votes nullptr";
    }
    os << ", " << isActive;
    auto totalFrames = hBoostModeDist.lightModeFrames + hBoostModeDist.moderateModeFrames +
                       hBoostModeDist.severeModeFrames;
    os << ", HBoost:"
       << (totalFrames <= 0 ? 0 : (hBoostModeDist.lightModeFrames * 10000 / totalFrames / 100.0))
       << "%-"
       << (totalFrames <= 0 ? 0 : (hBoostModeDist.moderateModeFrames * 10000 / totalFrames / 100.0))
       << "%-"
       << (totalFrames <= 0 ? 0 : (hBoostModeDist.severeModeFrames * 10000 / totalFrames / 100.0))
       << "%-" << totalFrames << ", ";
    os << sessFrameBuckets.toString() << ", ";
    return os;
}

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
