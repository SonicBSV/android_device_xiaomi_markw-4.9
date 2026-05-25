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

#include "SessionTaskMap.h"

#include <algorithm>
#include <sstream>

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

bool SessionTaskMap::add(int64_t sessionId, const SessionValueEntry &sv,
                         const std::vector<pid_t> &taskIds) {
    if (mSessions.find(sessionId) != mSessions.end()) {
        return false;
    }

    auto sessValPtr = std::make_shared<SessionValueEntry>();
    (*sessValPtr) = sv;
    sessValPtr->sessionId = sessionId;

    auto &sessEntry = mSessions[sessionId];
    sessEntry.val = sessValPtr;
    sessEntry.linkedTasks = taskIds;

    for (auto taskId : taskIds) {
        mTasks[taskId].push_back(sessValPtr);
    }
    return true;
}

void SessionTaskMap::addVote(int64_t sessionId, int voteId, int uclampMin, int uclampMax,
                             std::chrono::steady_clock::time_point startTime,
                             std::chrono::nanoseconds durationNs) {
    auto sessItr = mSessions.find(sessionId);
    if (sessItr == mSessions.end()) {
        return;
    }

    sessItr->second.val->votes->add(voteId,
                                    CpuVote(true, startTime, durationNs, uclampMin, uclampMax));
}

void SessionTaskMap::addGpuVote(int64_t sessionId, int voteId, Cycles capacity,
                                std::chrono::steady_clock::time_point startTime,
                                std::chrono::nanoseconds durationNs) {
    auto sessItr = mSessions.find(sessionId);
    if (sessItr == mSessions.end()) {
        return;
    }

    sessItr->second.val->votes->add(voteId, GpuVote(true, startTime, durationNs, capacity));
}

std::shared_ptr<SessionValueEntry> SessionTaskMap::findSession(int64_t sessionId) const {
    auto sessItr = mSessions.find(sessionId);
    if (sessItr == mSessions.end()) {
        return nullptr;
    }
    return sessItr->second.val;
}

void SessionTaskMap::getTaskVoteRange(pid_t taskId, std::chrono::steady_clock::time_point timeNow,
                                      UclampRange &range,
                                      std::optional<int32_t> &uclampMaxEfficientBase,
                                      std::optional<int32_t> &uclampMaxEfficientOffset) const {
    auto taskItr = mTasks.find(taskId);
    if (taskItr == mTasks.end()) {
        // Assign to default range
        range = {};
        return;
    }

    for (const auto &sessInTask : taskItr->second) {
        if (!sessInTask->isActive) {
            continue;
        }
        sessInTask->votes->getUclampRange(range, timeNow);
        if (sessInTask->isPowerEfficient && uclampMaxEfficientBase.has_value()) {
            range.uclampMax = std::min(range.uclampMax,
                                       sessInTask->votes->allTimedOut(timeNow)
                                               ? *uclampMaxEfficientBase
                                               : range.uclampMin + *uclampMaxEfficientOffset);
        }
    }
}

Cycles SessionTaskMap::getSessionsGpuCapacity(
        std::chrono::steady_clock::time_point time_point) const {
    Cycles max(0);
    for (auto const &[_, session] : mSessions) {
        max = std::max(max,
                       session.val->votes->getGpuCapacityRequest(time_point).value_or(Cycles(0)));
    }
    return max;
}

std::vector<int64_t> SessionTaskMap::getSessionIds(pid_t taskId) const {
    auto itr = mTasks.find(taskId);
    if (itr == mTasks.end()) {
        static const std::vector<int64_t> emptySessionIdVec;
        return emptySessionIdVec;
    }
    std::vector<int64_t> res;
    res.reserve(itr->second.size());
    for (const auto &i : itr->second) {
        res.push_back(i->sessionId);
    }
    return res;
}

std::vector<pid_t> &SessionTaskMap::getTaskIds(int64_t sessionId) {
    auto taskItr = mSessions.find(sessionId);
    if (taskItr == mSessions.end()) {
        static std::vector<pid_t> emptyTaskIdVec;
        return emptyTaskIdVec;
    }
    return taskItr->second.linkedTasks;
}

