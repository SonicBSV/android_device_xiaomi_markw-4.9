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

#define LOG_TAG "powerhal-libperfmgr"
#define ATRACE_TAG (ATRACE_TAG_POWER | ATRACE_TAG_HAL)

#include "PowerSessionManager.h"

#include <android-base/file.h>
#include <log/log.h>
#include <perfmgr/HintManager.h>
#include <private/android_filesystem_config.h>
#include <processgroup/processgroup.h>
#include <sys/syscall.h>
#include <utils/Trace.h>

#include "AppDescriptorTrace.h"
#include "AppHintDesc.h"
#include "tests/mocks/MockHintManager.h"

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

using ::android::perfmgr::HintManager;
constexpr char kGameModeName[] = "GAME";

namespace {
/* there is no glibc or bionic wrapper */
struct sched_attr {
    __u32 size;
    __u32 sched_policy;
    __u64 sched_flags;
    __s32 sched_nice;
    __u32 sched_priority;
    __u64 sched_runtime;
    __u64 sched_deadline;
    __u64 sched_period;
    __u32 sched_util_min;
    __u32 sched_util_max;
};

static int set_uclamp(int tid, UclampRange range) {
    // Ensure min and max are bounded by the range limits and each other
    range.uclampMin = std::min(std::max(kUclampMin, range.uclampMin), kUclampMax);
    range.uclampMax = std::min(std::max(range.uclampMax, range.uclampMin), kUclampMax);
    sched_attr attr = {};
    attr.size = sizeof(attr);

    attr.sched_flags =
            (SCHED_FLAG_KEEP_ALL | SCHED_FLAG_UTIL_CLAMP_MIN | SCHED_FLAG_UTIL_CLAMP_MAX);
    attr.sched_util_min = range.uclampMin;
    attr.sched_util_max = range.uclampMax;

    const int ret = syscall(__NR_sched_setattr, tid, attr, 0);
    if (ret) {
        ALOGW("sched_setattr failed for thread %d, err=%d", tid, errno);
        return errno;
    }
    return 0;
}
}  // namespace

template <class HintManagerT>
void PowerSessionManager<HintManagerT>::updateHintMode(const std::string &mode, bool enabled) {
    ALOGD("%s %s:%b", __func__, mode.c_str(), enabled);
    if (mode.compare(kGameModeName) == 0) {
        mGameModeEnabled = enabled;
    }

    // TODO(jimmyshiu@): Deprecated. Remove once all powerhint.json up-to-date.
    if (enabled && HintManager::GetInstance()->GetAdpfProfileFromDoHint()) {
        HintManager::GetInstance()->SetAdpfProfileFromDoHint(mode);
    }
}

template <class HintManagerT>
bool PowerSessionManager<HintManagerT>::getGameModeEnableState() {
    return mGameModeEnabled;
}

template <class HintManagerT>
void PowerSessionManager<HintManagerT>::addPowerSession(
        const std::string &idString, const std::shared_ptr<AppHintDesc> &sessionDescriptor,
        const std::shared_ptr<AppDescriptorTrace> &sessionTrace,
        const std::vector<int32_t> &threadIds, const ProcessTag procTag) {
    if (!sessionDescriptor) {
        ALOGE("sessionDescriptor is null. PowerSessionManager failed to add power session: %s",
              idString.c_str());
        return;
    }
    const auto timeNow = std::chrono::steady_clock::now();
    SessionValueEntry sve;
    sve.tgid = sessionDescriptor->tgid;
    sve.uid = sessionDescriptor->uid;
    sve.idString = idString;
    sve.isActive = sessionDescriptor->is_active;
    sve.isAppSession = sessionDescriptor->uid >= AID_APP_START;
    sve.lastUpdatedTime = timeNow;
    sve.votes = std::make_shared<Votes>();
    sve.sessionTrace = sessionTrace;
    sve.votes->add(
            static_cast<std::underlying_type_t<AdpfVoteType>>(AdpfVoteType::CPU_VOTE_DEFAULT),
            CpuVote(false, timeNow, sessionDescriptor->targetNs, kUclampMin, kUclampMax));

    bool addedRes = false;
    {
        std::lock_guard<std::mutex> lock(mSessionTaskMapMutex);
        addedRes = mSessionTaskMap.add(sessionDescriptor->sessionId, sve, {});
    }
    if (!addedRes) {
        ALOGE("sessionTaskMap failed to add power session: %" PRId64, sessionDescriptor->sessionId);
    }

    setThreadsFromPowerSession(sessionDescriptor->sessionId, threadIds, procTag);
}

