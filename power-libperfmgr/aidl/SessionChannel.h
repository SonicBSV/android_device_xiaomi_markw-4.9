/*
 * Copyright (C) 2024 The Android Open Source Project
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

#include <cstdint>

#include "AdpfTypes.h"

namespace aidl::google::hardware::power::impl::pixel {

class SessionChannel {
  public:
    SessionChannel(int32_t tgid, int32_t uid, int64_t id, int32_t offset);
    void getDesc(ChannelQueueDesc *_return_desc);
    bool isValid() const;
    ChannelQueue *getQueue();
    int32_t getTgid() const;
    int32_t getUid() const;
    int64_t getId() const;
    // write is the lowest 16 bits, read is the upper 16 bits
    uint32_t getWriteBitmask() const;
    uint32_t getReadBitmask() const;

  private:
    int32_t mTgid = -1;
    int32_t mUid = -1;
    // An ID starting from 0 increasing sequentially, representing
    // location in the session array. If a channel dies, it isn't removed
    // but killed, so that the order remains the same for everyone. It will
    // be replaced when new sessions come along. The first 15 sessions
    // all get unique sets of bits to communicate with their client,
    // and 16+ have to share the last slot. It's worth considering making
    // another thread if we go beyond 16.
    const int64_t mId = -1;
    const uint32_t mReadMask = 0;
    const uint32_t mWriteMask = 0;
    ChannelQueue mChannelQueue;
};

}  // namespace aidl::google::hardware::power::impl::pixel
