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

#include <unordered_map>
#include <vector>

#include "SessionValueEntry.h"

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

/**
 * Map session id to a value and link to many task ids
 * Maintain consistency between mappings
 * e.g.
 *  Sessions[sid1] -> SessionValueEntry1, [tid1, tid2]
 *  Tasks[tid1] -> [sid1]
 *  Tasks[tid2] -> [sid1]
 *  ...
 *  Sessions[sid2] -> SessionValueEntry2, [tid2, tid3]
 *  Tasks[tid1] -> [sid1]
 *  Tasks[tid2] -> [sid1, sid2]
 *  Tasks[tid3] -> [sid2]
 */
class SessionTaskMap {
  public:
    // Add a session with associated tasks to mapping
    bool add(int64_t sessionId, const SessionValueEntry &sv, const std::vector<pid_t> &taskIds);

    // Add a vote to a session
    void addVote(int64_t sessionId, int voteId, int uclampMin, int uclampMax,
                 std::chrono::steady_clock::time_point startTime,
                 std::chrono::nanoseconds durationNs);
    void addGpuVote(int64_t sessionId, int voteId, Cycles capacity,
                    std::chrono::steady_clock::time_point startTime,
                    std::chrono::nanoseconds durationNs);

    // Find session id and run callback on session value, linked tasks
    std::shared_ptr<SessionValueEntry> findSession(int64_t sessionId) const;

    void getTaskVoteRange(pid_t taskId, std::chrono::steady_clock::time_point timeNow,
                          UclampRange &range, std::optional<int32_t> &uclampMaxEfficientBase,
                          std::optional<int32_t> &uclampMaxEfficientOffset) const;
    Cycles getSessionsGpuCapacity(std::chrono::steady_clock::time_point time_point) const;

    // Find session ids given a task id if it exists
    std::vector<int64_t> getSessionIds(pid_t taskId) const;

    // Get a vec of tasks associated with a session
    std::vector<pid_t> &getTaskIds(int64_t sessionId);

    // Return true if any app session is active, false otherwise
    bool isAnyAppSessionActive(std::chrono::steady_clock::time_point timePoint) const;

    // Remove a session based on session id
    bool remove(int64_t sessionId);

    // Maintain value of session, remove old task mapping add new
    bool replace(int64_t sessionId, const std::vector<pid_t> &taskIds,
                 std::vector<pid_t> *addedThreads, std::vector<pid_t> *removedThreads);

    size_t sizeSessions() const;

    size_t sizeTasks() const;

    // Given task id, for each linked-to session id call fn
    template <typename FN>
    void forEachSessionInTask(pid_t taskId, FN fn) const {
        auto taskSessItr = mTasks.find(taskId);
        if (taskSessItr == mTasks.end()) {
            return;
        }
        for (const auto &session : taskSessItr->second) {
            auto sessionItr = mSessions.find(session->sessionId);
            if (sessionItr == mSessions.end()) {
                continue;
            }
            fn(sessionItr->first, *(sessionItr->second.val));
        }
    }

    // Iterate over all entries in session map and run callback fn
    // fn takes int64_t session id, session entry val, linked task ids
    template <typename FN>
    void forEachSessionValTasks(FN fn) const {
        for (const auto &e : mSessions) {
            fn(e.first, *(e.second.val), e.second.linkedTasks);
        }
    }

    // Returns string id of session
    const std::string &idString(int64_t sessionId) const;

    // Returns true if session id is an app session id
    bool isAppSession(int64_t sessionId) const;

    // Remove dead task-session map entry
    bool removeDeadTaskSessionMap(int64_t sessionId, pid_t taskId);

  private:
    // Internal struct to hold per-session data and linked tasks
    struct ValEntry {
        std::shared_ptr<SessionValueEntry> val;
        std::vector<pid_t> linkedTasks;
    };
    // Map session id to value
    std::unordered_map<int64_t, ValEntry> mSessions;
    // Map task id to set of session ids
    std::unordered_map<pid_t, std::vector<std::shared_ptr<SessionValueEntry>>> mTasks;
};

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
