/*
 * Copyright 2021 The Android Open Source Project
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

#include <android-base/properties.h>
#include <perfmgr/HintManager.h>
#include <utils/Mutex.h>

#include <mutex>
#include <optional>

#include "AdpfTypes.h"
#include "AppHintDesc.h"
#include "BackgroundWorker.h"
#include "GpuCapacityNode.h"
#include "SessionTaskMap.h"

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

using ::android::Thread;

constexpr char kPowerHalAdpfDisableTopAppBoost[] = "vendor.powerhal.adpf.disable.hint";

template <class HintManagerT = ::android::perfmgr::HintManager>
class PowerSessionManager : public Immobile {
  public:
    ~PowerSessionManager() = default;

    // Update the current hint info
    void updateHintMode(const std::string &mode, bool enabled);
    // Add and remove power hint session
    void addPowerSession(const std::string &idString,
                         const std::shared_ptr<AppHintDesc> &sessionDescriptor,
                         const std::shared_ptr<AppDescriptorTrace> &sessionTrace,
                         const std::vector<int32_t> &threadIds, const ProcessTag procTag);
    void removePowerSession(int64_t sessionId, const ProcessTag procTag);
    // Replace current threads in session with threadIds
    void setThreadsFromPowerSession(int64_t sessionId, const std::vector<int32_t> &threadIds,
                                    const ProcessTag procTag);
    // Pause and resume power hint session
    void pause(int64_t sessionId);
    void resume(int64_t sessionId);

    void updateUniversalBoostMode();
    void dumpToFd(int fd);

    void updateTargetWorkDuration(int64_t sessionId, AdpfVoteType voteId,
                                  std::chrono::nanoseconds durationNs);

    // Set vote for power hint session
    void voteSet(int64_t sessionId, AdpfVoteType voteId, int uclampMin, int uclampMax,
                 std::chrono::steady_clock::time_point startTime,
                 std::chrono::nanoseconds durationNs);
    void voteSet(int64_t sessionId, AdpfVoteType voteId, Cycles capacity,
                 std::chrono::steady_clock::time_point startTime,
                 std::chrono::nanoseconds durationNs);

    void disableBoosts(int64_t sessionId);

    void setPreferPowerEfficiency(int64_t sessionId, bool enabled);

    void updateHboostStatistics(int64_t sessionId, SessionJankyLevel jankyLevel,
                                int32_t numOfFrames);

    void updateFrameBuckets(int64_t sessionId, const FrameBuckets &lastReportedFrames);

    // Singleton
    static PowerSessionManager *getInstance() {
        static PowerSessionManager instance{};
        return &instance;
    }

    std::optional<Frequency> gpuFrequency() const;

    void registerSession(std::shared_ptr<void> session, int64_t sessionId);
    void unregisterSession(int64_t sessionId);
    // Only for testing
    void clear();
    std::shared_ptr<void> getSession(int64_t sessionId);
    bool getGameModeEnableState();

  private:
    std::optional<bool> isAnyAppSessionActive();
    void disableSystemTopAppBoost();
    void enableSystemTopAppBoost();
    const std::string kDisableBoostHintName;

    // Rewrite specific
    mutable std::mutex mSessionTaskMapMutex;
    SessionTaskMap mSessionTaskMap;
    std::shared_ptr<PriorityQueueWorkerPool> mPriorityQueueWorkerPool;

    // Session timeout
    struct EventSessionTimeout {
        std::chrono::steady_clock::time_point timeStamp;
        int64_t sessionId{0};
        int voteId{0};
    };
    void handleEvent(const EventSessionTimeout &e);
    TemplatePriorityQueueWorker<EventSessionTimeout> mEventSessionTimeoutWorker;

    // Calculate uclamp range
    void applyUclampLocked(int64_t sessionId, std::chrono::steady_clock::time_point timePoint)
            REQUIRES(mSessionTaskMapMutex);

    void applyGpuVotesLocked(int64_t sessionId, std::chrono::steady_clock::time_point timePoint)
            REQUIRES(mSessionTaskMapMutex);

    void applyCpuAndGpuVotes(int64_t sessionId, std::chrono::steady_clock::time_point timePoint);
    // Force a session active or in-active, helper for other methods
    void forceSessionActive(int64_t sessionId, bool isActive);

    // Singleton
    PowerSessionManager()
        : kDisableBoostHintName(::android::base::GetProperty(kPowerHalAdpfDisableTopAppBoost,
                                                             "ADPF_DISABLE_TA_BOOST")),
          mPriorityQueueWorkerPool(new PriorityQueueWorkerPool(1, "adpf_handler")),
          mEventSessionTimeoutWorker([&](auto e) { handleEvent(e); }, mPriorityQueueWorkerPool),
          mGpuCapacityNode(createGpuCapacityNode()) {}
    PowerSessionManager(PowerSessionManager const &) = delete;
    PowerSessionManager &operator=(PowerSessionManager const &) = delete;

    std::optional<std::unique_ptr<GpuCapacityNode>> const mGpuCapacityNode;

    std::mutex mSessionMapMutex;
    std::unordered_map<int, std::weak_ptr<void>> mSessionMap GUARDED_BY(mSessionMapMutex);

    std::atomic<bool> mGameModeEnabled{false};
};

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