template <class HintManagerT>
void PowerSessionManager<HintManagerT>::removePowerSession(int64_t sessionId,
                                                           const ProcessTag procTag) {
    // To remove a session we also need to undo the effects the session
    // has on currently enabled votes which means setting vote to inactive
    // and then forceing a uclamp update to occur
    forceSessionActive(sessionId, false);

    std::vector<pid_t> addedThreads;
    std::vector<pid_t> removedThreads;

    {
        // Wait till end to remove session because it needs to be around for apply U clamp
        // to work above since applying the uclamp needs a valid session id
        std::lock_guard<std::mutex> lock(mSessionTaskMapMutex);
        mSessionTaskMap.replace(sessionId, {}, &addedThreads, &removedThreads);
        mSessionTaskMap.remove(sessionId);
    }

    if (procTag == ProcessTag::SYSTEM_UI) {
        for (auto tid : removedThreads) {
            if (!SetTaskProfiles(tid, {"SCHED_QOS_SENSITIVE_EXTREME_CLEAR"})) {
                ALOGE("Failed to set SCHED_QOS_SENSITIVE_EXTREME_CLEAR task profile for tid:%d",
                      tid);
            }
        }
    } else {
        for (auto tid : removedThreads) {
            if (!SetTaskProfiles(tid, {"SCHED_QOS_SENSITIVE_STANDARD_CLEAR"})) {
                ALOGE("Failed to set SCHED_QOS_SENSITIVE_STANDARD_CLEAR task profile for tid:%d",
                      tid);
            }
        }
    }

    unregisterSession(sessionId);
}

template <class HintManagerT>
void PowerSessionManager<HintManagerT>::setThreadsFromPowerSession(
        int64_t sessionId, const std::vector<int32_t> &threadIds, const ProcessTag procTag) {
    std::vector<pid_t> addedThreads;
    std::vector<pid_t> removedThreads;
    forceSessionActive(sessionId, false);
    {
        std::lock_guard<std::mutex> lock(mSessionTaskMapMutex);
        mSessionTaskMap.replace(sessionId, threadIds, &addedThreads, &removedThreads);
    }
    if (procTag == ProcessTag::SYSTEM_UI) {
        for (auto tid : addedThreads) {
            if (!SetTaskProfiles(tid, {"SCHED_QOS_SENSITIVE_EXTREME_SET"})) {
                ALOGE("Failed to set SCHED_QOS_SENSITIVE_EXTREME_SET task profile for tid:%d", tid);
            }
        }
    } else {
        for (auto tid : addedThreads) {
            if (!SetTaskProfiles(tid, {"SCHED_QOS_SENSITIVE_STANDARD_SET"})) {
                ALOGE("Failed to set SCHED_QOS_SENSITIVE_STANDARD_SET task profile for tid:%d",
                      tid);
            }
        }
    }
    if (procTag == ProcessTag::SYSTEM_UI) {
        for (auto tid : removedThreads) {
            if (!SetTaskProfiles(tid, {"SCHED_QOS_SENSITIVE_EXTREME_CLEAR"})) {
                ALOGE("Failed to set SCHED_QOS_SENSITIVE_EXTREME_CLEAR task profile for tid:%d",
                      tid);
            }
        }
    } else {
        for (auto tid : removedThreads) {
            if (!SetTaskProfiles(tid, {"SCHED_QOS_SENSITIVE_STANDARD_CLEAR"})) {
                ALOGE("Failed to set SCHED_QOS_SENSITIVE_STANDARD_CLEAR task profile for tid:%d",
                      tid);
            }
        }
    }
    forceSessionActive(sessionId, true);
}

template <class HintManagerT>
std::optional<bool> PowerSessionManager<HintManagerT>::isAnyAppSessionActive() {
    bool isAnyAppSessionActive = false;
    {
        std::lock_guard<std::mutex> lock(mSessionTaskMapMutex);
        isAnyAppSessionActive =
                mSessionTaskMap.isAnyAppSessionActive(std::chrono::steady_clock::now());
    }
    return isAnyAppSessionActive;
}

template <class HintManagerT>
void PowerSessionManager<HintManagerT>::updateUniversalBoostMode() {
    const auto active = isAnyAppSessionActive();
    if (!active.has_value()) {
        return;
    }
    if (active.value()) {
        disableSystemTopAppBoost();
    } else {
        enableSystemTopAppBoost();
    }
}

