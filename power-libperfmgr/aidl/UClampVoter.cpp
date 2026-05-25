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

#include "UClampVoter.h"

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

static void confine(UclampRange &uclampRange, const CpuVote &cpu_vote,
                    std::chrono::steady_clock::time_point t) {
    if (!cpu_vote.isTimeInRange(t)) {
        return;
    }
    uclampRange.uclampMin = std::max(uclampRange.uclampMin, cpu_vote.mUclampRange.uclampMin);
    uclampRange.uclampMax = std::min(uclampRange.uclampMax, cpu_vote.mUclampRange.uclampMax);
}

std::ostream &operator<<(std::ostream &o, const UclampRange &uc) {
    o << "[" << uc.uclampMin << "," << uc.uclampMax << "]";
    return o;
}

Votes::Votes() {}

constexpr static auto gpu_vote_id = static_cast<int>(AdpfVoteType::GPU_CAPACITY);

static inline bool isGpuVote(int type_raw) {
    AdpfVoteType const type = static_cast<AdpfVoteType>(type_raw);
    return type == AdpfVoteType::GPU_CAPACITY || type == AdpfVoteType::GPU_LOAD_UP ||
           type == AdpfVoteType::GPU_LOAD_DOWN || type == AdpfVoteType::GPU_LOAD_RESET;
}

void Votes::add(int id, CpuVote const &vote) {
    if (!isGpuVote(id)) {
        mCpuVotes[id] = vote;
    }
}

std::optional<Cycles> Votes::getGpuCapacityRequest(std::chrono::steady_clock::time_point t) const {
    std::optional<Cycles> res = std::nullopt;

    constexpr AdpfVoteType gpu_capacity_hints[] = {
            AdpfVoteType::GPU_CAPACITY,
            AdpfVoteType::GPU_LOAD_UP,
    };
    for (auto const hint : gpu_capacity_hints) {
        auto it = mGpuVotes.find(static_cast<int>(hint));
        if (it != mGpuVotes.end() && it->second.isTimeInRange(t)) {
            res = res.value_or(Cycles(0)) + it->second.mCapacity;
        }
    }

    return res;
}

void Votes::add(int id, GpuVote const &vote) {
    if (isGpuVote(id)) {
        mGpuVotes[id] = vote;
    }
}

void Votes::updateDuration(int voteId, std::chrono::nanoseconds durationNs) {
    if (isGpuVote(voteId)) {
        auto const it = mGpuVotes.find(voteId);
        if (it != mGpuVotes.end()) {
            it->second.updateDuration(durationNs);
        }
        return;
    }

    auto const voteItr = mCpuVotes.find(voteId);
    if (voteItr != mCpuVotes.end()) {
        voteItr->second.updateDuration(durationNs);
    }
}

void Votes::getUclampRange(UclampRange &uclampRange,
                           std::chrono::steady_clock::time_point t) const {
    for (auto it = mCpuVotes.begin(); it != mCpuVotes.end(); it++) {
        auto timings_it = mCpuVotes.find(it->first);
        confine(uclampRange, it->second, t);
    }
}

bool Votes::anyTimedOut(std::chrono::steady_clock::time_point t) const {
    for (const auto &v : mGpuVotes) {
        if (!v.second.isTimeInRange(t)) {
            return true;
        }
    }

    for (const auto &v : mCpuVotes) {
        if (!v.second.isTimeInRange(t)) {
            return true;
        }
    }
    return false;
}

bool Votes::allTimedOut(std::chrono::steady_clock::time_point t) const {
    for (const auto &v : mGpuVotes) {
        if (v.second.isTimeInRange(t)) {
            return false;
        }
    }

    for (const auto &v : mCpuVotes) {
        if (v.second.isTimeInRange(t)) {
            return false;
        }
    }
    return true;
}

bool Votes::remove(int voteId) {
    if (isGpuVote(voteId)) {
        auto const it = mGpuVotes.find(voteId);
        if (it != mGpuVotes.end()) {
            mGpuVotes.erase(it);
            return true;
        }
        return false;
    }

    auto const it = mCpuVotes.find(voteId);
    if (it != mCpuVotes.end()) {
        mCpuVotes.erase(it);
        return true;
    }

    return false;
}

bool Votes::setUseVote(int voteId, bool active) {
    if (isGpuVote(voteId)) {
        auto const itr = mGpuVotes.find(voteId);
        if (itr == mGpuVotes.end()) {
            return false;
        }
        itr->second.setActive(active);
        return true;
    }

    auto const itr = mCpuVotes.find(voteId);
    if (itr == mCpuVotes.end()) {
        return false;
    }
    itr->second.setActive(active);
    return true;
}

size_t Votes::size() const {
    return mCpuVotes.size() + mGpuVotes.size();
}

bool Votes::voteIsActive(int voteId) const {
    if (isGpuVote(voteId)) {
        auto const itr = mGpuVotes.find(voteId);
        if (itr == mGpuVotes.end()) {
            return false;
        }
        return itr->second.active();
    }

    auto const itr = mCpuVotes.find(voteId);
    if (itr == mCpuVotes.end()) {
        return false;
    }
    return itr->second.active();
}

std::chrono::steady_clock::time_point Votes::voteTimeout(int voteId) const {
    if (isGpuVote(voteId)) {
        auto const itr = mGpuVotes.find(voteId);
        if (itr == mGpuVotes.end()) {
            return std::chrono::steady_clock::time_point{};
        }
        return itr->second.startTime() + itr->second.durationNs();
    }

    auto const itr = mCpuVotes.find(voteId);
    if (itr == mCpuVotes.end()) {
        return std::chrono::steady_clock::time_point{};
    }
    return itr->second.startTime() + itr->second.durationNs();
}

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
