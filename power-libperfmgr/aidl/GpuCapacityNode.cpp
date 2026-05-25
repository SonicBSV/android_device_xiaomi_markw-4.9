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

#define LOG_TAG "powerhal-libperfmgr"
#define ATRACE_TAG (ATRACE_TAG_POWER | ATRACE_TAG_HAL)

#include "GpuCapacityNode.h"

#include <android-base/logging.h>
#include <utils/Trace.h>

#include <charconv>

#include "perfmgr/HintManager.h"

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

GpuCapacityNode::~GpuCapacityNode() {
    fd_interface_->close(frequency_fd_);
    fd_interface_->close(capacity_headroom_fd_);
}

GpuCapacityNode::GpuCapacityNode(std::unique_ptr<FdInterface> fd_interface,
                                 int validated_capacity_headroom_fd, int validated_frequency_fd,
                                 std::string_view node_path)
    : fd_interface_(std::move(fd_interface)),
      capacity_node_path_(node_path),
      capacity_headroom_fd_(validated_capacity_headroom_fd),
      frequency_fd_(validated_frequency_fd) {
    if (capacity_headroom_fd_ < 0) {
        LOG(FATAL) << ("precondition violation for GpuCapacityNode: invalid capacity_headroom_fd_");
    }
    if (frequency_fd_ < 0) {
        LOG(FATAL) << ("precondition violation for GpuCapacityNode: invalid frequency_fd_");
    }
}

std::unique_ptr<GpuCapacityNode> GpuCapacityNode::init_gpu_capacity_node(
        std::unique_ptr<FdInterface> fd_interface, std::string_view gpu_node_dir) {
    static constexpr auto fd_flags_common = O_CLOEXEC | O_NONBLOCK;
    auto const capacity_headroom_file = std::string(gpu_node_dir) + "/capacity_headroom";
    auto const capacity_headroom_fd =
            fd_interface->open(capacity_headroom_file.c_str(), O_RDWR | fd_flags_common);
    if (capacity_headroom_fd < 0) {
        LOG(ERROR) << "could not open gpu capacity path: " << capacity_headroom_file << ": "
                   << strerror(errno);
        return nullptr;
    }
    auto const gpu_freq_file = std::string(gpu_node_dir) + "/cur_freq";
    auto const frequency_fd = fd_interface->open(gpu_freq_file.c_str(), O_RDONLY | fd_flags_common);
    if (frequency_fd < 0) {
        fd_interface->close(capacity_headroom_fd);
        LOG(ERROR) << "could not open gpu capacity path: " << gpu_freq_file << ": "
                   << strerror(errno);
        return nullptr;
    }
    return std::make_unique<GpuCapacityNode>(std::move(fd_interface), capacity_headroom_fd,
                                             frequency_fd, gpu_node_dir);
}

bool GpuCapacityNode::set_gpu_capacity(Cycles capacity) const {
    std::lock_guard lk(capacity_mutex_);
    auto const capacity_str = std::to_string(static_cast<int>(capacity));
    ATRACE_INT("gpuCapacitySet", static_cast<int>(static_cast<int>(capacity)));
    auto const rc =
            fd_interface_->write(capacity_headroom_fd_, capacity_str.c_str(), capacity_str.size());
    if (!rc) {
        LOG(ERROR) << "could not write to capacity node: " << capacity_node_path_ << ": "
                   << strerror(errno);
    }
    return !rc;
}

std::optional<Frequency> GpuCapacityNode::gpu_frequency() const {
    std::lock_guard lk(freq_mutex_);
    std::array<char, 16> buffer;
    buffer.fill('\0');

    ssize_t bytes_read_total = 0;
    size_t const effective_size = buffer.size() - 1;
    do {
        auto const bytes_read = fd_interface_->read(frequency_fd_, buffer.data() + bytes_read_total,
                                                    effective_size - bytes_read_total);
        if (bytes_read == 0) {
            break;
        }

        if (bytes_read < 0) {
            LOG(ERROR) << "could not read gpu frequency:" << bytes_read << ": " << strerror(errno);
            return {};
        }
        bytes_read_total += bytes_read;
    } while (bytes_read_total < effective_size);

    auto const rc = fd_interface_->lseek(frequency_fd_, 0, SEEK_SET);
    if (rc < 0) {
        LOG(ERROR) << "could not seek gpu frequency file: " << strerror(errno);
        return {};
    }

    int frequency_raw = 0;
    auto [ptr, ec] = std::from_chars(buffer.data(), buffer.data() + buffer.size(), frequency_raw);
    if (ec != std::errc() || frequency_raw <= 0) {
        LOG(ERROR) << "could not parse gpu frequency" << buffer.data();
        return {};
    }

    auto const frequency = Frequency(frequency_raw * 1000);
    ATRACE_INT("gpuRecordedFreq", static_cast<int>(frequency));
    return frequency;
}

std::optional<std::unique_ptr<GpuCapacityNode>> createGpuCapacityNode() {
    auto const path = ::android::perfmgr::HintManager::GetInstance()->gpu_sysfs_config_path();
    if (!path) {
        return {};
    }
    return {GpuCapacityNode::init_gpu_capacity_node(std::make_unique<FdWriter>(), *path)};
}

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
