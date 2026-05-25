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

#define LOG_TAG "powerhal-libperfmgr"
#define ATRACE_TAG (ATRACE_TAG_POWER | ATRACE_TAG_HAL)

#include "ChannelGroup.h"

#include <gmock/gmock.h>
#include <processgroup/processgroup.h>
#include <sys/resource.h>
#include <utils/SystemClock.h>
#include <utils/Trace.h>

#include <algorithm>
#include <mutex>

#include "AdpfTypes.h"
#include "ChannelManager.h"
#include "log/log_main.h"
#include "tests/mocks/MockPowerHintSession.h"
#include "tests/mocks/MockPowerSessionManager.h"

namespace aidl::google::hardware::power::impl::pixel {
using Tag = ChannelMessage::ChannelMessageContents::Tag;

template <class PowerSessionManagerT, class PowerHintSessionT>
ChannelGroup<PowerSessionManagerT, PowerHintSessionT>::ChannelGroup(int32_t id)
    : mGroupId(id),
      mFlagQueue(std::make_shared<FlagQueue>(1, true)),
      mGroupThread(std::thread(&ChannelGroup::runChannelGroup, this)) {}

template <class PowerSessionManagerT, class PowerHintSessionT>
ChannelGroup<PowerSessionManagerT, PowerHintSessionT>::~ChannelGroup() {
    mDestructing = true;

    EventFlag *flag;
    EventFlag::createEventFlag(mFlagQueue->getEventFlagWord(), &flag);
    // Wake up the handler. 0xffffffff wakes on every bit in the flag,
    // to ensure the wake-up will be handled regardless of other configuration settings,
    // and even if some of these bits are already set.
    flag->wake(0xffffffff);

    mGroupThread.join();
}

template <class PowerSessionManagerT, class PowerHintSessionT>
int32_t ChannelGroup<PowerSessionManagerT, PowerHintSessionT>::getChannelCount() const {
    return mLiveChannels;
}

template <class PowerSessionManagerT, class PowerHintSessionT>
bool ChannelGroup<PowerSessionManagerT, PowerHintSessionT>::removeChannel(int32_t slot) {
    std::scoped_lock lock(mGroupMutex);
    if (!mChannels[slot]) {
        return false;
    }
    mChannels[slot] = nullptr;
    --mLiveChannels;
    return true;
}

template <class PowerSessionManagerT, class PowerHintSessionT>
std::shared_ptr<SessionChannel>
ChannelGroup<PowerSessionManagerT, PowerHintSessionT>::createChannel(int32_t tgid, int32_t uid) {
    std::scoped_lock lock(mGroupMutex);
    ALOGV("Creating channel for tgid: %" PRId32 " uid: %" PRId32, tgid, uid);
    int slot = 0;
    for (slot = 0; slot < kMaxChannels; ++slot) {
        if (!mChannels[slot]) {
            break;
        }
    }
    LOG_ALWAYS_FATAL_IF(slot == kMaxChannels, "Failed to create channel!");
    ++mLiveChannels;
    ChannelManager<SessionChannel>::ChannelMapValue channelId{
            {.groupId = static_cast<int32_t>(mGroupId), .offset = slot}};
    mChannels[slot] = std::make_shared<SessionChannel>(tgid, uid, channelId, slot);
    ALOGV("Channel created on group: %" PRId32 " slot: %" PRId32, mGroupId, slot);
    return mChannels[slot];
}

template <class PowerSessionManagerT, class PowerHintSessionT>
std::shared_ptr<SessionChannel> ChannelGroup<PowerSessionManagerT, PowerHintSessionT>::getChannel(
        int32_t slot) {
    std::scoped_lock lock(mGroupMutex);
    LOG_ALWAYS_FATAL_IF(!mChannels[slot], "Requesting a dead channel!");
    return mChannels[slot];
}

template <class PowerSessionManagerT, class PowerHintSessionT>
void ChannelGroup<PowerSessionManagerT, PowerHintSessionT>::getFlagDesc(
        std::optional<FlagQueueDesc> *_return_desc) const {
    *_return_desc = std::make_optional(mFlagQueue->dupeDesc());
}

template <class PowerSessionManagerT, class PowerHintSessionT>
void ChannelGroup<PowerSessionManagerT, PowerHintSessionT>::runChannelGroup() {
    EventFlag *flag;
    {
        std::scoped_lock lock(mGroupMutex);
        EventFlag::createEventFlag(mFlagQueue->getEventFlagWord(), &flag);
    }

    setpriority(PRIO_PROCESS, getpid(), -20);

    uint32_t flagState = 0;
    static std::set<uid_t> blocklist = {};
    std::vector<ChannelMessage> messages;
    std::vector<android::hardware::power::WorkDuration> durations;
    durations.reserve(kFMQQueueSize);
    messages.reserve(kFMQQueueSize);

    while (!mDestructing) {
        messages.clear();
        flag->wait(kWriteBits, &flagState, 0, true);
        int64_t now = ::android::uptimeNanos();
        if (mDestructing) {
            return;
        }
        {
            std::scoped_lock lock(mGroupMutex);
            // Get the rightmost nonzero bit, corresponding to the next active channel
            for (int channelNum = std::countr_zero(flagState);
                 channelNum < kMaxChannels && !mDestructing;
                 channelNum = std::countr_zero(flagState)) {
                // Drop the lowest set write bit
                flagState &= (flagState - 1);
                auto &channel = mChannels[channelNum];
                if (!channel || !channel->isValid()) {
                    continue;
                }
                if (blocklist.contains(channel->getUid())) {
                    continue;
                }
                int toRead = channel->getQueue()->availableToRead();
                if (toRead <= 0) {
                    continue;
                }
                messages.resize(toRead);
                if (!channel->getQueue()->read(messages.data(), toRead)) {
                    // stop messing with your buffer >:(
                    blocklist.insert(channel->getUid());
                    continue;
                }
                flag->wake(mChannels[channelNum]->getReadBitmask());
                for (int messageIndex = 0; messageIndex < messages.size() && !mDestructing;
                     ++messageIndex) {
                    ChannelMessage &message = messages[messageIndex];
                    auto sessionPtr = std::static_pointer_cast<PowerHintSessionT>(
                            PowerSessionManagerT::getInstance()->getSession(message.sessionID));
                    if (!sessionPtr) {
                        continue;
                    }
                    switch (message.data.getTag()) {
                        case Tag::hint: {
                            sessionPtr->sendHint(message.data.get<Tag::hint>());
                            break;
                        }
                        case Tag::targetDuration: {
                            sessionPtr->updateTargetWorkDuration(
                                    message.data.get<Tag::targetDuration>());
                            break;
                        }
                        case Tag::workDuration: {
                            durations.clear();
                            for (; !mDestructing && messageIndex < messages.size() &&
                                   messages[messageIndex].data.getTag() == Tag::workDuration &&
                                   messages[messageIndex].sessionID == message.sessionID;
                                 ++messageIndex) {
                                ChannelMessage &currentMessage = messages[messageIndex];
                                auto &durationData = currentMessage.data.get<Tag::workDuration>();
                                durations.emplace_back(WorkDuration{
                                        .timeStampNanos = currentMessage.timeStampNanos,
                                        .durationNanos = durationData.durationNanos,
                                        .cpuDurationNanos = durationData.cpuDurationNanos,
                                        .gpuDurationNanos = durationData.gpuDurationNanos,
                                        .workPeriodStartTimestampNanos =
                                                durationData.workPeriodStartTimestampNanos});
                            }
                            sessionPtr->reportActualWorkDuration(durations);
                            break;
                        }
                        case Tag::mode: {
                            auto mode = message.data.get<Tag::mode>();
                            sessionPtr->setMode(mode.modeInt, mode.enabled);
                            break;
                        }
                        default: {
                            ALOGE("Invalid data tag sent: %s",
                                  std::to_string(static_cast<int>(message.data.getTag())).c_str());
                            break;
                        }
                    }
                }
            }
        }
    }
}

template class ChannelGroup<>;
template class ChannelGroup<testing::NiceMock<mock::pixel::MockPowerSessionManager>,
                            testing::NiceMock<mock::pixel::MockPowerHintSession>>;

}  // namespace aidl::google::hardware::power::impl::pixel