template <class HintManagerT>
void PowerSessionManager<HintManagerT>::dumpToFd(int fd) {
    std::ostringstream dump_buf;
    dump_buf << "========== Begin PowerSessionManager ADPF list ==========\n";
    std::lock_guard<std::mutex> lock(mSessionTaskMapMutex);
    mSessionTaskMap.forEachSessionValTasks(
            [&](auto /* sessionId */, const auto &sessionVal, const auto &tasks) {
                sessionVal.dump(dump_buf);
                dump_buf << " Tid:Ref[";

                size_t tasksLen = tasks.size();
                for (auto taskId : tasks) {
                    dump_buf << taskId << ":";
                    const auto &sessionIds = mSessionTaskMap.getSessionIds(taskId);
                    if (!sessionIds.empty()) {
                        dump_buf << sessionIds.size();
                    }
                    if (tasksLen > 0) {
                        dump_buf << ", ";
                        --tasksLen;
                    }
                }
                dump_buf << "]\n";
            });
    dump_buf << "========== End PowerSessionManager ADPF list ==========\n";
    if (!::android::base::WriteStringToFd(dump_buf.str(), fd)) {
        ALOGE("Failed to dump one of session list to fd:%d", fd);
    }
}

template <class HintManagerT>
void PowerSessionManager<HintManagerT>::pause(int64_t sessionId) {
    {
        std::lock_guard<std::mutex> lock(mSessionTaskMapMutex);
        auto sessValPtr = mSessionTaskMap.findSession(sessionId);
        if (nullptr == sessValPtr) {
            ALOGW("Pause failed, session is null %" PRId64, sessionId);
            return;
        }

        if (!sessValPtr->isActive) {
            ALOGW("Sess(%" PRId64 "), cannot pause, already inActive", sessionId);
            return;
        }
        sessValPtr->isActive = false;
    }
    applyCpuAndGpuVotes(sessionId, std::chrono::steady_clock::now());
    updateUniversalBoostMode();
}

template <class HintManagerT>
void PowerSessionManager<HintManagerT>::resume(int64_t sessionId) {
    {
        std::lock_guard<std::mutex> lock(mSessionTaskMapMutex);
        auto sessValPtr = mSessionTaskMap.findSession(sessionId);
        if (nullptr == sessValPtr) {
            ALOGW("Resume failed, session is null %" PRId64, sessionId);
            return;
        }

        if (sessValPtr->isActive) {
            ALOGW("Sess(%" PRId64 "), cannot resume, already active", sessionId);
            return;
        }
        sessValPtr->isActive = true;
    }
    applyCpuAndGpuVotes(sessionId, std::chrono::steady_clock::now());
    updateUniversalBoostMode();
}

template <class HintManagerT>
void PowerSessionManager<HintManagerT>::updateTargetWorkDuration(
        int64_t sessionId, AdpfVoteType voteId, std::chrono::nanoseconds durationNs) {
    int voteIdInt = static_cast<std::underlying_type_t<AdpfVoteType>>(voteId);
    std::lock_guard<std::mutex> lock(mSessionTaskMapMutex);
    auto sessValPtr = mSessionTaskMap.findSession(sessionId);
    if (nullptr == sessValPtr) {
        ALOGE("Failed to updateTargetWorkDuration, session val is null id: %" PRId64, sessionId);
        return;
    }

    sessValPtr->votes->updateDuration(voteIdInt, durationNs);
    // Note, for now we are not recalculating and applying uclamp because
    // that maintains behavior from before.  In the future we may want to
    // revisit that decision.
}

template <typename T>
auto shouldScheduleTimeout(Votes const &votes, int vote_id, std::chrono::time_point<T> deadline)
        -> bool {
    return !votes.voteIsActive(vote_id) || deadline < votes.voteTimeout(vote_id);
}

