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

#include "PowerHintSession.h"

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/parsedouble.h>
#include <android-base/properties.h>
#include <android-base/stringprintf.h>
#include <private/android_filesystem_config.h>
#include <sys/syscall.h>
#include <time.h>
#include <utils/Trace.h>

#include <atomic>

#include "GpuCalculationHelpers.h"
#include "tests/mocks/MockHintManager.h"
#include "tests/mocks/MockPowerSessionManager.h"

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

using ::android::base::StringPrintf;
using ::android::perfmgr::AdpfConfig;
using ::android::perfmgr::HintManager;
using std::chrono::duration_cast;
using std::chrono::nanoseconds;

using std::operator""ms;

namespace {

static std::atomic<int64_t> sSessionIDCounter{0};

static inline int64_t ns_to_100us(int64_t ns) {
    return ns / 100000;
}

static const char systemSessionCheckPath[] = "/proc/vendor_sched/is_tgid_system_ui";
static const bool systemSessionCheckNodeExist = access(systemSessionCheckPath, W_OK) == 0;
static constexpr int32_t kTargetDurationChangeThreshold = 30;  // Percentage change threshold

}  // namespace

template <class HintManagerT, class PowerSessionManagerT>
int64_t PowerHintSession<HintManagerT, PowerSessionManagerT>::convertWorkDurationToBoostByPid(
        const std::vector<WorkDuration> &actualDurations) {
    std::shared_ptr<AdpfConfig> adpfConfig = getAdpfProfile();
    const nanoseconds &targetDuration = mDescriptor->targetNs;
    int64_t &integral_error = mDescriptor->integral_error;
    int64_t &previous_error = mDescriptor->previous_error;
    uint64_t samplingWindowP = adpfConfig->mSamplingWindowP;
    uint64_t samplingWindowI = adpfConfig->mSamplingWindowI;
    uint64_t samplingWindowD = adpfConfig->mSamplingWindowD;
    int64_t targetDurationNanos = (int64_t)targetDuration.count();
    int64_t length = actualDurations.size();
    int64_t p_start =
            samplingWindowP == 0 || samplingWindowP > length ? 0 : length - samplingWindowP;
    int64_t i_start =
            samplingWindowI == 0 || samplingWindowI > length ? 0 : length - samplingWindowI;
    int64_t d_start =
            samplingWindowD == 0 || samplingWindowD > length ? 0 : length - samplingWindowD;
    int64_t dt = ns_to_100us(targetDurationNanos);
    int64_t err_sum = 0;
    int64_t derivative_sum = 0;
    for (int64_t i = std::min({p_start, i_start, d_start}); i < length; i++) {
        int64_t actualDurationNanos = actualDurations[i].durationNanos;
        if (std::abs(actualDurationNanos) > targetDurationNanos * 20) {
            ALOGW("The actual duration is way far from the target (%" PRId64 " >> %" PRId64 ")",
                  actualDurationNanos, targetDurationNanos);
        }
        // PID control algorithm
        int64_t error = ns_to_100us(actualDurationNanos - targetDurationNanos);
        if (i >= d_start) {
            derivative_sum += error - previous_error;
        }
        if (i >= p_start) {
            err_sum += error;
        }
        if (i >= i_start) {
            integral_error += error * dt;
            integral_error = std::min(adpfConfig->getPidIHighDivI(), integral_error);
            integral_error = std::max(adpfConfig->getPidILowDivI(), integral_error);
        }
        previous_error = error;
    }

    auto pid_pu_active = adpfConfig->mPidPu;
    if (adpfConfig->mHeuristicBoostOn.has_value() && adpfConfig->mHeuristicBoostOn.value()) {
        auto hboostPidPu = std::min(adpfConfig->mHBoostSevereJankPidPu.value(), adpfConfig->mPidPu);
        if (mJankyLevel == SessionJankyLevel::MODERATE) {
            double JankyFactor =
                    mJankyFrameNum < adpfConfig->mHBoostModerateJankThreshold.value()
                            ? 0.0
                            : (mJankyFrameNum - adpfConfig->mHBoostModerateJankThreshold.value()) *
                                      1.0 /
                                      (adpfConfig->mHBoostSevereJankThreshold.value() -
                                       adpfConfig->mHBoostModerateJankThreshold.value());
            pid_pu_active = adpfConfig->mPidPu + JankyFactor * (hboostPidPu - adpfConfig->mPidPu);
        } else if (mJankyLevel == SessionJankyLevel::SEVERE) {
            pid_pu_active = hboostPidPu;
        }
        ATRACE_INT(mAppDescriptorTrace->trace_hboost_pid_pu.c_str(), pid_pu_active * 100);
    }
    int64_t pOut = static_cast<int64_t>((err_sum > 0 ? adpfConfig->mPidPo : pid_pu_active) *
                                        err_sum / (length - p_start));
    int64_t iOut = static_cast<int64_t>(adpfConfig->mPidI * integral_error);
    int64_t dOut =
            static_cast<int64_t>((derivative_sum > 0 ? adpfConfig->mPidDo : adpfConfig->mPidDu) *
                                 derivative_sum / dt / (length - d_start));

    int64_t output = pOut + iOut + dOut;
    ATRACE_INT(mAppDescriptorTrace->trace_pid_err.c_str(), err_sum / (length - p_start));
    ATRACE_INT(mAppDescriptorTrace->trace_pid_integral.c_str(), integral_error);
    ATRACE_INT(mAppDescriptorTrace->trace_pid_derivative.c_str(),
               derivative_sum / dt / (length - d_start));
    ATRACE_INT(mAppDescriptorTrace->trace_pid_pOut.c_str(), pOut);
    ATRACE_INT(mAppDescriptorTrace->trace_pid_iOut.c_str(), iOut);
    ATRACE_INT(mAppDescriptorTrace->trace_pid_dOut.c_str(), dOut);
    ATRACE_INT(mAppDescriptorTrace->trace_pid_output.c_str(), output);
    return output;
}

