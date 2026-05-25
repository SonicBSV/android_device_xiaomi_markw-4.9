#! /vendor/bin/sh

LOGTAG="post_boot_markw"

write() { [ -e "$1" ] && echo "$2" > "$1"; }
write_str() { [ -e "$1" ] && printf '%s' "$2" > "$1"; }

target="$(getprop ro.board.platform)"
if [ "$target" != "msm8953" ]; then
    setprop vendor.post_boot.parsed 1
    exit 0
fi

soc_id=""
[ -f /sys/devices/soc0/soc_id ] && soc_id="$(cat /sys/devices/soc0/soc_id)"
[ -z "$soc_id" ] && [ -f /sys/devices/system/soc/soc0/id ] && soc_id="$(cat /sys/devices/system/soc/soc0/id)"

case "$soc_id" in
    "293"|"304"|"338"|"351") ;;
    *) setprop vendor.post_boot.parsed 1; exit 0 ;;
esac

log -t "$LOGTAG" -p i "Starting: soc=$soc_id kernel=$(uname -r)"

# Prepare
write /sys/module/msm_thermal/core_control/enabled 0

if [ -f /sys/devices/system/cpu/cpu0/core_ctl/enable ]; then
    write /sys/devices/system/cpu/cpu0/core_ctl/enable 0
else
    write /sys/devices/system/cpu/cpu0/core_ctl/disable 1
fi

for cpu in 1 2 3 4 5 6 7; do
    write /sys/devices/system/cpu/cpu${cpu}/online 1
done

# Scheduler
write /proc/sys/kernel/sched_boost 0
write /proc/sys/kernel/sched_ravg_window 20000000
write /proc/sys/kernel/sched_init_task_load 15
write /proc/sys/kernel/sched_prefer_sync_wakee_to_waker 1
write /proc/sys/kernel/power_aware_timer_migration 1
write /proc/sys/kernel/sched_upmigrate 95
write /proc/sys/kernel/sched_downmigrate 85
write /proc/sys/kernel/sched_spill_nr_run 3
write /proc/sys/kernel/sched_restrict_cluster_spill 1

# Only confirmed stune node
write /dev/stune/top-app/schedtune.boost 1

# Cpuset
write /dev/cpuset/top-app/cpus 0-7
write /dev/cpuset/foreground/cpus 0-7
write /dev/cpuset/background/cpus 0-7
write /dev/cpuset/system-background/cpus 0-7
write /dev/cpuset/restricted/cpus 0-7

# schedutil
write_str /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor "schedutil"

for node in \
    /sys/devices/system/cpu/cpufreq/schedutil/up_rate_limit_us \
    /sys/devices/system/cpu/cpu0/cpufreq/schedutil/up_rate_limit_us \
    /sys/devices/system/cpu/cpufreq/policy0/schedutil/up_rate_limit_us
do
    [ -e "$node" ] && echo 1000 > "$node" && break
done

for node in \
    /sys/devices/system/cpu/cpufreq/schedutil/down_rate_limit_us \
    /sys/devices/system/cpu/cpu0/cpufreq/schedutil/down_rate_limit_us \
    /sys/devices/system/cpu/cpufreq/policy0/schedutil/down_rate_limit_us
do
    [ -e "$node" ] && echo 20000 > "$node" && break
done

for node in \
    /sys/devices/system/cpu/cpufreq/schedutil/hispeed_freq \
    /sys/devices/system/cpu/cpu0/cpufreq/schedutil/hispeed_freq \
    /sys/devices/system/cpu/cpufreq/policy0/schedutil/hispeed_freq
do
    [ -e "$node" ] && echo 1248000 > "$node" && break
done

for node in \
    /sys/devices/system/cpu/cpufreq/schedutil/hispeed_load \
    /sys/devices/system/cpu/cpu0/cpufreq/schedutil/hispeed_load \
    /sys/devices/system/cpu/cpufreq/policy0/schedutil/hispeed_load
do
    [ -e "$node" ] && echo 90 > "$node" && break
done

write /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq 652800

# CPU boost
for node in \
    /sys/module/cpu_boost/parameters/input_boost_freq \
    /sys/devices/system/cpu/cpu_boost/input_boost_freq
do
    [ -e "$node" ] && printf '%s' "0:1094400" > "$node" && break
done

for node in \
    /sys/module/cpu_boost/parameters/input_boost_ms \
    /sys/devices/system/cpu/cpu_boost/input_boost_ms
do
    [ -e "$node" ] && echo 40 > "$node" && break
done

# Devfreq boost
write /sys/module/devfreq_boost/parameters/wake_boost_duration 250
write /sys/module/devfreq_boost/parameters/input_boost_duration 64

# Adreno Idler only
write /sys/module/adreno_idler/parameters/adreno_idler_active Y
write /sys/module/adreno_idler/parameters/adreno_idler_idleworkload 5000
write /sys/module/adreno_idler/parameters/adreno_idler_downdifferential 25
write /sys/module/adreno_idler/parameters/adreno_idler_idlewait 15