bool SessionTaskMap::isAnyAppSessionActive(std::chrono::steady_clock::time_point timePoint) const {
    for (auto &sessionVal : mSessions) {
        if (!sessionVal.second.val->isAppSession) {
            continue;
        }
        if (!sessionVal.second.val->isActive) {
            continue;
        }
        if (!sessionVal.second.val->votes->allTimedOut(timePoint)) {
            return true;
        }
    }
    return false;
}

bool SessionTaskMap::remove(int64_t sessionId) {
    auto sessItr = mSessions.find(sessionId);
    if (sessItr == mSessions.end()) {
        return false;
    }

    // For each task id in linked tasks need to remove the corresponding
    // task to session mapping in the task map
    for (const auto taskId : sessItr->second.linkedTasks) {
        // Used linked task ids to cleanup
        auto taskItr = mTasks.find(taskId);
        if (taskItr == mTasks.end()) {
            // Inconsisent state
            continue;
        }

        // Now lookup session id in task's set
        auto taskSessItr =
                std::find(taskItr->second.begin(), taskItr->second.end(), sessItr->second.val);
        if (taskSessItr == taskItr->second.end()) {
            // Should not happen
            continue;
        }

        // Remove session id from task map
        taskItr->second.erase(taskSessItr);
        if (taskItr->second.empty()) {
            mTasks.erase(taskItr);
        }
    }

    // Now we can safely remove session entirely since there are no more
    // mappings in task to session id
    mSessions.erase(sessItr);
    return true;
}

bool SessionTaskMap::removeDeadTaskSessionMap(int64_t sessionId, pid_t taskId) {
    auto sessItr = mSessions.find(sessionId);
    if (sessItr == mSessions.end()) {
        return false;
    }

    auto taskItr = mTasks.find(taskId);
    if (taskItr == mTasks.end()) {
        // Inconsisent state
        return false;
    }

    // Now lookup session id in task's set
    auto taskSessItr =
            std::find(taskItr->second.begin(), taskItr->second.end(), sessItr->second.val);
    if (taskSessItr == taskItr->second.end()) {
        // Should not happen
        return false;
    }

    // Remove session id from task map
    taskItr->second.erase(taskSessItr);
    if (taskItr->second.empty()) {
        mTasks.erase(taskItr);
    }

    return true;
}

bool SessionTaskMap::replace(int64_t sessionId, const std::vector<pid_t> &taskIds,
                             std::vector<pid_t> *addedThreads, std::vector<pid_t> *removedThreads) {
    auto itr = mSessions.find(sessionId);
    if (itr == mSessions.end()) {
        return false;
    }

    // Make copies of val and threads
    auto svTmp = itr->second.val;
    const auto previousTaskIds = itr->second.linkedTasks;

    // Determine newly added threads
    if (addedThreads) {
        for (auto tid : taskIds) {
            auto taskSessItr = mTasks.find(tid);
            if (taskSessItr == mTasks.end()) {
                addedThreads->push_back(tid);
            }
        }
    }

    // Remove session from mappings
    remove(sessionId);
    // Add session value and task mappings
    add(sessionId, *svTmp, taskIds);

    // Determine completely removed threads
    if (removedThreads) {
        for (auto tid : previousTaskIds) {
            auto taskSessItr = mTasks.find(tid);
            if (taskSessItr == mTasks.end()) {
                removedThreads->push_back(tid);
            }
        }
    }

    return true;
}

size_t SessionTaskMap::sizeSessions() const {
    return mSessions.size();
}

size_t SessionTaskMap::sizeTasks() const {
    return mTasks.size();
}

const std::string &SessionTaskMap::idString(int64_t sessionId) const {
    auto sessItr = mSessions.find(sessionId);
    if (sessItr == mSessions.end()) {
        static const std::string emptyString;
        return emptyString;
    }
    return sessItr->second.val->idString;
}

bool SessionTaskMap::isAppSession(int64_t sessionId) const {
    auto sessItr = mSessions.find(sessionId);
    if (sessItr == mSessions.end()) {
        return false;
    }

    return sessItr->second.val->isAppSession;
}

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
