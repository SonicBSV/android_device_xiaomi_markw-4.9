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

// Keeps track of groups of channels that all use the same EventFlag

#pragma once

#include <android-base/thread_annotations.h>

#include <array>
#include <mutex>
#include <optional>
#include <thread>

#include "AdpfTypes.h"
#include "PowerHintSession.h"
#include "PowerSessionManager.h"
#include "SessionChannel.h"

namespace aidl::google::hardware::power::impl::pixel {
// This manages a group of FMQ channels that are watched by a single thread. This class manages the
// channels, the thread that waits on their callbacks, and the mutex for that thread.
//
// We have the channels guarded by a lock in this class and the channel count guarded by
// the manager, because adding/removing is only done by the manager while locked.
// Thus, only the manager lock is required to count the group size when figuring out where
// to insert a new channel.
template <class PowerSessionManagerT = PowerSessionManager<>,
          class PowerHintSessionT = PowerHintSession<>>
class ChannelGroup : public Immobile {
  public:
    ~ChannelGroup();
    ChannelGroup(int32_t id);
    bool removeChannel(int32_t slot) EXCLUDES(mGroupMutex);
    int32_t getChannelCount() const;
    // Returns the channel ID
    std::shared_ptr<SessionChannel> createChannel(int32_t tgid, int32_t uid) EXCLUDES(mGroupMutex);
    std::shared_ptr<SessionChannel> getChannel(int32_t slot) EXCLUDES(mGroupMutex);
    void getFlagDesc(std::optional<FlagQueueDesc> *_return_desc) const;

  private:
    void runChannelGroup() EXCLUDES(mGroupMutex);

    // Guard the number of channels with the global lock, so we only need one
    // lock in order to figure out where to insert new sessions, instead of getting
    // a lock for each channelgroup.
    int32_t mLiveChannels = 0;
    const int32_t mGroupId;

    // Tracks whether the group is currently being destructed, used to kill the helper thread
    bool mDestructing = false;
    // Used to guard items internal to the FMQ thread
    std::mutex mGroupMutex;
    std::array<std::shared_ptr<SessionChannel>, kMaxChannels> mChannels GUARDED_BY(mGroupMutex){};
    const std::shared_ptr<FlagQueue> mFlagQueue;

    std::thread mGroupThread;
};

}  // namespace aidl::google::hardware::power::impl::pixel
