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

#include <aidl/android/hardware/power/BnPowerHintSession.h>
#include <perfmgr/HintManager.h>
#include <utils/Looper.h>
#include <utils/Thread.h>

#include <array>
#include <unordered_map>

#include "AdpfTypes.h"
#include "AppDescriptorTrace.h"
#include "PowerSessionManager.h"
#include "SessionRecords.h"

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

using aidl::android::hardware::power::BnPowerHintSession;
using ::android::Message;
using ::android::MessageHandler;
using ::android::sp;
using ::android::perfmgr::AdpfConfig;
using std::chrono::milliseconds;
using std::chrono::nanoseconds;
using std::chrono::steady_clock;
using std::chrono::time_point;

// The Power Hint Session is responsible for providing an
// interface for creating, updating, and closing power hints
// for a Session. Each sesion that is mapped to multiple
// threads (or task ids).
template <class HintManagerT = ::android::perfmgr::HintManager,
          class PowerSessionManagerT = PowerSessionManager<>>
class PowerHintSession : public BnPowerHintSession, public Immobile {
  public:
    explicit PowerHintSession(int32_t tgid, int32_t uid, const std::vector<int32_t> &threadIds,
                              int64_t durationNanos, SessionTag tag);
    ~PowerHintSession();
    ndk::ScopedAStatus close() override;
    ndk::ScopedAStatus pause() override;
    ndk::ScopedAStatus resume() override;
    ndk::ScopedAStatus updateTargetWorkDuration(int64_t targetDurationNanos) override;
    ndk::ScopedAStatus reportActualWorkDuration(
            const std::vector<WorkDuration> &actualDurations) override;
    ndk::ScopedAStatus sendHint(SessionHint hint) override;
    ndk::ScopedAStatus setMode(SessionMode mode, bool enabled) override;
    ndk::ScopedAStatus setThreads(const std::vector<int32_t> &threadIds) override;
    ndk::ScopedAStatus getSessionConfig(SessionConfig *_aidl_return) override;

    void dumpToStream(std::ostream &stream);
    SessionTag getSessionTag() const;
    void setAdpfProfile(const std::shared_ptr<AdpfConfig> profile);

  private:
    // In practice this lock should almost never get contested, but it's necessary for FMQ
    std::mutex mPowerHintSessionLock;
    bool isTimeout() REQUIRES(mPowerHintSessionLock);
    // Is hint session for a user application
    bool isAppSession() REQUIRES(mPowerHintSessionLock);
    void tryToSendPowerHint(std::string hint);
    void updatePidControlVariable(int pidControlVariable, bool updateVote = true)
            REQUIRES(mPowerHintSessionLock);
    int64_t convertWorkDurationToBoostByPid(const std::vector<WorkDuration> &actualDurations)
            REQUIRES(mPowerHintSessionLock);
    SessionJankyLevel updateSessionJankState(SessionJankyLevel oldState, int32_t numOfJankFrames,
                                             double durationVariance, bool isLowFPS)
            REQUIRES(mPowerHintSessionLock);
    void updateHeuristicBoost() REQUIRES(mPowerHintSessionLock);
    void resetSessionHeuristicStates() REQUIRES(mPowerHintSessionLock);
    const std::shared_ptr<AdpfConfig> getAdpfProfile() const;
    ProcessTag getProcessTag(int32_t tgid);
    ndk::ScopedAStatus setModeLocked(SessionMode mode, bool enabled)
            REQUIRES(mPowerHintSessionLock);

    // Data
    PowerSessionManagerT *mPSManager;
    const int64_t mSessionId = 0;
    // Tag labeling what kind of session this is
    const SessionTag mSessTag;
    // Pixel process tag for more granular session control.
    const ProcessTag mProcTag{ProcessTag::DEFAULT};
    const std::string mIdString;
    std::shared_ptr<AppHintDesc> mDescriptor GUARDED_BY(mPowerHintSessionLock);

    // Trace strings, this is thread safe since only assigned during construction
    std::shared_ptr<AppDescriptorTrace> mAppDescriptorTrace;
    time_point<steady_clock> mLastUpdatedTime GUARDED_BY(mPowerHintSessionLock);
    bool mSessionClosed GUARDED_BY(mPowerHintSessionLock) = false;
    // Are cpu load change related hints are supported
    std::unordered_map<std::string, std::optional<bool>> mSupportedHints;
    // Use the value of the last enum in enum_range +1 as array size
    std::array<bool, enum_size<SessionMode>()> mModes GUARDED_BY(mPowerHintSessionLock){};
    std::shared_ptr<AdpfConfig> mAdpfProfile;
    std::function<void(const std::shared_ptr<AdpfConfig>)> mOnAdpfUpdate;
    std::unique_ptr<SessionRecords> mSessionRecords GUARDED_BY(mPowerHintSessionLock) = nullptr;
    bool mHeuristicBoostActive GUARDED_BY(mPowerHintSessionLock){false};
    SessionJankyLevel mJankyLevel GUARDED_BY(mPowerHintSessionLock){SessionJankyLevel::LIGHT};
    uint32_t mJankyFrameNum GUARDED_BY(mPowerHintSessionLock){0};
};

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
