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

#include <aidl/AdpfTypes.h>
#include <aidl/AppDescriptorTrace.h>
#include <aidl/AppHintDesc.h>
#include <aidl/PhysicalQuantityTypes.h>
#include <aidl/SessionMetrics.h>
#include <gmock/gmock.h>

namespace aidl::google::hardware::power::mock::pixel {

class MockPowerSessionManager {
  public:
    MockPowerSessionManager() = default;
    ~MockPowerSessionManager() = default;

    MOCK_METHOD(void, updateHintMode, (const std::string &mode, bool enabled), ());
    MOCK_METHOD(void, updateHintBoost, (const std::string &boost, int32_t durationMs), ());
    MOCK_METHOD(int, getDisplayRefreshRate, (), ());
    MOCK_METHOD(void, addPowerSession,
                (const std::string &idString,
                 const std::shared_ptr<impl::pixel::AppHintDesc> &sessionDescriptor,
                 const std::shared_ptr<impl::pixel::AppDescriptorTrace> &sessionTrace,
                 const std::vector<int32_t> &threadIds, const impl::pixel::ProcessTag procTag),
                ());
    MOCK_METHOD(void, removePowerSession,
                (int64_t sessionId, const impl::pixel::ProcessTag procTag), ());
    MOCK_METHOD(void, setThreadsFromPowerSession,
                (int64_t sessionId, const std::vector<int32_t> &threadIds,
                 const impl::pixel::ProcessTag procTag),
                ());
    MOCK_METHOD(void, pause, (int64_t sessionId), ());
    MOCK_METHOD(void, resume, (int64_t sessionId), ());
    MOCK_METHOD(void, updateUniversalBoostMode, (), ());
    MOCK_METHOD(void, dumpToFd, (int fd), ());
    MOCK_METHOD(void, updateTargetWorkDuration,
                (int64_t sessionId, impl::pixel::AdpfVoteType voteId,
                 std::chrono::nanoseconds durationNs),
                ());
    MOCK_METHOD(void, voteSet,
                (int64_t sessionId, impl::pixel::AdpfVoteType voteId, int uclampMin, int uclampMax,
                 std::chrono::steady_clock::time_point startTime,
                 std::chrono::nanoseconds durationNs),
                ());
    MOCK_METHOD(void, voteSet,
                (int64_t sessionId, impl::pixel::AdpfVoteType voteId, impl::pixel::Cycles capacity,
                 std::chrono::steady_clock::time_point startTime,
                 std::chrono::nanoseconds durationNs),
                ());

    MOCK_METHOD(void, disableBoosts, (int64_t sessionId), ());
    MOCK_METHOD(void, setPreferPowerEfficiency, (int64_t sessionId, bool enabled), ());
    MOCK_METHOD(std::optional<impl::pixel::Frequency>, gpuFrequency, (), (const));

    MOCK_METHOD(void, registerSession, (std::shared_ptr<void> session, int64_t sessionId), ());
    MOCK_METHOD(void, unregisterSession, (int64_t sessionId), ());
    MOCK_METHOD(void, clear, (), ());
    MOCK_METHOD(std::shared_ptr<void>, getSession, (int64_t sessionId), ());
    MOCK_METHOD(void, updateHboostStatistics,
                (int64_t sessionId, impl::pixel::SessionJankyLevel jankyLevel, int32_t numOfFrames),
                ());
    MOCK_METHOD(bool, getGameModeEnableState, (), ());
    MOCK_METHOD(void, updateFrameBuckets,
                (int64_t sessionId, const impl::pixel::FrameBuckets &lastReportedFrames), ());

    static testing::NiceMock<MockPowerSessionManager> *getInstance() {
        static testing::NiceMock<MockPowerSessionManager> instance{};
        return &instance;
    }
};

}  // namespace aidl::google::hardware::power::mock::pixel