template <class HintManagerT>
void PowerSessionManager<HintManagerT>::voteSet(int64_t sessionId, AdpfVoteType voteId,
                                                int uclampMin, int uclampMax,
                                                std::chrono::steady_clock::time_point startTime,
                                                std::chrono::nanoseconds durationNs) {
    const int voteIdInt = static_cast<std::underlying_type_t<AdpfVoteType>>(voteId);
    const auto timeoutDeadline = startTime + durationNs;
    bool scheduleTimeout = false;

    {
        std::lock_guard lock(mSessionTaskMapMutex);
        auto session = mSessionTaskMap.findSession(sessionId);
        if (!session) {
            // Because of the async nature of some events an event for a session
            // that has been removed is a possibility
            return;
        }
        scheduleTimeout = shouldScheduleTimeout(*session->votes, voteIdInt, timeoutDeadline),
        mSessionTaskMap.addVote(sessionId, voteIdInt, uclampMin, uclampMax, startTime, durationNs);
        if (ATRACE_ENABLED()) {
            ATRACE_INT(session->sessionTrace->trace_votes[voteIdInt].c_str(), uclampMin);
        }
        session->lastUpdatedTime = startTime;
        applyUclampLocked(sessionId, startTime);
    }

    if (scheduleTimeout) {
        mEventSessionTimeoutWorker.schedule(
                {.timeStamp = startTime, .sessionId = sessionId, .voteId = voteIdInt},
                timeoutDeadline);
    }
}

template <class HintManagerT>
void PowerSessionManager<HintManagerT>::voteSet(int64_t sessionId, AdpfVoteType voteId,
                                                Cycles capacity,
                                                std::chrono::steady_clock::time_point startTime,
                                                std::chrono::nanoseconds durationNs) {
    const int voteIdInt = static_cast<std::underlying_type_t<AdpfVoteType>>(voteId);
    const auto timeoutDeadline = startTime + durationNs;
    bool scheduleTimeout = false;

    {
        std::lock_guard lock(mSessionTaskMapMutex);
        auto session = mSessionTaskMap.findSession(sessionId);
        if (!session) {
            return;
        }
        scheduleTimeout = shouldScheduleTimeout(*session->votes, voteIdInt, timeoutDeadline),
        mSessionTaskMap.addGpuVote(sessionId, voteIdInt, capacity, startTime, durationNs);
        if (ATRACE_ENABLED()) {
            ATRACE_INT(session->sessionTrace->trace_votes[voteIdInt].c_str(),
                       static_cast<int>(capacity));
        }
        session->lastUpdatedTime = startTime;
        applyGpuVotesLocked(sessionId, startTime);
    }

    if (scheduleTimeout) {
        mEventSessionTimeoutWorker.schedule(
                {.timeStamp = startTime, .sessionId = sessionId, .voteId = voteIdInt},
                timeoutDeadline);
    }
}

template <class HintManagerT>
void PowerSessionManager<HintManagerT>::disableBoosts(int64_t sessionId) {
    {
        std::lock_guard<std::mutex> lock(mSessionTaskMapMutex);
        auto sessValPtr = mSessionTaskMap.findSession(sessionId);
        if (nullptr == sessValPtr) {
            // Because of the async nature of some events an event for a session
            // that has been removed is a possibility
            return;
        }

        // sessValPtr->disableBoosts();
        for (auto vid : {AdpfVoteType::CPU_LOAD_UP, AdpfVoteType::CPU_LOAD_RESET,
                         AdpfVoteType::CPU_LOAD_RESUME, AdpfVoteType::VOTE_POWER_EFFICIENCY,
                         AdpfVoteType::GPU_LOAD_UP, AdpfVoteType::GPU_LOAD_RESET}) {
            auto vint = static_cast<std::underlying_type_t<AdpfVoteType>>(vid);
            sessValPtr->votes->setUseVote(vint, false);
            if (ATRACE_ENABLED()) {
                ATRACE_INT(sessValPtr->sessionTrace->trace_votes[vint].c_str(), 0);
            }
        }
    }
}

template <class HintManagerT>
void PowerSessionManager<HintManagerT>::enableSystemTopAppBoost() {
    if (HintManagerT::GetInstance()->IsHintSupported(kDisableBoostHintName)) {
        ALOGV("PowerSessionManager::enableSystemTopAppBoost!!");
        HintManagerT::GetInstance()->EndHint(kDisableBoostHintName);
    }
}

template <class HintManagerT>
void PowerSessionManager<HintManagerT>::disableSystemTopAppBoost() {
    if (HintManagerT::GetInstance()->IsHintSupported(kDisableBoostHintName)) {
        ALOGV("PowerSessionManager::disableSystemTopAppBoost!!");
        HintManagerT::GetInstance()->DoHint(kDisableBoostHintName);
    }
}

