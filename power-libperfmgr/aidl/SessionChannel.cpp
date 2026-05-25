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

#include "SessionChannel.h"

#include "AdpfTypes.h"

namespace aidl::google::hardware::power::impl::pixel {

SessionChannel::SessionChannel(int32_t tgid, int32_t uid, int64_t id, int32_t offset)
    : mTgid(tgid),
      mUid(uid),
      mId(id),
      mReadMask(1u << (offset + 16)),
      mWriteMask(1u << offset),
      mChannelQueue(kFMQQueueSize, true) {}

void SessionChannel::getDesc(ChannelQueueDesc *_return_desc) {
    *_return_desc = mChannelQueue.dupeDesc();
}

bool SessionChannel::isValid() const {
    return mChannelQueue.isValid();
}

ChannelQueue *SessionChannel::getQueue() {
    return &mChannelQueue;
}

int32_t SessionChannel::getTgid() const {
    return mTgid;
}

int32_t SessionChannel::getUid() const {
    return mUid;
}

int64_t SessionChannel::getId() const {
    return mId;
}

uint32_t SessionChannel::getReadBitmask() const {
    return mReadMask;
}

uint32_t SessionChannel::getWriteBitmask() const {
    return mWriteMask;
}

}  // namespace aidl::google::hardware::power::impl::pixel
