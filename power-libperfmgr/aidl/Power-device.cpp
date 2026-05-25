#include "Power.h"
#include <android-base/file.h>
#include <android-base/logging.h>

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

constexpr char kTapToWakeNode[] = "/proc/gesture/onoff";

bool isDeviceSpecificModeSupported(Mode type, bool* _aidl_return) {
    if (type == Mode::DOUBLE_TAP_TO_WAKE) {
        *_aidl_return = true;
        return true;
    }
    return false;
}

bool setDeviceSpecificMode(Mode type, bool enabled) {
    if (type == Mode::DOUBLE_TAP_TO_WAKE) {
        ::android::base::WriteStringToFile(enabled ? "1" : "0", kTapToWakeNode);
        return true;
    }
    return false;
}

} // namespace pixel
} // namespace impl
} // namespace power
} // namespace hardware
} // namespace google
} // namespace aidl
