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

#pragma once

#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>

#include "AdpfTypes.h"

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

// Background thread processing from priority queue based on time deadline
// This class isn't meant to be used directly, use TemplatePriorityQueueWorker below
class PriorityQueueWorkerPool {
  public:
    // CTOR
    // thread count is number of threads to create in thread pool
    // thread name prefix is use for naming threads to help with debugging
    PriorityQueueWorkerPool(size_t threadCount, const std::string &threadNamePrefix);
    // DTOR
    ~PriorityQueueWorkerPool();
    // Map callback id to callback function
    void addCallback(int64_t templateQueueWorkerId, std::function<void(int64_t)> callback);
    // Unmap callback id with callback function
    void removeCallback(int64_t templateQueueWorkerId);
    // Schedule work for specific worker id with package id to be run at time deadline
    void schedule(int64_t templateQueueWorkerId, int64_t packageId,
                  std::chrono::steady_clock::time_point deadline);

  private:
    // Thread coordination
    std::mutex mMutex;
    bool mRunning;
    std::condition_variable mCv;
    std::vector<std::thread> mThreadPool;
    void loop();

    // Work package with worker id to find correct callback in
    struct Package {
        Package() {}
        Package(std::chrono::steady_clock::time_point pDeadline, int64_t pTemplateQueueWorkerId,
                int64_t pPackageId)
            : deadline(pDeadline),
              templateQueueWorkerId(pTemplateQueueWorkerId),
              packageId(pPackageId) {}
        std::chrono::steady_clock::time_point deadline;
        int64_t templateQueueWorkerId{0};
        int64_t packageId{0};
        // Sort time earliest first
        bool operator<(const Package &p) const { return deadline > p.deadline; }
    };
    std::priority_queue<Package> mPackageQueue;

    // Callback management
    std::shared_mutex mSharedMutex;
    std::unordered_map<int64_t, std::function<void(int64_t)>> mCallbackMap;
};

// Generic templated worker for registering a single std::function callback one time
// and reusing it to reduce memory allocations. Many TemplatePriorityQueueWorkers
// can make use of the same PriorityQueue worker which enables sharing a thread pool
// across callbacks of different types. This class is a template to allow for different
// types of work packages while not requiring virtual calls.
template <typename PACKAGE>
class TemplatePriorityQueueWorker {
  public:
    // CTOR, callback to run when added work is run, worker to use for adding work to
    TemplatePriorityQueueWorker(std::function<void(const PACKAGE &)> cb,
                                std::shared_ptr<PriorityQueueWorkerPool> worker)
        : mCallbackId(reinterpret_cast<std::intptr_t>(this)), mCallback(cb), mWorker(worker) {
        if (!mCallback) {
            mCallback = [](const auto &) {};
        }
        mWorker->addCallback(mCallbackId, [&](int64_t packageId) { process(packageId); });
    }

    // DTOR
    ~TemplatePriorityQueueWorker() { mWorker->removeCallback(mCallbackId); }

    void schedule(const PACKAGE &package,
                  std::chrono::steady_clock::time_point t = std::chrono::steady_clock::now()) {
        {
            std::lock_guard<std::mutex> lock(mMutex);
            ++mPackageIdCounter;
            mPackages.emplace(mPackageIdCounter, package);
        }
        mWorker->schedule(mCallbackId, mPackageIdCounter, t);
    }

  private:
    int64_t mCallbackId{0};
    std::function<void(const PACKAGE &)> mCallback;
    // Must ensure PriorityQueueWorker does not go out of scope before this class does
    std::shared_ptr<PriorityQueueWorkerPool> mWorker;
    mutable std::mutex mMutex;
    // Counter is used as a unique identifier for work packages
    int64_t mPackageIdCounter{0};
    // Want a container that is:
    // fast to add, fast random find find, fast random removal,
    // and with reasonable space efficiency
    std::unordered_map<int64_t, PACKAGE> mPackages;

    void process(int64_t packageId) {
        PACKAGE package;
        {
            std::lock_guard<std::mutex> lock(mMutex);
            auto itr = mPackages.find(packageId);
            if (itr == mPackages.end()) {
                // Work id does not have matching entry, drop it
                return;
            }

            package = itr->second;
            mPackages.erase(itr);
        }
        mCallback(package);
    }
};

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