template <class HintManagerT, class PowerSessionManagerT>
ProcessTag PowerHintSession<HintManagerT, PowerSessionManagerT>::getProcessTag(int32_t tgid) {
    if (!systemSessionCheckNodeExist) {
        ALOGD("Vendor system session checking node doesn't exist");
        return ProcessTag::DEFAULT;
    }

    int flags = O_WRONLY | O_TRUNC | O_CLOEXEC;
    ::android::base::unique_fd fd(TEMP_FAILURE_RETRY(open(systemSessionCheckPath, flags)));
    if (fd == -1) {
        ALOGW("Can't open system session checking node %s", systemSessionCheckPath);
        return ProcessTag::DEFAULT;
    }
    // The file-write return status is true if the task belongs to systemUI or Launcher. Other task
    // or invalid tgid will return a false value.
    auto stat = ::android::base::WriteStringToFd(std::to_string(tgid), fd);
    ALOGD("System session checking result: %d - %d", tgid, stat);
    if (stat) {
        return ProcessTag::SYSTEM_UI;
    } else {
        return ProcessTag::DEFAULT;
    }
}

template <class HintManagerT, class PowerSessionManagerT>
PowerHintSession<HintManagerT, PowerSessionManagerT>::PowerHintSession(
        int32_t tgid, int32_t uid, const std::vector<int32_t> &threadIds, int64_t durationNs,
        SessionTag tag)
    : mPSManager(PowerSessionManagerT::getInstance()),
      mSessionId(++sSessionIDCounter),
      mSessTag(tag),
      mProcTag(getProcessTag(tgid)),
      mIdString(StringPrintf("%" PRId32 "-%" PRId32 "-%" PRId64 "-%s-%" PRId32, tgid, uid,
                             mSessionId, toString(tag).c_str(), static_cast<int32_t>(mProcTag))),
      mDescriptor(std::make_shared<AppHintDesc>(mSessionId, tgid, uid, threadIds, tag,
                                                std::chrono::nanoseconds(durationNs))),
      mAppDescriptorTrace(std::make_shared<AppDescriptorTrace>(mIdString)),
      mAdpfProfile(mProcTag != ProcessTag::DEFAULT
                           ? HintManager::GetInstance()->GetAdpfProfile(toString(mProcTag))
                           : HintManager::GetInstance()->GetAdpfProfile(toString(mSessTag))),
      mOnAdpfUpdate(
              [this](const std::shared_ptr<AdpfConfig> config) { this->setAdpfProfile(config); }),
      mSessionRecords(getAdpfProfile()->mHeuristicBoostOn.has_value() &&
                                      getAdpfProfile()->mHeuristicBoostOn.value()
                              ? std::make_unique<SessionRecords>(
                                        getAdpfProfile()->mMaxRecordsNum.value(),
                                        getAdpfProfile()->mJankCheckTimeFactor.value())
                              : nullptr) {
    ATRACE_CALL();
    ATRACE_INT(mAppDescriptorTrace->trace_target.c_str(), mDescriptor->targetNs.count());
    ATRACE_INT(mAppDescriptorTrace->trace_active.c_str(), mDescriptor->is_active.load());

    if (mProcTag != ProcessTag::DEFAULT) {
        HintManager::GetInstance()->RegisterAdpfUpdateEvent(toString(mProcTag), &mOnAdpfUpdate);
    } else {
        HintManager::GetInstance()->RegisterAdpfUpdateEvent(toString(mSessTag), &mOnAdpfUpdate);
    }

    mLastUpdatedTime = std::chrono::steady_clock::now();
    mPSManager->addPowerSession(mIdString, mDescriptor, mAppDescriptorTrace, threadIds, mProcTag);
    // init boost
    auto adpfConfig = getAdpfProfile();
    mPSManager->voteSet(
            mSessionId, AdpfVoteType::CPU_LOAD_RESET, adpfConfig->mUclampMinLoadReset, kUclampMax,
            std::chrono::steady_clock::now(),
            duration_cast<nanoseconds>(mDescriptor->targetNs * adpfConfig->mStaleTimeFactor / 2.0));

    mPSManager->voteSet(mSessionId, AdpfVoteType::CPU_VOTE_DEFAULT, adpfConfig->mUclampMinInit,
                        kUclampMax, std::chrono::steady_clock::now(), mDescriptor->targetNs);
    ALOGV("PowerHintSession created: %s", mDescriptor->toString().c_str());
}

