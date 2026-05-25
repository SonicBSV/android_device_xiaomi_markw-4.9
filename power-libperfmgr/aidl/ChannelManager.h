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

#pragma once

#include <map>
#include <unordered_map>

#include "AdpfTypes.h"
#include "ChannelGroup.h"
#include "SessionChannel.h"

namespace aidl::google::hardware::power::impl::pixel {

template <class ChannelGroupT = ChannelGroup<>>
class ChannelManager : public Immobile {
  public:
    ~ChannelManager() = default;
    bool closeChannel(int32_t tgid, int32_t uid);
    bool getChannelConfig(int32_t tgid, int32_t uid, ChannelConfig *config);
    int getGroupCount();
    int getChannelCount();
    // The instance of this class is actually owned by the PowerSessionManager singleton
    // This is mostly to reduce the number of singletons and make it simpler to mock
    static ChannelManager *getInstance();

    union ChannelMapKey {
        struct {
            int32_t tgid;
            int32_t uid;
        };
        int64_t key;
        operator int64_t() { return key; }
    };

    union ChannelMapValue {
        struct {
            int32_t groupId;
            int32_t offset;
        };
        int64_t value;
        operator int64_t() { return value; }
    };

  private:
    std::mutex mChannelManagerMutex;

    std::map<int32_t, ChannelGroupT> mChannelGroups GUARDED_BY(mChannelManagerMutex);
    std::shared_ptr<SessionChannel> getOrCreateChannel(int32_t tgid, int32_t uid)
            REQUIRES(mChannelManagerMutex);

    // Used to look up where channels actually are in this data structure, and guarantee uniqueness
    std::unordered_map<int64_t, int64_t> mChannelMap GUARDED_BY(mChannelManagerMutex);
};

}  // namespace aidl::google::hardware::power::impl::pixel