template <class HintManagerT>
void PowerSessionManager<HintManagerT>::handleEvent(const EventSessionTimeout &eventTimeout) {
    bool recalcUclamp = false;
    const auto tNow = std::chrono::steady_clock::now();
    {
        std::lock_guard<std::mutex> lock(mSessionTaskMapMutex);
        auto sessValPtr = mSessionTaskMap.findSession(eventTimeout.sessionId);
        if (nullptr == sessValPtr) {
            // It is ok for session timeouts to fire after a session has been
            // removed
            return;
        }

        // To minimize the number of events pushed into the queue, we are using
        // the following logic to make use of a single timeout event which will
        // requeue itself if the timeout has been changed since it was added to
        // the work queue.  Requeue Logic:
        // if vote active and vote timeout <= sched time
        //    then deactivate vote and recalc uclamp (near end of function)
        // if vote active and vote timeout > sched time
        //    then requeue timeout event for new deadline (which is vote timeout)
        const bool voteIsActive = sessValPtr->votes->voteIsActive(eventTimeout.voteId);
        const auto voteTimeout = sessValPtr->votes->voteTimeout(eventTimeout.voteId);

        if (voteIsActive) {
            if (voteTimeout <= tNow) {
                sessValPtr->votes->setUseVote(eventTimeout.voteId, false);
                recalcUclamp = true;
                if (ATRACE_ENABLED()) {
                    ATRACE_INT(sessValPtr->sessionTrace->trace_votes[eventTimeout.voteId].c_str(),
                               0);
                }
            } else {
                // Can unlock sooner than we do
                auto eventTimeout2 = eventTimeout;
                mEventSessionTimeoutWorker.schedule(eventTimeout2, voteTimeout);
            }
        }
    }

    if (!recalcUclamp) {
        return;
    }

    // It is important to use the correct time here, time now is more reasonable
    // than trying to use the event's timestamp which will be slightly off given
    // the background priority queue introduces latency
    applyCpuAndGpuVotes(eventTimeout.sessionId, tNow);
    updateUniversalBoostMode();
}

template <class HintManagerT>
void PowerSessionManager<HintManagerT>::applyUclampLocked(
        int64_t sessionId, std::chrono::steady_clock::time_point timePoint) {
    auto config = HintManagerT::GetInstance()->GetAdpfProfile();
    {
        // TODO(kevindubois) un-indent this in followup patch to reduce churn.
        auto sessValPtr = mSessionTaskMap.findSession(sessionId);
        if (nullptr == sessValPtr) {
            return;
        }

        if (!config->mUclampMinOn) {
            ALOGV("PowerSessionManager::set_uclamp: skip");
        } else {
            auto &threadList = mSessionTaskMap.getTaskIds(sessionId);
            auto tidIter = threadList.begin();
            while (tidIter != threadList.end()) {
                UclampRange uclampRange;
                mSessionTaskMap.getTaskVoteRange(*tidIter, timePoint, uclampRange,
                                                 config->mUclampMaxEfficientBase,
                                                 config->mUclampMaxEfficientOffset);
                int stat = set_uclamp(*tidIter, uclampRange);
                if (stat == ESRCH) {
                    ALOGV("Removing dead thread %d from hint session %s.", *tidIter,
                          sessValPtr->idString.c_str());
                    if (mSessionTaskMap.removeDeadTaskSessionMap(sessionId, *tidIter)) {
                        ALOGV("Removed dead thread-session map.");
                    }
                    tidIter = threadList.erase(tidIter);
                } else {
                    tidIter++;
                }
            }
        }

        sessValPtr->lastUpdatedTime = timePoint;
    }
}

template <class HintManagerT>
void PowerSessionManager<HintManagerT>::applyGpuVotesLocked(
        int64_t sessionId, std::chrono::steady_clock::time_point timePoint) {
    auto const sessValPtr = mSessionTaskMap.findSession(sessionId);
    if (!sessValPtr) {
        return;
    }

    auto const gpuVotingOn = HintManagerT::GetInstance()->GetAdpfProfile()->mGpuBoostOn;
    if (mGpuCapacityNode && gpuVotingOn) {
        auto const capacity = mSessionTaskMap.getSessionsGpuCapacity(timePoint);
        (*mGpuCapacityNode)->set_gpu_capacity(capacity);
    }

    sessValPtr->lastUpdatedTime = timePoint;
}

template <class HintManagerT>
void PowerSessionManager<HintManagerT>::applyCpuAndGpuVotes(
        int64_t sessionId, std::chrono::steady_clock::time_point timePoint) {
    std::lock_guard lock(mSessionTaskMapMutex);
    applyUclampLocked(sessionId, timePoint);
    applyGpuVotesLocked(sessionId, timePoint);
}

