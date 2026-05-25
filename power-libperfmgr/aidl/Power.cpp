/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include "Power.h"

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android-base/strings.h>
#include <fmq/AidlMessageQueue.h>
#include <fmq/EventFlag.h>
#include <perfmgr/HintManager.h>
#include <utils/Log.h>

#include <cstdint>
#include <memory>
#include <optional>

#include "AdpfTypes.h"
#include "ChannelManager.h"
#include "PowerHintSession.h"
#include "PowerSessionManager.h"
#include "SupportManager.h"

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {
using ::android::perfmgr::HintManager;

constexpr char kPowerHalStateProp[] = "vendor.powerhal.state";
constexpr char kPowerHalAudioProp[] = "vendor.powerhal.audio";
constexpr char kPowerHalRenderingProp[] = "vendor.powerhal.rendering";
constexpr char kTapToWakeNode[] = "/proc/gesture/onoff";

extern bool isDeviceSpecificModeSupported(Mode type, bool* _aidl_return);
extern bool setDeviceSpecificMode(Mode type, bool enabled);

Power::Power()
    : mInteractionHandler(nullptr),
      mSustainedPerfModeOn(false) {
    mInteractionHandler = std::make_unique<InteractionHandler>();
    mInteractionHandler->Init();

    std::string state = ::android::base::GetProperty(kPowerHalStateProp, "");
    if (state == "SUSTAINED_PERFORMANCE") {
        LOG(INFO) << "Initialize with SUSTAINED_PERFORMANCE on";
        HintManager::GetInstance()->DoHint("SUSTAINED_PERFORMANCE");
        mSustainedPerfModeOn = true;
    } else {
        LOG(INFO) << "Initialize PowerHAL";
    }

    state = ::android::base::GetProperty(kPowerHalAudioProp, "");
    if (state == "AUDIO_STREAMING_LOW_LATENCY") {
        LOG(INFO) << "Initialize with AUDIO_LOW_LATENCY on";
        HintManager::GetInstance()->DoHint(state);
    }

    state = ::android::base::GetProperty(kPowerHalRenderingProp, "");
    if (state == "EXPENSIVE_RENDERING") {
        LOG(INFO) << "Initialize with EXPENSIVE_RENDERING on";
        HintManager::GetInstance()->DoHint("EXPENSIVE_RENDERING");
    }

    auto status = this->getInterfaceVersion(&mServiceVersion);
    LOG(INFO) << "PowerHAL InterfaceVersion:" << mServiceVersion << " isOK: " << status.isOk();

    mSupportInfo = SupportManager::makeSupportInfo();
}

ndk::ScopedAStatus Power::setMode(Mode type, bool enabled) {
    LOG(DEBUG) << "Power setMode: " << toString(type) << " to: " << enabled;
    if (HintManager::GetInstance()->IsAdpfSupported()) {
        PowerSessionManager<>::getInstance()->updateHintMode(toString(type), enabled);
    }
    if (setDeviceSpecificMode(type, enabled)) {
        return ndk::ScopedAStatus::ok();
    }

    switch (type) {
        case Mode::SUSTAINED_PERFORMANCE:
            if (enabled) {
                HintManager::GetInstance()->DoHint("SUSTAINED_PERFORMANCE");
            }
            mSustainedPerfModeOn = true;
            break;
        case Mode::DOUBLE_TAP_TO_WAKE:
            {
                // Записываем 1 (включено) или 0 (выключено) в узел ядра
                bool success = ::android::base::WriteStringToFile(enabled ? "1" : "0", kTapToWakeNode);
                if (!success) {
                    PLOG(ERROR) << "Failed to write to tap to wake node: " << kTapToWakeNode;
                }
                // Возвращаем OK, чтобы не проваливаться в default (DoHint)
                return ndk::ScopedAStatus::ok();
            }            
        case Mode::LAUNCH:
            if (mSustainedPerfModeOn) {
                break;
            }
            [[fallthrough]];
        // case Mode::DOUBLE_TAP_TO_WAKE: 
        //    [[fallthrough]];
        case Mode::FIXED_PERFORMANCE:
            [[fallthrough]];
        case Mode::EXPENSIVE_RENDERING:
            [[fallthrough]];
        case Mode::INTERACTIVE:
            [[fallthrough]];
        case Mode::DEVICE_IDLE:
            [[fallthrough]];
        case Mode::DISPLAY_INACTIVE:
            [[fallthrough]];
        case Mode::AUDIO_STREAMING_LOW_LATENCY:
            [[fallthrough]];
        case Mode::GAME_LOADING:
            [[fallthrough]];
        default:
            if (enabled) {
                HintManager::GetInstance()->DoHint(toString(type));
            } else {
                HintManager::GetInstance()->EndHint(toString(type));
            }
            break;
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::isModeSupported(Mode type, bool *_aidl_return) {
    if (isDeviceSpecificModeSupported(type, _aidl_return)) {
        return ndk::ScopedAStatus::ok();
    }
    bool supported = supportFromBitset(mSupportInfo.modes, type);
    LOG(INFO) << "Power Mode " << toString(type) << " isModeSupported: " << supported;
    *_aidl_return = supported;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::setBoost(Boost type, int32_t durationMs) {
    LOG(DEBUG) << "Power setBoost: " << toString(type) << " duration: " << durationMs;
    switch (type) {
        case Boost::INTERACTION:
            if (mSustainedPerfModeOn) {
                break;
            }
            mInteractionHandler->Acquire(durationMs);
            break;
        case Boost::DISPLAY_UPDATE_IMMINENT:
            [[fallthrough]];
        case Boost::ML_ACC:
            [[fallthrough]];
        case Boost::AUDIO_LAUNCH:
            [[fallthrough]];
        default:
            if (mSustainedPerfModeOn) {
                break;
            }
            if (durationMs > 0) {
                HintManager::GetInstance()->DoHint(toString(type),
                                                   std::chrono::milliseconds(durationMs));
            } else if (durationMs == 0) {
                HintManager::GetInstance()->DoHint(toString(type));
            } else {
                HintManager::GetInstance()->EndHint(toString(type));
            }
            break;
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::isBoostSupported(Boost type, bool *_aidl_return) {
    bool supported = supportFromBitset(mSupportInfo.boosts, type);
    LOG(INFO) << "Power oost " << toString(type) << " isBoostSupported: " << supported;
    *_aidl_return = supported;
    return ndk::ScopedAStatus::ok();
}

constexpr const char *boolToString(bool b) {
    return b ? "true" : "false";
}

binder_status_t Power::dump(int fd, const char **, uint32_t) {
    std::string buf(::android::base::StringPrintf(
            "HintManager Running: %s\n"
            "SustainedPerformanceMode: %s\n",
            boolToString(HintManager::GetInstance()->IsRunning()),
            boolToString(mSustainedPerfModeOn)));
    // Dump nodes through libperfmgr
    HintManager::GetInstance()->DumpToFd(fd);
    PowerSessionManager<>::getInstance()->dumpToFd(fd);
    if (!::android::base::WriteStringToFd(buf, fd)) {
        PLOG(ERROR) << "Failed to dump state to fd";
    }
    fsync(fd);
    return STATUS_OK;
}

ndk::ScopedAStatus Power::getCpuHeadroom(const CpuHeadroomParams &_, CpuHeadroomResult *) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Power::getGpuHeadroom(const GpuHeadroomParams &_, GpuHeadroomResult *) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Power::createHintSession(int32_t tgid, int32_t uid,
                                            const std::vector<int32_t> &threadIds,
                                            int64_t durationNanos,
                                            std::shared_ptr<IPowerHintSession> *_aidl_return) {
    SessionConfig config;
    return createHintSessionWithConfig(tgid, uid, threadIds, durationNanos, SessionTag::OTHER,
                                       &config, _aidl_return);
}

ndk::ScopedAStatus Power::getHintSessionPreferredRate(int64_t *outNanoseconds) {
    *outNanoseconds = HintManager::GetInstance()->IsAdpfSupported()
                              ? HintManager::GetInstance()->GetAdpfProfile()->mReportingRateLimitNs
                              : 0;
    if (*outNanoseconds <= 0) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::createHintSessionWithConfig(
        int32_t tgid, int32_t uid, const std::vector<int32_t> &threadIds, int64_t durationNanos,
        SessionTag tag, SessionConfig *config, std::shared_ptr<IPowerHintSession> *_aidl_return) {
    if (!HintManager::GetInstance()->IsAdpfSupported()) {
        *_aidl_return = nullptr;
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
    if (threadIds.size() == 0) {
        LOG(ERROR) << "Error: threadIds.size() shouldn't be " << threadIds.size();
        *_aidl_return = nullptr;
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    std::shared_ptr<PowerHintSession<>> session =
            ndk::SharedRefBase::make<PowerHintSession<>>(tgid, uid, threadIds, durationNanos, tag);

    *_aidl_return = session;
    session->getSessionConfig(config);
    PowerSessionManager<>::getInstance()->registerSession(session, config->id);

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::getSessionChannel(int32_t tgid, int32_t uid,
                                            ChannelConfig *_aidl_return) {
    if (ChannelManager<>::getInstance()->getChannelConfig(tgid, uid, _aidl_return)) {
        return ndk::ScopedAStatus::ok();
    }
    return ndk::ScopedAStatus::fromStatus(EX_ILLEGAL_STATE);
}

ndk::ScopedAStatus Power::closeSessionChannel(int32_t tgid, int32_t uid) {
    ChannelManager<>::getInstance()->closeChannel(tgid, uid);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::getSupportInfo(SupportInfo *_aidl_return) {
    // Copy the support object into the binder
    *_aidl_return = mSupportInfo;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::sendCompositionData(const std::vector<CompositionData> &) {
    LOG(INFO) << "Composition data received!";
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::sendCompositionUpdate(const CompositionUpdate &) {
    LOG(INFO) << "Composition update received!";
    return ndk::ScopedAStatus::ok();
}

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
