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

#include <fcntl.h>
#include <unistd.h>

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "PhysicalQuantityTypes.h"

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

struct FdInterface {
    virtual int open(const char *, int) const = 0;
    virtual int write(int, const char *, size_t) const = 0;
    virtual ssize_t read(int, void *, size_t) const = 0;
    virtual off_t lseek(int, off_t, int) const = 0;
    virtual int close(int) const = 0;

    FdInterface() = default;
    virtual ~FdInterface() = default;
    FdInterface(FdInterface const &) = delete;
    FdInterface &operator=(FdInterface const &) = delete;
};

struct FdWriter : FdInterface {
    int open(const char *path, int flags) const override { return ::open(path, flags); }
    int write(int fd, const char *data, size_t len) const override {
        return ::write(fd, data, len);
    }
    ssize_t read(int fd, void *data, size_t len) const final { return ::read(fd, data, len); }
    off_t lseek(int fd, off_t offset, int whence) const final {
        return ::lseek(fd, offset, whence);
    }
    int close(int fd) const override { return ::close(fd); }
};

struct GpuCapacityNode final {
    // Exceptions should really be allowed, use exploded constructor pattern and provide
    // helper construction function.
    GpuCapacityNode(std::unique_ptr<FdInterface> fd_interface, int validated_capacity_headroom_fd,
                    int validated_frequency_fd, std::string_view gpu_node_dir);
    static std::unique_ptr<GpuCapacityNode> init_gpu_capacity_node(
            std::unique_ptr<FdInterface> fd_interface, std::string_view gpu_node_dir);

    ~GpuCapacityNode() noexcept;

    bool set_gpu_capacity(Cycles capacity) const;
    std::optional<Frequency> gpu_frequency() const;

  private:
    std::unique_ptr<FdInterface> const fd_interface_;
    std::string const capacity_node_path_;
    int const capacity_headroom_fd_;
    int const frequency_fd_;
    std::mutex mutable freq_mutex_;
    std::mutex mutable capacity_mutex_;
};

// There's not a global object factory or context in PowerHal, maybe introducing one would simplify
// resource management.
std::optional<std::unique_ptr<GpuCapacityNode>> createGpuCapacityNode();

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