template <class HintManagerT>
std::optional<Frequency> PowerSessionManager<HintManagerT>::gpuFrequency() const {
    if (mGpuCapacityNode) {
        return (*mGpuCapacityNode)->gpu_frequency();
    }
    return {};
}

template <class HintManagerT>
void PowerSessionManager<HintManagerT>::forceSessionActive(int64_t sessionId, bool isActive) {
    {
        std::lock_guard<std::mutex> lock(mSessionTaskMapMutex);
        auto sessValPtr = mSessionTaskMap.findSession(sessionId);
        if (nullptr == sessValPtr) {
            return;
        }
        sessValPtr->isActive = isActive;
    }

    // As currently written, call needs to occur synchronously so as to ensure
    // that the SessionId remains valid and mapped to the proper threads/tasks
    // which enables apply u clamp to work correctly
    applyCpuAndGpuVotes(sessionId, std::chrono::steady_clock::now());
    updateUniversalBoostMode();
}

template <class HintManagerT>
void PowerSessionManager<HintManagerT>::setPreferPowerEfficiency(int64_t sessionId, bool enabled) {
    std::lock_guard<std::mutex> lock(mSessionTaskMapMutex);
    auto sessValPtr = mSessionTaskMap.findSession(sessionId);
    if (nullptr == sessValPtr) {
        return;
    }
    if (enabled != sessValPtr->isPowerEfficient) {
        sessValPtr->isPowerEfficient = enabled;
        applyUclampLocked(sessionId, std::chrono::steady_clock::now());
    }
}

template <class HintManagerT>
void PowerSessionManager<HintManagerT>::registerSession(std::shared_ptr<void> session,
                                                        int64_t sessionId) {
    std::lock_guard<std::mutex> lock(mSessionMapMutex);
    mSessionMap[sessionId] = session;
}

template <class HintManagerT>
void PowerSessionManager<HintManagerT>::unregisterSession(int64_t sessionId) {
    std::lock_guard<std::mutex> lock(mSessionMapMutex);
    mSessionMap.erase(sessionId);
}

template <class HintManagerT>
std::shared_ptr<void> PowerSessionManager<HintManagerT>::getSession(int64_t sessionId) {
    std::scoped_lock lock(mSessionMapMutex);
    auto ptr = mSessionMap.find(sessionId);
    if (ptr == mSessionMap.end()) {
        return nullptr;
    }
    std::shared_ptr<void> out = ptr->second.lock();
    if (!out) {
        mSessionMap.erase(sessionId);
        return nullptr;
    }
    return out;
}

template <class HintManagerT>
void PowerSessionManager<HintManagerT>::clear() {
    std::scoped_lock lock(mSessionMapMutex);
    mSessionMap.clear();
}

template <class HintManagerT>
void PowerSessionManager<HintManagerT>::updateFrameBuckets(int64_t sessionId,
                                                           const FrameBuckets &lastReportedFrames) {
    std::lock_guard<std::mutex> lock(mSessionTaskMapMutex);
    auto sessValPtr = mSessionTaskMap.findSession(sessionId);
    if (nullptr == sessValPtr) {
        return;
    }

    sessValPtr->sessFrameBuckets.addUpNewFrames(lastReportedFrames);
}

template <class HintManagerT>
void PowerSessionManager<HintManagerT>::updateHboostStatistics(int64_t sessionId,
                                                               SessionJankyLevel jankyLevel,
                                                               int32_t numOfFrames) {
    std::lock_guard<std::mutex> lock(mSessionTaskMapMutex);
    auto sessValPtr = mSessionTaskMap.findSession(sessionId);
    if (nullptr == sessValPtr) {
        return;
    }
    switch (jankyLevel) {
        case SessionJankyLevel::LIGHT:
            sessValPtr->hBoostModeDist.lightModeFrames += numOfFrames;
            break;
        case SessionJankyLevel::MODERATE:
            sessValPtr->hBoostModeDist.moderateModeFrames += numOfFrames;
            break;
        case SessionJankyLevel::SEVERE:
            sessValPtr->hBoostModeDist.severeModeFrames += numOfFrames;
            break;
        default:
            ALOGW("Unknown janky level during updateHboostStatistics");
    }
}

template class PowerSessionManager<>;
template class PowerSessionManager<testing::NiceMock<mock::pixel::MockHintManager>>;

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
