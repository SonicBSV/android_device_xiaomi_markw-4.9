#include <stdint.h>

namespace android {
    // libandroid.so
    extern "C" void ALooper_forThread() {}
    extern "C" void ALooper_prepare() {}
    extern "C" void ALooper_pollOnce() {}
    extern "C" void ALooper_wake() {}
}
