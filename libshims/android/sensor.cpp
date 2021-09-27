#include <stdint.h>

namespace android {
    // libandroid.so
    extern "C" void ASensorManager_getInstanceForPackage() {}
    extern "C" void ASensorManager_getDefaultSensor() {}
    extern "C" void ASensorManager_createEventQueue() {}
    extern "C" void ASensorManager_destroyEventQueue() {}    
    extern "C" void ASensorEventQueue_enableSensor() {}
    extern "C" void ASensorEventQueue_disableSensor() {}
    extern "C" void ASensorEventQueue_setEventRate() {}
    extern "C" void ASensorEventQueue_getEvents() {}    
    extern "C" void ASensor_getName() {}
    extern "C" void ASensor_getVendor() {}
    extern "C" void ASensor_getMinDelay() {}
}