# Bus DCVS
write_str /sys/class/devfreq/soc:qcom,mincpubw/governor "cpufreq"

if [ -d /sys/class/devfreq/soc:qcom,cpubw ]; then
    write_str /sys/class/devfreq/soc:qcom,cpubw/governor "bw_hwmon"
    write /sys/class/devfreq/soc:qcom,cpubw/polling_interval 50
    write /sys/class/devfreq/soc:qcom,cpubw/min_freq 769
    write /sys/class/devfreq/soc:qcom,cpubw/bw_hwmon/io_percent 34
    write /sys/class/devfreq/soc:qcom,cpubw/bw_hwmon/sample_ms 4
    write /sys/class/devfreq/soc:qcom,cpubw/bw_hwmon/guard_band_mbps 0
    write /sys/class/devfreq/soc:qcom,cpubw/bw_hwmon/hist_memory 20
    write /sys/class/devfreq/soc:qcom,cpubw/bw_hwmon/hyst_length 10
    write /sys/class/devfreq/soc:qcom,cpubw/bw_hwmon/down_thres 80
    write /sys/class/devfreq/soc:qcom,cpubw/bw_hwmon/low_power_delay 20
    write /sys/class/devfreq/soc:qcom,cpubw/bw_hwmon/low_power_io_percent 34
    write /sys/class/devfreq/soc:qcom,cpubw/bw_hwmon/low_power_ceil_mbps 0
    write /sys/class/devfreq/soc:qcom,cpubw/bw_hwmon/up_scale 250
    write /sys/class/devfreq/soc:qcom,cpubw/bw_hwmon/idle_mbps 1600
fi

if [ -d /sys/class/devfreq/soc:qcom,gpubw ]; then
    write_str /sys/class/devfreq/soc:qcom,gpubw/governor "bw_hwmon"
    write /sys/class/devfreq/soc:qcom,gpubw/bw_hwmon/io_percent 40
fi

# Re-enable thermal
write /sys/module/msm_thermal/core_control/enabled 1

# LPM
write /sys/module/lpm_levels/parameters/sleep_disabled 0
write /sys/module/lpm_levels/lpm_workarounds/dynamic_clock_gating 1

# I/O
for queue in /sys/block/mmcblk0/queue /sys/block/mmcblk1/queue; do
    [ -d "$queue" ] || continue
    if grep -q maple "$queue/scheduler" 2>/dev/null; then
        write_str "$queue/scheduler" "maple"
    elif grep -q deadline "$queue/scheduler" 2>/dev/null; then
        write_str "$queue/scheduler" "deadline"
    fi
    write "$queue/read_ahead_kb" 128
    write "$queue/nr_requests" 128
    write "$queue/iostats" 0
    write "$queue/add_random" 0
    write "$queue/nomerges" 0
    write "$queue/rq_affinity" 1
    write "$queue/rotational" 0
done

for queue in /sys/block/dm-*/queue; do
    [ -d "$queue" ] || continue
    write "$queue/read_ahead_kb" 128
    write "$queue/nr_requests" 128
    write "$queue/iostats" 0
    write "$queue/add_random" 0
done

for node in /sys/block/mmcblk0/bdi/read_ahead_kb /sys/block/mmcblk0rpmb/bdi/read_ahead_kb; do
    [ -e "$node" ] && echo 128 > "$node"
done

# Legacy LMK off
write /sys/module/lowmemorykiller/parameters/enable_lmk 0
write /sys/module/lowmemorykiller/parameters/enable_adaptive_lmk 0
write /sys/module/process_reclaim/parameters/enable_process_reclaim 0
write /proc/sys/vm/reap_mem_on_sigkill 1
write /sys/module/lowmemorykiller/parameters/oom_reaper 1

write /proc/sys/kernel/panic 5
write /proc/sys/kernel/printk_devkmsg off
write /proc/sys/kernel/hung_task_timeout_secs 0

setprop vendor.post_boot.parsed 1

if [ -f /sys/devices/soc0/select_image ]; then
    write /sys/devices/soc0/select_image 10
    write_str /sys/devices/soc0/image_version "10:$(getprop ro.build.id):$(getprop ro.build.version.incremental)"
    write_str /sys/devices/soc0/image_variant "$(getprop ro.product.name)-$(getprop ro.build.type)"
    write_str /sys/devices/soc0/image_crm_version "$(getprop ro.build.version.codename)"
fi

[ "$(getprop persist.vendor.console.silent.config)" = "1" ] && write /proc/sys/kernel/printk 0

misc_link="$(ls -l /dev/block/bootdevice/by-name/misc 2>/dev/null)"
real_path="${misc_link##*>}"
[ -n "$real_path" ] && setprop persist.vendor.mmi.misc_dev_path "$real_path"

# Включаем Power HAL (запускаем NodeLooperThread)
setprop vendor.powerhal.init 1

# Сигнал о завершении парсинга пост-бута
setprop vendor.post_boot.parsed 1

log -t "$LOGTAG" -p i "post_boot complete, Power HAL initiated"
