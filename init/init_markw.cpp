/*
   Copyright (c) 2016, The CyanogenMod Project
             (c) 2018, The LineageOS Project
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of The Linux Foundation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.
   THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
   ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
   BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
   BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
   OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
   IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <fcntl.h>
#include <stdlib.h>
#include <sys/sysinfo.h>
#include <android-base/file.h>
#include <android-base/properties.h>
#include <android-base/strings.h>
#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include <sys/_system_properties.h>

#include "vendor_init.h"
#include "property_service.h"

char const *heapstartsize;
char const *heapgrowthlimit;
char const *heapsize;
char const *heapminfree;
char const *heapmaxfree;
char const *texture_cache_size;
char const *layer_cache_size;
char const *shape_cache_size;
char const *gradient_cache_size;
char const *drop_shadow_cache_size;

using android::base::GetProperty;
using android::base::ReadFileToString;
using android::init::property_set;

void property_override(char const prop[], char const value[])
{
    prop_info *pi;

    pi = (prop_info*) __system_property_find(prop);
    if (pi)
        __system_property_update(pi, value, strlen(value));
    else
        __system_property_add(prop, strlen(prop), value, strlen(value));
}

void check_ram()
{
    struct sysinfo sys;

    sysinfo(&sys);

    if (sys.totalram > 3072ull * 1024 * 1024) {
        // original values in file framework/native: phone-xxhdpi-4096-dalvik-heap.mk
        // 4GB - from vince 7.1 values
        heapstartsize = "16m";
        heapgrowthlimit = "192m";
        heapsize = "512m";
        heapminfree = "4m";
        heapmaxfree = "8m";
        texture_cache_size="88"; //increased texture cachce size
        layer_cache_size="58"; //increased layer cachce size
        shape_cache_size="4";
        gradient_cache_size="1";
        drop_shadow_cache_size="6";
    } else if (sys.totalram > 2048ull * 1024 * 1024) {
        // original values in file framework/native: phone-xxhdpi-3072-dalvik-heap.mk
        // 3GB - from markw 6.0 values
        heapstartsize = "16m";
        heapgrowthlimit = "192m";
        heapsize = "512m";
        heapminfree = "8m";
        heapmaxfree = "32m";
        texture_cache_size="88"; //increased texture cachce size
        layer_cache_size="58"; //increased layer cachce size
        shape_cache_size="4";
        gradient_cache_size="1";
        drop_shadow_cache_size="6";
    } else {
        // from - phone-xxhdpi-2048-dalvik-heap.mk
        heapstartsize = "16m";
        heapgrowthlimit = "192m";
        heapsize = "512m";
        heapminfree = "2m";
        heapmaxfree = "8m";
        texture_cache_size="72";
        layer_cache_size="48";
        shape_cache_size="4";
        gradient_cache_size="1";
        drop_shadow_cache_size="6";
   }
}

void gsi_check()
{
    std::string product;

    product = GetProperty("ro.product.device", "");

    // override device specific props for GSI
    if ((product == "phhgsi_arm64_a")) {
        property_override("ro.build.tags", "release-keys");
        property_override("ro.product.model", "Redmi 4");
        property_override("ro.product.brand", "Xiaomi");
        property_override("ro.product.name", "markw");
        property_override("ro.product.device", "markw");
        property_override("ro.product.manufacturer", "Xiaomi");
        property_override("ro.build.product", "markw");
        property_override("ro.build.description", "markw-user 6.0.1 MMB29M V10.2.1.0.MBEMIXM release-keys");
        property_override("ro.build.fingerprint", "Xiaomi/markw/markw:6.0.1/MMB29M/V10.2.1.0.MBEMIXM:user/release-keys");
    }
}

void vendor_load_properties()
{
    check_ram();
    gsi_check();

    property_set("dalvik.vm.heapstartsize", heapstartsize);
    property_set("dalvik.vm.heapgrowthlimit", heapgrowthlimit);
    property_set("dalvik.vm.heapsize", heapsize);
    property_set("dalvik.vm.heaptargetutilization", "0.75");
    property_set("dalvik.vm.heapminfree", heapminfree);
    property_set("dalvik.vm.heapmaxfree", heapmaxfree);
}
