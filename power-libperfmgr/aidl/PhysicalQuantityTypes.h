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

#include <chrono>
#include <compare>
#include <numeric>
#include <ostream>
#include <type_traits>

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

template <class C>
struct is_chrono_duration : std::false_type {};

template <class Rep, class Per>
struct is_chrono_duration<std::chrono::duration<Rep, Per>> : std::true_type {};

template <typename T>
concept chrono_duration = is_chrono_duration<T>::value;

template <typename T>
concept arithmetic = std::is_arithmetic<T>::value;

template <arithmetic T, typename W>
struct PhysicalQuantityType final {
    constexpr PhysicalQuantityType() noexcept = default;
    constexpr explicit PhysicalQuantityType(T const &value) noexcept : value_(value) {}
    constexpr PhysicalQuantityType(PhysicalQuantityType const &) noexcept = default;
    constexpr PhysicalQuantityType &operator=(PhysicalQuantityType const &) noexcept = default;
    explicit constexpr inline operator T() const { return value_; }

    inline constexpr auto operator+(PhysicalQuantityType const &other) const {
        return PhysicalQuantityType(value_ + other.value_);
    }
    inline constexpr auto operator-(PhysicalQuantityType const &other) const {
        return PhysicalQuantityType(value_ - other.value_);
    }
    template <arithmetic C>
    inline constexpr auto operator*(C const &other) const {
        return PhysicalQuantityType(value_ * other);
    }
    template <arithmetic C>
    inline constexpr auto operator/(C const &other) const {
        return PhysicalQuantityType(value_ / other);
    }
    inline constexpr bool operator==(PhysicalQuantityType const &other) const {
        return value_ == other.value_;
    }
    inline constexpr auto operator<=>(PhysicalQuantityType const &other) const {
        return value_ <=> other.value_;
    }

  private:
    T value_;
};

template <arithmetic C, arithmetic T, typename W>
inline auto operator*(C const &other, PhysicalQuantityType<T, W> type) {
    return type * other;
}

using Cycles = PhysicalQuantityType<int, struct CyclesTag>;
using Frequency = PhysicalQuantityType<int, struct FrequencyTag>;

inline std::ostream &operator<<(std::ostream &os, Cycles cycles) {
    return os << "Cycles(" << int(cycles) << ")";
};

inline std::ostream &operator<<(std::ostream &os, Frequency freq) {
    return os << "Frequency(" << int(freq) << ")";
};

inline constexpr Frequency operator/(Cycles const &cycles, chrono_duration auto time) {
    auto const fpchrono = std::chrono::duration<float, std::chrono::seconds::period>(time);
    return Frequency(static_cast<int>(cycles) / fpchrono.count());
}

constexpr Frequency operator""_hz(unsigned long long hz) {
    return Frequency(hz);
}

inline constexpr Cycles operator*(Frequency const &freq, chrono_duration auto time) {
    auto const fpchrono = std::chrono::duration<float, std::chrono::seconds::period>(time);
    return Cycles(static_cast<int>(freq) * fpchrono.count());
}

inline constexpr Cycles operator*(chrono_duration auto time, Frequency const &freq) {
    return freq * time;
}

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