template <class HintManagerT, class PowerSessionManagerT>
PowerHintSession<HintManagerT, PowerSessionManagerT>::~PowerHintSession() {
    ATRACE_CALL();
    close();
    ALOGV("PowerHintSession deleted: %s", mDescriptor->toString().c_str());
    ATRACE_INT(mAppDescriptorTrace->trace_target.c_str(), 0);
    ATRACE_INT(mAppDescriptorTrace->trace_actl_last.c_str(), 0);
    ATRACE_INT(mAppDescriptorTrace->trace_active.c_str(), 0);
}

template <class HintManagerT, class PowerSessionManagerT>
bool PowerHintSession<HintManagerT, PowerSessionManagerT>::isAppSession() {
    // Check if uid is in range reserved for applications
    return mDescriptor->uid >= AID_APP_START;
}

template <class HintManagerT, class PowerSessionManagerT>
void PowerHintSession<HintManagerT, PowerSessionManagerT>::updatePidControlVariable(
        int pidControlVariable, bool updateVote) {
    mDescriptor->pidControlVariable = pidControlVariable;
    if (updateVote) {
        auto adpfConfig = getAdpfProfile();
        mPSManager->voteSet(mSessionId, AdpfVoteType::CPU_VOTE_DEFAULT, pidControlVariable,
                            kUclampMax, std::chrono::steady_clock::now(),
                            std::max(duration_cast<nanoseconds>(mDescriptor->targetNs *
                                                                adpfConfig->mStaleTimeFactor),
                                     nanoseconds(adpfConfig->mReportingRateLimitNs) * 2));
    }
    ATRACE_INT(mAppDescriptorTrace->trace_min.c_str(), pidControlVariable);
}

template <class HintManagerT, class PowerSessionManagerT>
void PowerHintSession<HintManagerT, PowerSessionManagerT>::tryToSendPowerHint(std::string hint) {
    if (!mSupportedHints[hint].has_value()) {
        mSupportedHints[hint] = HintManagerT::GetInstance()->IsHintSupported(hint);
    }
    if (mSupportedHints[hint].value()) {
        HintManagerT::GetInstance()->DoHint(hint);
    }
}

template <class HintManagerT, class PowerSessionManagerT>
void PowerHintSession<HintManagerT, PowerSessionManagerT>::dumpToStream(std::ostream &stream) {
    std::scoped_lock lock{mPowerHintSessionLock};
    stream << "ID.Min.Act.Timeout(" << mIdString;
    stream << ", " << mDescriptor->pidControlVariable;
    stream << ", " << mDescriptor->is_active;
    stream << ", " << isTimeout() << ")";
}

