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

#include "ChannelManager.h"

#include <inttypes.h>

#include "tests/mocks/MockChannelGroup.h"
#include "tests/mocks/MockPowerHintSession.h"
#include "tests/mocks/MockPowerSessionManager.h"

namespace aidl::google::hardware::power::impl::pixel {

template <class ChannelGroupT>
bool ChannelManager<ChannelGroupT>::closeChannel(int32_t tgid, int32_t uid) {
    std::scoped_lock lock{mChannelManagerMutex};
    ChannelMapKey key{.tgid = tgid, .uid = uid};
    auto channelIter = mChannelMap.find(key);
    if (channelIter == mChannelMap.end()) {
        return false;
    }
    ChannelMapValue value{.value = channelIter->second};
    auto groupIter = mChannelGroups.find(value.groupId);
    if (groupIter == mChannelGroups.end()) {
        return false;
    }

    if (!groupIter->second.removeChannel(value.offset)) {
        return false;
    }

    // Ensure the group is cleaned up if we remove the last channel
    if (groupIter->second.getChannelCount() == 0) {
        mChannelGroups.erase(groupIter);
    }
    mChannelMap.erase(channelIter);
    return true;
}

template <class ChannelGroupT>
std::shared_ptr<SessionChannel> ChannelManager<ChannelGroupT>::getOrCreateChannel(int32_t tgid,
                                                                                  int32_t uid) {
    ChannelMapKey key{.tgid = tgid, .uid = uid};
    auto channelIter = mChannelMap.find(key);
    if (channelIter != mChannelMap.end()) {
        ChannelMapValue value{.value = channelIter->second};
        return mChannelGroups.at(value.groupId).getChannel(value.offset);
    }
    // If channel does not exist, we need to create it
    int32_t groupId = -1;
    for (auto &&group : mChannelGroups) {
        if (group.second.getChannelCount() < kMaxChannels) {
            groupId = group.first;
            break;
        }
    }
    // No group was found, we need to make a new one
    if (groupId == -1) {
        groupId = mChannelGroups.empty() ? 0 : mChannelGroups.rbegin()->first + 1;
        mChannelGroups.emplace(std::piecewise_construct, std::forward_as_tuple(groupId),
                               std::forward_as_tuple(groupId));
    }

    std::shared_ptr<SessionChannel> channel = mChannelGroups.at(groupId).createChannel(tgid, uid);
    mChannelMap[key] = channel->getId();

    return channel;
}

template <class ChannelGroupT>
bool ChannelManager<ChannelGroupT>::getChannelConfig(int32_t tgid, int32_t uid,
                                                     ChannelConfig *config) {
    std::scoped_lock lock{mChannelManagerMutex};
    std::shared_ptr<SessionChannel> channel = getOrCreateChannel(tgid, uid);
    if (!channel || !channel->isValid()) {
        return false;
    }
    mChannelGroups.at(ChannelMapValue{.value = channel->getId()}.groupId)
            .getFlagDesc(&config->eventFlagDescriptor);
    channel->getDesc(&config->channelDescriptor);
    config->readFlagBitmask = channel->getReadBitmask();
    config->writeFlagBitmask = channel->getWriteBitmask();
    return true;
}

template <class ChannelGroupT>
int ChannelManager<ChannelGroupT>::getGroupCount() {
    std::scoped_lock lock{mChannelManagerMutex};
    return mChannelGroups.size();
}

template <class ChannelGroupT>
int ChannelManager<ChannelGroupT>::getChannelCount() {
    std::scoped_lock lock{mChannelManagerMutex};
    int out = 0;
    for (auto &&group : mChannelGroups) {
        out += group.second.getChannelCount();
    }
    return out;
}

template <class ChannelGroupT>
ChannelManager<ChannelGroupT> *ChannelManager<ChannelGroupT>::getInstance() {
    static ChannelManager instance{};
    return &instance;
}

template class ChannelManager<>;
template class ChannelManager<testing::NiceMock<mock::pixel::MockChannelGroup>>;
template class ChannelManager<ChannelGroup<testing::NiceMock<mock::pixel::MockPowerSessionManager>,
                                           testing::NiceMock<mock::pixel::MockPowerHintSession>>>;

}  // namespace aidl::google::hardware::power::impl::pixel
