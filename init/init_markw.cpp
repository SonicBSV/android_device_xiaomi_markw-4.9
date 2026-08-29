/*
   Copyright (c) 2016, The CyanogenMod Project

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

#include <string.h>
#include <fstream>

#include "vendor_init.h"
#include "property_service.h"
#include "log/log.h"

#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include <sys/_system_properties.h>

namespace {

static void property_override(const char prop[], const char value[], bool add = true) {
    auto pi = (prop_info*)__system_property_find(prop);
    if (pi != nullptr) {
        __system_property_update(pi, value, strlen(value));
    } else if (add) {
        __system_property_add(prop, strlen(prop), value, strlen(value));
    }
}

static bool is_psi_available() {
    std::ifstream psi_file("/proc/pressure/memory");
    return psi_file.good();
}

static void load_dalvik_properties() {
    /*
     * markw: 3GB RAM / 32GB eMMC / 1080p
     * Kernel 4.9, crDroid 11.x (Android 15)
     * Ужатые лимиты кучи для LPDDR3 + eMMC
     */
    property_override("dalvik.vm.heapstartsize", "8m");
    property_override("dalvik.vm.heapgrowthlimit", "192m");
    property_override("dalvik.vm.heapsize", "384m");
    property_override("dalvik.vm.heaptargetutilization", "0.75");
    property_override("dalvik.vm.heapminfree", "2m");
    property_override("dalvik.vm.heapmaxfree", "8m");

    property_override("dalvik.vm.dex2oat-threads", "4");
    property_override("dalvik.vm.dex2oat64.enabled", "true");
    property_override("dalvik.vm.usejit", "true");
    property_override("dalvik.vm.usejitprofiles", "true");
    property_override("dalvik.vm.dex2oat-minidebuginfo", "false");
    property_override("dalvik.vm.minidebuginfo", "false");
}

static void load_lmk_properties() {
    /*
     * LMKD для 3GB LPDDR3 + eMMC
     * Ядро 4.9: CONFIG_PSI=y → PSI-based LMKD
     */
    bool psi_enabled = is_psi_available();

    if (psi_enabled) {
        property_override("ro.lmk.use_psi", "true");
        property_override("ro.lmk.use_minfree_levels", "false");
        ALOGI("LMKD: PSI enabled (kernel 4.9)");
    } else {
        property_override("ro.lmk.use_psi", "false");
        property_override("ro.lmk.use_minfree_levels", "true");
        ALOGW("LMKD: PSI not available, fallback to minfree");
    }

    property_override("ro.lmk.psi_partial_stall_ms", "70");
    property_override("ro.lmk.psi_complete_stall_ms", "500");
    property_override("ro.lmk.thrashing_limit", "30");
    property_override("ro.lmk.thrashing_limit_decay", "10");
    property_override("ro.lmk.swap_util_max", "100");
    property_override("ro.lmk.swap_free_low_percentage", "10");
    property_override("ro.lmk.kill_heaviest_task", "true");
    property_override("ro.lmk.kill_timeout_ms", "50");
    property_override("ro.lmk.upgrade_pressure", "100");
    property_override("ro.lmk.downgrade_pressure", "30");
    property_override("ro.lmk.filecache_min_kb", "51200");
    property_override("ro.lmk.critical_upgrade", "false");
    property_override("ro.lmk.stall_limit_critical", "50");

    property_override("ro.lmk.low_ram", "true");
    property_override("ro.config.low_ram", "true");

    property_override("ro.lmk.debug", "false");
    property_override("ro.lmk.log_stats", "false");
}

static void load_zram_properties() {
    /*
     * ZRAM для 3GB + eMMC, ядро 4.9
     * lz4 — единственный разумный выбор на A53 (zstd слишком тяжёл)
     * В 4.9 НЕТ zram writeback, поэтому writeback-свойства не задаём.
     */
    property_override("vendor.zram.size", "1610612736");
    property_override("vendor.zram.streams", "4");
    property_override("vendor.zram.swappiness", "100");
    property_override("vendor.zram.comp_algorithm", "lz4");
    property_override("vendor.zram.enabled", "true");
}

static void load_performance_properties() {
    property_override("debug.sf.latch_unsignaled", "0");
    property_override("debug.sf.disable_backpressure", "0");
    property_override("debug.sf.enable_gl_backpressure", "1");
    property_override("debug.hwui.renderer", "skiagl");
    property_override("renderthread.skia.reduceopstasksplitting", "true");
    property_override("persist.traced.enable", "0");
}

}  // namespace

void vendor_load_properties() {
    ALOGI("Loading markw vendor properties (3GB/32GB, kernel 4.9, crDroid 15)");

    load_dalvik_properties();
    load_lmk_properties();
    load_zram_properties();
    load_performance_properties();

    ALOGI("markw: vendor properties loaded");
}