template <class HintManagerT, class PowerSessionManagerT>
ndk::ScopedAStatus PowerHintSession<HintManagerT, PowerSessionManagerT>::pause() {
    std::scoped_lock lock{mPowerHintSessionLock};
    if (mSessionClosed) {
        ALOGE("Error: session is dead");
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    if (!mDescriptor->is_active.load())
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    // Reset to default uclamp value.
    mPSManager->setThreadsFromPowerSession(mSessionId, {}, mProcTag);
    mDescriptor->is_active.store(false);
    mPSManager->pause(mSessionId);
    ATRACE_INT(mAppDescriptorTrace->trace_active.c_str(), false);
    ATRACE_INT(mAppDescriptorTrace->trace_min.c_str(), 0);
    return ndk::ScopedAStatus::ok();
}

template <class HintManagerT, class PowerSessionManagerT>
ndk::ScopedAStatus PowerHintSession<HintManagerT, PowerSessionManagerT>::resume() {
    std::scoped_lock lock{mPowerHintSessionLock};
    if (mSessionClosed) {
        ALOGE("Error: session is dead");
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    if (mDescriptor->is_active.load()) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    mPSManager->setThreadsFromPowerSession(mSessionId, mDescriptor->thread_ids, mProcTag);
    mDescriptor->is_active.store(true);
    // resume boost
    mPSManager->resume(mSessionId);
    ATRACE_INT(mAppDescriptorTrace->trace_active.c_str(), true);
    ATRACE_INT(mAppDescriptorTrace->trace_min.c_str(), mDescriptor->pidControlVariable);
    return ndk::ScopedAStatus::ok();
}

template <class HintManagerT, class PowerSessionManagerT>
ndk::ScopedAStatus PowerHintSession<HintManagerT, PowerSessionManagerT>::close() {
    std::scoped_lock lock{mPowerHintSessionLock};
    if (mSessionClosed) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    mSessionClosed = true;
    // Remove the session from PowerSessionManager first to avoid racing.
    mPSManager->removePowerSession(mSessionId, mProcTag);
    mDescriptor->is_active.store(false);

    if (mProcTag != ProcessTag::DEFAULT) {
        HintManager::GetInstance()->UnregisterAdpfUpdateEvent(toString(mProcTag), &mOnAdpfUpdate);
    } else {
        HintManager::GetInstance()->UnregisterAdpfUpdateEvent(toString(mSessTag), &mOnAdpfUpdate);
    }
    ATRACE_INT(mAppDescriptorTrace->trace_min.c_str(), 0);
    return ndk::ScopedAStatus::ok();
}

template <class HintManagerT, class PowerSessionManagerT>
ndk::ScopedAStatus PowerHintSession<HintManagerT, PowerSessionManagerT>::updateTargetWorkDuration(
        int64_t targetDurationNanos) {
    std::scoped_lock lock{mPowerHintSessionLock};
    if (mSessionClosed) {
        ALOGE("Error: session is dead");
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    if (targetDurationNanos <= 0) {
        ALOGE("Error: targetDurationNanos(%" PRId64 ") should bigger than 0", targetDurationNanos);
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    targetDurationNanos = targetDurationNanos * getAdpfProfile()->mTargetTimeFactor;

    // Reset session records and heuristic boost states when the percentage change of target
    // duration is over the threshold.
    if (targetDurationNanos != mDescriptor->targetNs.count() &&
        getAdpfProfile()->mHeuristicBoostOn.has_value() &&
        getAdpfProfile()->mHeuristicBoostOn.value()) {
        auto lastTargetNs = mDescriptor->targetNs.count();
        if (abs(targetDurationNanos - lastTargetNs) >
            lastTargetNs / 100 * kTargetDurationChangeThreshold) {
            resetSessionHeuristicStates();
        }
    }

    mDescriptor->targetNs = std::chrono::nanoseconds(targetDurationNanos);
    mPSManager->updateTargetWorkDuration(mSessionId, AdpfVoteType::CPU_VOTE_DEFAULT,
                                         mDescriptor->targetNs);
    ATRACE_INT(mAppDescriptorTrace->trace_target.c_str(), targetDurationNanos);

    return ndk::ScopedAStatus::ok();
}

template <class HintManagerT, class PowerSessionManagerT>
void PowerHintSession<HintManagerT, PowerSessionManagerT>::resetSessionHeuristicStates() {
    mSessionRecords->resetRecords();
    mJankyLevel = SessionJankyLevel::LIGHT;
    mJankyFrameNum = 0;
    ATRACE_INT(mAppDescriptorTrace->trace_hboost_janky_level.c_str(),
               static_cast<int32_t>(mJankyLevel));
    ATRACE_INT(mAppDescriptorTrace->trace_missed_cycles.c_str(), mJankyFrameNum);
    ATRACE_INT(mAppDescriptorTrace->trace_avg_duration.c_str(), 0);
    ATRACE_INT(mAppDescriptorTrace->trace_max_duration.c_str(), 0);
    ATRACE_INT(mAppDescriptorTrace->trace_low_frame_rate.c_str(), false);
}

template <class HintManagerT, class PowerSessionManagerT>
SessionJankyLevel PowerHintSession<HintManagerT, PowerSessionManagerT>::updateSessionJankState(
        SessionJankyLevel oldState, int32_t numOfJankFrames, double durationVariance,
        bool isLowFPS) {
    SessionJankyLevel newState = SessionJankyLevel::LIGHT;
    if (isLowFPS) {
        newState = SessionJankyLevel::LIGHT;
        return newState;
    }

    auto adpfConfig = getAdpfProfile();
    if (numOfJankFrames < adpfConfig->mHBoostModerateJankThreshold.value()) {
        if (oldState == SessionJankyLevel::LIGHT ||
            durationVariance < adpfConfig->mHBoostOffMaxAvgDurRatio.value()) {
            newState = SessionJankyLevel::LIGHT;
        } else {
            newState = SessionJankyLevel::MODERATE;
        }
    } else if (numOfJankFrames < adpfConfig->mHBoostSevereJankThreshold.value()) {
        newState = SessionJankyLevel::MODERATE;
    } else {
        newState = SessionJankyLevel::SEVERE;
    }

    return newState;
}

template <class HintManagerT, class PowerSessionManagerT>
void PowerHintSession<HintManagerT, PowerSessionManagerT>::updateHeuristicBoost() {
    auto maxDurationUs = mSessionRecords->getMaxDuration();  // micro seconds
    auto avgDurationUs = mSessionRecords->getAvgDuration();  // micro seconds
    auto numOfJankFrames = mSessionRecords->getNumOfMissedCycles();

    if (!maxDurationUs.has_value() || !avgDurationUs.has_value() || avgDurationUs.value() <= 0) {
        // No history data stored or invalid average duration.
        return;
    }

    auto maxToAvgRatio = maxDurationUs.value() * 1.0 / avgDurationUs.value();
    auto isLowFPS =
            mSessionRecords->isLowFrameRate(getAdpfProfile()->mLowFrameRateThreshold.value());

    mJankyLevel = updateSessionJankState(mJankyLevel, numOfJankFrames, maxToAvgRatio, isLowFPS);
    mJankyFrameNum = numOfJankFrames;

    ATRACE_INT(mAppDescriptorTrace->trace_hboost_janky_level.c_str(),
               static_cast<int32_t>(mJankyLevel));
    ATRACE_INT(mAppDescriptorTrace->trace_missed_cycles.c_str(), mJankyFrameNum);
    ATRACE_INT(mAppDescriptorTrace->trace_avg_duration.c_str(), avgDurationUs.value());
    ATRACE_INT(mAppDescriptorTrace->trace_max_duration.c_str(), maxDurationUs.value());
    ATRACE_INT(mAppDescriptorTrace->trace_low_frame_rate.c_str(), isLowFPS);
    if (mSessTag == SessionTag::SURFACEFLINGER) {
        ATRACE_INT(mAppDescriptorTrace->trace_game_mode_fps.c_str(),
                   mSessionRecords->getLatestFPS());
        ATRACE_INT(mAppDescriptorTrace->trace_game_mode_fps_jitters.c_str(),
                   mSessionRecords->getNumOfFPSJitters());
    }
}

template <class HintManagerT, class PowerSessionManagerT>
ndk::ScopedAStatus PowerHintSession<HintManagerT, PowerSessionManagerT>::reportActualWorkDuration(
        const std::vector<WorkDuration> &actualDurations) {
    std::scoped_lock lock{mPowerHintSessionLock};
    if (mSessionClosed) {
        ALOGE("Error: session is dead");
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    if (mDescriptor->targetNs.count() == 0LL) {
        ALOGE("Expect to call updateTargetWorkDuration() first.");
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    if (actualDurations.empty()) {
        ALOGE("Error: durations shouldn't be empty.");
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    if (!mDescriptor->is_active.load()) {
        ALOGE("Error: shouldn't report duration during pause state.");
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    auto adpfConfig = getAdpfProfile();
    mDescriptor->update_count++;
    bool isFirstFrame = isTimeout();
    ATRACE_INT(mAppDescriptorTrace->trace_batch_size.c_str(), actualDurations.size());
    ATRACE_INT(mAppDescriptorTrace->trace_actl_last.c_str(), actualDurations.back().durationNanos);
    ATRACE_INT(mAppDescriptorTrace->trace_target.c_str(), mDescriptor->targetNs.count());
    ATRACE_INT(mAppDescriptorTrace->trace_hint_count.c_str(), mDescriptor->update_count);
    ATRACE_INT(mAppDescriptorTrace->trace_hint_overtime.c_str(),
               actualDurations.back().durationNanos - mDescriptor->targetNs.count() > 0);
    ATRACE_INT(mAppDescriptorTrace->trace_is_first_frame.c_str(), (isFirstFrame) ? (1) : (0));
    ATRACE_INT(mAppDescriptorTrace->trace_cpu_duration.c_str(),
               actualDurations.back().cpuDurationNanos);
    ATRACE_INT(mAppDescriptorTrace->trace_gpu_duration.c_str(),
               actualDurations.back().gpuDurationNanos);

    mLastUpdatedTime = std::chrono::steady_clock::now();
    if (isFirstFrame) {
        mPSManager->updateUniversalBoostMode();
    }

    mPSManager->disableBoosts(mSessionId);

    if (!adpfConfig->mPidOn) {
        updatePidControlVariable(adpfConfig->mUclampMinHigh);
        return ndk::ScopedAStatus::ok();
    }

    bool hboostEnabled =
            adpfConfig->mHeuristicBoostOn.has_value() && adpfConfig->mHeuristicBoostOn.value();

    if (hboostEnabled) {
        FrameBuckets newFramesInBuckets;
        mSessionRecords->addReportedDurations(
                actualDurations, mDescriptor->targetNs.count(), newFramesInBuckets,
                mSessTag == SessionTag::SURFACEFLINGER && mPSManager->getGameModeEnableState());
        mPSManager->updateHboostStatistics(mSessionId, mJankyLevel, actualDurations.size());
        mPSManager->updateFrameBuckets(mSessionId, newFramesInBuckets);
        updateHeuristicBoost();
    }

    int64_t output = convertWorkDurationToBoostByPid(actualDurations);

    // Apply to all the threads in the group
    auto uclampMinFloor = adpfConfig->mUclampMinLow;
    auto uclampMinCeiling = adpfConfig->mUclampMinHigh;
    if (hboostEnabled) {
        auto hboostMinUclampMinFloor = std::max(
                adpfConfig->mUclampMinLow, adpfConfig->mHBoostUclampMinFloorRange.value().first);
        auto hboostMaxUclampMinFloor = std::max(
                adpfConfig->mUclampMinLow, adpfConfig->mHBoostUclampMinFloorRange.value().second);
        auto hboostMinUclampMinCeiling = std::max(
                adpfConfig->mUclampMinHigh, adpfConfig->mHBoostUclampMinCeilingRange.value().first);
        auto hboostMaxUclampMinCeiling =
                std::max(adpfConfig->mUclampMinHigh,
                         adpfConfig->mHBoostUclampMinCeilingRange.value().second);
        if (mJankyLevel == SessionJankyLevel::MODERATE) {
            double JankyFactor =
                    mJankyFrameNum < adpfConfig->mHBoostModerateJankThreshold.value()
                            ? 0.0
                            : (mJankyFrameNum - adpfConfig->mHBoostModerateJankThreshold.value()) *
                                      1.0 /
                                      (adpfConfig->mHBoostSevereJankThreshold.value() -
                                       adpfConfig->mHBoostModerateJankThreshold.value());
            uclampMinFloor = hboostMinUclampMinFloor +
                             (hboostMaxUclampMinFloor - hboostMinUclampMinFloor) * JankyFactor;
            uclampMinCeiling =
                    hboostMinUclampMinCeiling +
                    (hboostMaxUclampMinCeiling - hboostMinUclampMinCeiling) * JankyFactor;
        } else if (mJankyLevel == SessionJankyLevel::SEVERE) {
            uclampMinFloor = hboostMaxUclampMinFloor;
            uclampMinCeiling = hboostMaxUclampMinCeiling;
        }
        ATRACE_INT(mAppDescriptorTrace->trace_uclamp_min_ceiling.c_str(), uclampMinCeiling);
        ATRACE_INT(mAppDescriptorTrace->trace_uclamp_min_floor.c_str(), uclampMinFloor);
    }

    int next_min = std::min(static_cast<int>(uclampMinCeiling),
                            mDescriptor->pidControlVariable + static_cast<int>(output));
    next_min = std::max(static_cast<int>(uclampMinFloor), next_min);

    updatePidControlVariable(next_min);

    if (!adpfConfig->mGpuBoostOn.value_or(false) || !adpfConfig->mGpuBoostCapacityMax ||
        !actualDurations.back().gpuDurationNanos) {
        return ndk::ScopedAStatus::ok();
    }

    auto const gpu_freq = mPSManager->gpuFrequency();
    if (!gpu_freq) {
        return ndk::ScopedAStatus::ok();
    }
    auto const additional_gpu_capacity =
            calculate_capacity(actualDurations.back(), mDescriptor->targetNs, *gpu_freq);
    ATRACE_INT(mAppDescriptorTrace->trace_gpu_capacity.c_str(),
               static_cast<int>(additional_gpu_capacity));

    auto const additional_gpu_capacity_clamped = std::clamp(
            additional_gpu_capacity, Cycles(0), Cycles(*adpfConfig->mGpuBoostCapacityMax));

    mPSManager->voteSet(
            mSessionId, AdpfVoteType::GPU_CAPACITY, additional_gpu_capacity_clamped,
            std::chrono::steady_clock::now(),
            duration_cast<nanoseconds>(mDescriptor->targetNs * adpfConfig->mStaleTimeFactor));

    return ndk::ScopedAStatus::ok();
}

template <class HintManagerT, class PowerSessionManagerT>
ndk::ScopedAStatus PowerHintSession<HintManagerT, PowerSessionManagerT>::sendHint(
        SessionHint hint) {
    {
        std::scoped_lock lock{mPowerHintSessionLock};
        if (mSessionClosed) {
            ALOGE("Error: session is dead");
            return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
        }
        if (mDescriptor->targetNs.count() == 0LL) {
            ALOGE("Expect to call updateTargetWorkDuration() first.");
            return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
        }
        auto adpfConfig = getAdpfProfile();

        switch (hint) {
            case SessionHint::CPU_LOAD_UP:
                updatePidControlVariable(mDescriptor->pidControlVariable);
                mPSManager->voteSet(mSessionId, AdpfVoteType::CPU_LOAD_UP,
                                    adpfConfig->mUclampMinLoadUp, kUclampMax,
                                    std::chrono::steady_clock::now(), mDescriptor->targetNs * 2);
                break;
            case SessionHint::CPU_LOAD_DOWN:
                updatePidControlVariable(adpfConfig->mUclampMinLow);
                break;
            case SessionHint::CPU_LOAD_RESET:
                updatePidControlVariable(
                        std::max(adpfConfig->mUclampMinInit,
                                 static_cast<uint32_t>(mDescriptor->pidControlVariable)),
                        false);
                mPSManager->voteSet(mSessionId, AdpfVoteType::CPU_LOAD_RESET,
                                    adpfConfig->mUclampMinLoadReset, kUclampMax,
                                    std::chrono::steady_clock::now(),
                                    duration_cast<nanoseconds>(mDescriptor->targetNs *
                                                               adpfConfig->mStaleTimeFactor / 2.0));
                break;
            case SessionHint::CPU_LOAD_RESUME:
                mPSManager->voteSet(mSessionId, AdpfVoteType::CPU_LOAD_RESUME,
                                    mDescriptor->pidControlVariable, kUclampMax,
                                    std::chrono::steady_clock::now(),
                                    duration_cast<nanoseconds>(mDescriptor->targetNs *
                                                               adpfConfig->mStaleTimeFactor / 2.0));
                break;
            case SessionHint::POWER_EFFICIENCY:
                setModeLocked(SessionMode::POWER_EFFICIENCY, true);
                break;
            case SessionHint::GPU_LOAD_UP:
                mPSManager->voteSet(mSessionId, AdpfVoteType::GPU_LOAD_UP,
                                    Cycles(adpfConfig->mGpuCapacityLoadUpHeadroom),
                                    std::chrono::steady_clock::now(), mDescriptor->targetNs);
                break;
            case SessionHint::GPU_LOAD_DOWN:
                // TODO(kevindubois): add impl
                break;
            case SessionHint::GPU_LOAD_RESET:
                // TODO(kevindubois): add impl
                break;
            case SessionHint::CPU_LOAD_SPIKE:
                // TODO(mattbuckley): add impl
                break;
            case SessionHint::GPU_LOAD_SPIKE:
                // TODO(kevindubois): add impl
                break;
            default:
                ALOGE("Error: hint is invalid");
                return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        }
        mLastUpdatedTime = std::chrono::steady_clock::now();
    }
    // Don't hold a lock (mPowerHintSession) while DoHint will try to take another
    // lock(NodeLooperThread).
    tryToSendPowerHint(toString(hint));
    return ndk::ScopedAStatus::ok();
}

template <class HintManagerT, class PowerSessionManagerT>
ndk::ScopedAStatus PowerHintSession<HintManagerT, PowerSessionManagerT>::setMode(SessionMode mode,
                                                                                 bool enabled) {
    std::scoped_lock lock{mPowerHintSessionLock};
    return setModeLocked(mode, enabled);
}

template <class HintManagerT, class PowerSessionManagerT>
ndk::ScopedAStatus PowerHintSession<HintManagerT, PowerSessionManagerT>::setModeLocked(
        SessionMode mode, bool enabled) {
    if (mSessionClosed) {
        ALOGE("Error: session is dead");
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }

    switch (mode) {
        case SessionMode::POWER_EFFICIENCY:
            mPSManager->setPreferPowerEfficiency(mSessionId, enabled);
            break;
        default:
            ALOGE("Error: mode is invalid");
            return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    mModes[static_cast<size_t>(mode)] = enabled;
    ATRACE_INT(mAppDescriptorTrace->trace_modes[static_cast<size_t>(mode)].c_str(), enabled);
    mLastUpdatedTime = std::chrono::steady_clock::now();
    return ndk::ScopedAStatus::ok();
}

template <class HintManagerT, class PowerSessionManagerT>
ndk::ScopedAStatus PowerHintSession<HintManagerT, PowerSessionManagerT>::setThreads(
        const std::vector<int32_t> &threadIds) {
    std::scoped_lock lock{mPowerHintSessionLock};
    if (mSessionClosed) {
        ALOGE("Error: session is dead");
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    if (threadIds.empty()) {
        ALOGE("Error: threadIds should not be empty");
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    mDescriptor->thread_ids = threadIds;
    mPSManager->setThreadsFromPowerSession(mSessionId, threadIds, mProcTag);
    // init boost
    updatePidControlVariable(getAdpfProfile()->mUclampMinInit);
    return ndk::ScopedAStatus::ok();
}

template <class HintManagerT, class PowerSessionManagerT>
ndk::ScopedAStatus PowerHintSession<HintManagerT, PowerSessionManagerT>::getSessionConfig(
        SessionConfig *_aidl_return) {
    _aidl_return->id = mSessionId;
    return ndk::ScopedAStatus::ok();
}

template <class HintManagerT, class PowerSessionManagerT>
SessionTag PowerHintSession<HintManagerT, PowerSessionManagerT>::getSessionTag() const {
    return mSessTag;
}

template <class HintManagerT, class PowerSessionManagerT>
const std::shared_ptr<AdpfConfig>
PowerHintSession<HintManagerT, PowerSessionManagerT>::getAdpfProfile() const {
    if (!mAdpfProfile) {
        return mProcTag == ProcessTag::DEFAULT
                       ? HintManager::GetInstance()->GetAdpfProfile(toString(mSessTag))
                       : HintManager::GetInstance()->GetAdpfProfile(toString(mProcTag));
    }
    return mAdpfProfile;
}

template <class HintManagerT, class PowerSessionManagerT>
void PowerHintSession<HintManagerT, PowerSessionManagerT>::setAdpfProfile(
        const std::shared_ptr<AdpfConfig> profile) {
    // Must prevent profile from being changed in a binder call duration.
    std::scoped_lock lock{mPowerHintSessionLock};
    mAdpfProfile = profile;
}

std::string AppHintDesc::toString() const {
    std::string out = StringPrintf("session %" PRId64 "\n", sessionId);
    out.append(
            StringPrintf("  duration: %" PRId64 " ns\n", static_cast<int64_t>(targetNs.count())));
    out.append(StringPrintf("  uclamp.min: %d \n", pidControlVariable));
    out.append(StringPrintf("  uid: %d, tgid: %d\n", uid, tgid));
    return out;
}

template <class HintManagerT, class PowerSessionManagerT>
bool PowerHintSession<HintManagerT, PowerSessionManagerT>::isTimeout() {
    auto now = std::chrono::steady_clock::now();
    time_point<steady_clock> staleTime =
            mLastUpdatedTime +
            nanoseconds(static_cast<int64_t>(mDescriptor->targetNs.count() *
                                             getAdpfProfile()->mStaleTimeFactor));
    return now >= staleTime;
}

template class PowerHintSession<>;
template class PowerHintSession<testing::NiceMock<mock::pixel::MockHintManager>,
                                testing::NiceMock<mock::pixel::MockPowerSessionManager>>;
template class PowerHintSession<
        testing::NiceMock<mock::pixel::MockHintManager>,
        PowerSessionManager<testing::NiceMock<mock::pixel::MockHintManager>>>;
}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
