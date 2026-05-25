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

#include <gmock/gmock.h>
#include <perfmgr/AdpfConfig.h>
#include <perfmgr/HintManager.h>

namespace aidl::google::hardware::power::mock::pixel {

class MockHintManager {
  public:
    MockHintManager() = default;
    ~MockHintManager() = default;

    MOCK_METHOD(bool, IsRunning, (), (const));
    MOCK_METHOD(bool, DoHint, (const std::string &hint_type), ());
    MOCK_METHOD(bool, DoHint,
                (const std::string &hint_type, std::chrono::milliseconds timeout_ms_override), ());
    MOCK_METHOD(bool, EndHint, (const std::string &hint_type), ());
    MOCK_METHOD(bool, IsHintSupported, (const std::string &hint_type), (const));
    MOCK_METHOD(bool, IsHintEnabled, (const std::string &hint_type), (const));
    MOCK_METHOD(bool, SetAdpfProfile, (const std::string &profile_name), ());
    MOCK_METHOD(std::shared_ptr<::android::perfmgr::AdpfConfig>, GetAdpfProfile, (), (const));
    MOCK_METHOD(bool, IsAdpfProfileSupported, (const std::string &name), (const));
    MOCK_METHOD(std::vector<std::string>, GetHints, (), (const));
    MOCK_METHOD(::android::perfmgr::HintStats, GetHintStats, (const std::string &hint_type),
                (const));
    MOCK_METHOD(void, DumpToFd, (int fd), ());
    MOCK_METHOD(bool, Start, (), ());
    MOCK_METHOD(bool, SetAdpfProfileFromDoHint, (const std::string &profile_name), ());
    MOCK_METHOD(std::shared_ptr<::android::perfmgr::AdpfConfig>, GetAdpfProfileFromDoHint, (),
                (const));

    static testing::NiceMock<MockHintManager> *GetInstance() {
        static testing::NiceMock<MockHintManager> instance{};
        return &instance;
    }
};

}  // namespace aidl::google::hardware::power::mock::pixel
