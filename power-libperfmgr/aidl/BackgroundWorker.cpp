/*
 * Copyright 2023 The Android Open Source Project
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

#include "BackgroundWorker.h"

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

PriorityQueueWorkerPool::PriorityQueueWorkerPool(size_t threadCount,
                                                 const std::string &threadNamePrefix) {
    mRunning = true;
    mThreadPool.reserve(threadCount);
    for (size_t threadId = 0; threadId < threadCount; ++threadId) {
        mThreadPool.push_back(std::thread([&, tid = threadId]() { loop(); }));

        if (!threadNamePrefix.empty()) {
            const std::string fullThreadName = threadNamePrefix + std::to_string(threadId);
            pthread_setname_np(mThreadPool.back().native_handle(), fullThreadName.c_str());
        }
    }
}

PriorityQueueWorkerPool::~PriorityQueueWorkerPool() {
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mRunning = false;
        mCv.notify_all();
    }
    for (auto &t : mThreadPool) {
        if (t.joinable()) {
            t.join();
        }
    }
}

void PriorityQueueWorkerPool::addCallback(int64_t templateQueueWorkerId,
                                          std::function<void(int64_t)> callback) {
    if (!callback) {
        // Don't add callback if it isn't callable to prevent having to check later
        return;
    }
    std::unique_lock<std::shared_mutex> lock(mSharedMutex);
    auto itr = mCallbackMap.find(templateQueueWorkerId);
    if (itr != mCallbackMap.end()) {
        return;
    }
    mCallbackMap[templateQueueWorkerId] = callback;
}

void PriorityQueueWorkerPool::removeCallback(int64_t templateQueueWorkerId) {
    std::unique_lock<std::shared_mutex> lock(mSharedMutex);
    auto itr = mCallbackMap.find(templateQueueWorkerId);
    if (itr == mCallbackMap.end()) {
        return;
    }
    mCallbackMap.erase(itr);
}

void PriorityQueueWorkerPool::schedule(int64_t templateQueueWorkerId, int64_t packageId,
                                       std::chrono::steady_clock::time_point deadline) {
    std::unique_lock<std::mutex> lock(mMutex);
    mPackageQueue.emplace(deadline, templateQueueWorkerId, packageId);
    mCv.notify_all();
}

void PriorityQueueWorkerPool::loop() {
    Package package;
    while (mRunning) {
        std::unique_lock<std::mutex> lock(mMutex);
        // Default to longest wait possible without overflowing if there is
        // nothing to work on in the queue
        std::chrono::steady_clock::time_point deadline =
                std::chrono::steady_clock::time_point::max();

        // Use next item to work on deadline if available
        if (!mPackageQueue.empty()) {
            deadline = mPackageQueue.top().deadline;
        }

        // Wait until signal or deadline
        mCv.wait_until(lock, deadline, [&]() {
            // Check if stop running requested, if so return now
            if (!mRunning)
                return true;

            // Check if nothing in queue (e.g. spurious wakeup), wait as long as possible again
            if (mPackageQueue.empty()) {
                deadline = std::chrono::steady_clock::time_point::max();
                return false;
            }

            // Something in queue, use that as next deadline
            deadline = mPackageQueue.top().deadline;
            // Check if deadline is in the future still, continue waiting with updated deadline
            if (deadline > std::chrono::steady_clock::now())
                return false;
            // Next work entry's deadline is in the past or exactly now, time to work on it
            return true;
        });

        if (!mRunning)
            break;
        if (mPackageQueue.empty())
            continue;

        // Copy work entry from queue and unlock
        package = mPackageQueue.top();
        mPackageQueue.pop();
        lock.unlock();

        // Find callback based on package's callback id
        {
            std::shared_lock<std::shared_mutex> lockCb(mSharedMutex);
            auto callbackItr = mCallbackMap.find(package.templateQueueWorkerId);
            if (callbackItr == mCallbackMap.end()) {
                // Callback was removed before package could be worked on, that's ok just ignore
                continue;
            }
            // Exceptions disabled so no need to wrap this
            callbackItr->second(package.packageId);
        }
    }
}

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
