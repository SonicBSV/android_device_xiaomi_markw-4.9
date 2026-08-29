#!/vendor/bin/sh

# ═══════════════════════════════════════════════════════════════════════════
# init.qcom.post_boot.sh for MSM8953 (markw, 3GB/32GB eMMC)
# crDroid 11.x (Android 15) | Kernel 4.9 (EAS + WALT + SchedTune v1)
# ═══════════════════════════════════════════════════════════════════════════

LOGTAG="post_boot_markw"

write()     { [ -e "$1" ] && echo "$2" > "$1" 2>/dev/null; }
write_str() { [ -e "$1" ] && printf '%s' "$2" > "$1" 2>/dev/null; }

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

# ═══════════════════════════════════════════════════════════════════════════
# 1. ПОДГОТОВКА — отключаем термоконтроль и core_ctl на время настройки
# ═══════════════════════════════════════════════════════════════════════════
write /sys/module/msm_thermal/core_control/enabled 0

if [ -f /sys/devices/system/cpu/cpu0/core_ctl/enable ]; then
    write /sys/devices/system/cpu/cpu0/core_ctl/enable 0
else
    write /sys/devices/system/cpu/cpu0/core_ctl/disable 1
fi

for cpu in 1 2 3 4 5 6 7; do
    write /sys/devices/system/cpu/cpu${cpu}/online 1
done

# ═══════════════════════════════════════════════════════════════════════════
# 2. ПЛАНИРОВЩИК EAS/WALT (ядро 4.9)
#    Все 8 ядер — Cortex-A53, разделение на кластеры 0-3 / 4-7 логическое.
#    WALT отслеживает нагрузку по окнам, EAS принимает решения о размещении.
# ═══════════════════════════════════════════════════════════════════════════
write /proc/sys/kernel/sched_boost 0
write /proc/sys/kernel/sched_ravg_window 20000000
write /proc/sys/kernel/sched_init_task_load 15
write /proc/sys/kernel/sched_prefer_sync_wakee_to_waker 1
write /proc/sys/kernel/power_aware_timer_migration 1

# Пороги миграции WALT.
# Снижены для более быстрого переброса UI-потоков на свободные ядра.
write /proc/sys/kernel/sched_upmigrate 80
write /proc/sys/kernel/sched_downmigrate 70
write /proc/sys/kernel/sched_spill_nr_run 3
write /proc/sys/kernel/sched_restrict_cluster_spill 1

# Вращение тяжёлых задач между кластерами для равномерного нагрева
write /proc/sys/kernel/sched_walt_rotate_big_tasks 1

# ═══════════════════════════════════════════════════════════════════════════
# 3. SCHEDTUNE v1 (cgroups v1, ядро 4.9)
#    В 4.9 НЕТ uclamp! Используем schedtune через /dev/stune/.
# ═══════════════════════════════════════════════════════════════════════════
write /dev/stune/top-app/schedtune.boost 1
[ -e /dev/stune/top-app/schedtune.prefer_idle ] && \
    write /dev/stune/top-app/schedtune.prefer_idle 1
[ -e /dev/stune/foreground/schedtune.prefer_idle ] && \
    write /dev/stune/foreground/schedtune.prefer_idle 1

# ═══════════════════════════════════════════════════════════════════════════
# 4. CPUSETS (cgroups v1)
#    Фон ограничиваем кластером 0-3, UI получает все 0-7.
# ═══════════════════════════════════════════════════════════════════════════
write /dev/cpuset/top-app/cpus 0-7
write /dev/cpuset/foreground/cpus 0-7
write /dev/cpuset/background/cpus 0-3
write /dev/cpuset/system-background/cpus 0-3
write /dev/cpuset/restricted/cpus 0-3

# ═══════════════════════════════════════════════════════════════════════════
# 5. SCHEDUTIL GOVERNOR
# ═══════════════════════════════════════════════════════════════════════════
write_str /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor "schedutil"

# Быстрый подъём частоты (500us вместо 1000us)
for node in \
    /sys/devices/system/cpu/cpufreq/policy0/schedutil/up_rate_limit_us \
    /sys/devices/system/cpu/cpu0/cpufreq/schedutil/up_rate_limit_us \
    /sys/devices/system/cpu/cpufreq/schedutil/up_rate_limit_us
do
    [ -e "$node" ] && echo 500 > "$node" && break
done

# Медленный спуск (20ms)
for node in \
    /sys/devices/system/cpu/cpufreq/policy0/schedutil/down_rate_limit_us \
    /sys/devices/system/cpu/cpu0/cpufreq/schedutil/down_rate_limit_us \
    /sys/devices/system/cpu/cpufreq/schedutil/down_rate_limit_us
do
    [ -e "$node" ] && echo 20000 > "$node" && break
done

# Опорная частота 1401 МГц при нагрузке 85%
for node in \
    /sys/devices/system/cpu/cpufreq/policy0/schedutil/hispeed_freq \
    /sys/devices/system/cpu/cpu0/cpufreq/schedutil/hispeed_freq \
    /sys/devices/system/cpu/cpufreq/schedutil/hispeed_freq
do
    [ -e "$node" ] && echo 1401600 > "$node" && break
done

for node in \
    /sys/devices/system/cpu/cpufreq/policy0/schedutil/hispeed_load \
    /sys/devices/system/cpu/cpu0/cpufreq/schedutil/hispeed_load \
    /sys/devices/system/cpu/cpufreq/policy0/schedutil/hispeed_load
do
    [ -e "$node" ] && echo 85 > "$node" && break
done

# Минимальная частота — 652 МГц для отзывчивости
write /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq 652800

# ═══════════════════════════════════════════════════════════════════════════
# 6. CPU BOOST (CONFIG_CPU_BOOST=y в ядре 4.9)
# ═══════════════════════════════════════════════════════════════════════════
for node in \
    /sys/module/cpu_boost/parameters/input_boost_freq \
    /sys/devices/system/cpu/cpu_boost/input_boost_freq
do
    [ -e "$node" ] && printf '%s' "0:1401600 4:1401600" > "$node" && break
done

for node in \
    /sys/module/cpu_boost/parameters/input_boost_ms \
    /sys/devices/system/cpu/cpu_boost/input_boost_ms
do
    [ -e "$node" ] && echo 40 > "$node" && break
done

[ -e /sys/module/cpu_boost/parameters/input_boost_enabled ] && \
    echo Y > /sys/module/cpu_boost/parameters/input_boost_enabled

# ═══════════════════════════════════════════════════════════════════════════
# 7. DEVFREQ BOOST (CONFIG_DEVFREQ_BOOST=y)
# ═══════════════════════════════════════════════════════════════════════════
write /sys/module/devfreq_boost/parameters/wake_boost_duration 250
write /sys/module/devfreq_boost/parameters/input_boost_duration 64

# ═══════════════════════════════════════════════════════════════════════════
# 8. ADRENO 506 IDLER (CONFIG_ADRENO_IDLER=y)
# ═══════════════════════════════════════════════════════════════════════════
write /sys/module/adreno_idler/parameters/adreno_idler_active Y
write /sys/module/adreno_idler/parameters/adreno_idler_idleworkload 7000
write /sys/module/adreno_idler/parameters/adreno_idler_downdifferential 20
write /sys/module/adreno_idler/parameters/adreno_idler_idlewait 20

# GPU governor
[ -e /sys/class/kgsl/kgsl-3d0/devfreq/governor ] && \
    write_str /sys/class/kgsl/kgsl-3d0/devfreq/governor "msm-adreno-tz"

# ═══════════════════════════════════════════════════════════════════════════
# 9. BUS DCVS (DDR bandwidth)
# ═══════════════════════════════════════════════════════════════════════════
write_str /sys/class/devfreq/soc:qcom,mincpubw/governor "cpufreq"

if [ -d /sys/class/devfreq/soc:qcom,cpubw ]; then
    write_str /sys/class/devfreq/soc:qcom,cpubw/governor "bw_hwmon"
    write /sys/class/devfreq/soc:qcom,cpubw/polling_interval 50
    write /sys/class/devfreq/soc:qcom,cpubw/min_freq 769
    write /sys/class/devfreq/soc:qcom,cpubw/bw_hwmon/io_percent 40
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
    write /sys/class/devfreq/soc:qcom,gpubw/bw_hwmon/io_percent 45
fi

# ═══════════════════════════════════════════════════════════════════════════
# 10. ВОССТАНОВЛЕНИЕ ТЕРМОКОНТРОЛЯ
# ═══════════════════════════════════════════════════════════════════════════
write /sys/module/msm_thermal/core_control/enabled 1

# ═══════════════════════════════════════════════════════════════════════════
# 11. LPM (Low Power Modes)
# ═══════════════════════════════════════════════════════════════════════════
write /sys/module/lpm_levels/parameters/sleep_disabled 0
[ -e /sys/module/lpm_levels/lpm_workarounds/dynamic_clock_gating ] && \
    write /sys/module/lpm_levels/lpm_workarounds/dynamic_clock_gating 1

# ═══════════════════════════════════════════════════════════════════════════
# 12. I/O TUNING — КРИТИЧНО для eMMC 5.1 на ядре 4.9
# ═══════════════════════════════════════════════════════════════════════════
for queue in /sys/block/mmcblk0/queue /sys/block/mmcblk1/queue; do
    [ -d "$queue" ] || continue

    if grep -q maple "$queue/scheduler" 2>/dev/null; then
        write_str "$queue/scheduler" "maple"
    elif grep -q bfq "$queue/scheduler" 2>/dev/null; then
        write_str "$queue/scheduler" "bfq"
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

# Device-mapper (dm-verity, dm-crypt)
for queue in /sys/block/dm-*/queue; do
    [ -d "$queue" ] || continue
    write "$queue/read_ahead_kb" 128
    write "$queue/nr_requests" 128
    write "$queue/iostats" 0
    write "$queue/add_random" 0
done

# BDI read_ahead
for node in /sys/block/mmcblk0/bdi/read_ahead_kb /sys/block/mmcblk0rpmb/bdi/read_ahead_kb; do
    [ -e "$node" ] && echo 128 > "$node"
done

# ── Защита eMMC и ZRAM Tuning ──────────────────────────────────────────────
if [ -d /sys/block/zram0 ]; then
    # Отключаем опережающее чтение для RAM-диска
    write /sys/block/zram0/queue/read_ahead_kb 0

    # Если есть бэкпорт zRAM writeback — принудительно глушим
    write /sys/block/zram0/writeback_limit_enable 0
    write /sys/block/zram0/writeback_limit 0

    # Защита от монтирования накопителя eMMC под нужды zRAM
    if [ -e /sys/block/zram0/backing_dev ]; then
        backing_val="$(cat /sys/block/zram0/backing_dev 2>/dev/null)"
        if [ -n "$backing_val" ] && [ "$backing_val" != "none" ]; then
            log -t "$LOGTAG" -p w "WARNING: zRAM backing_dev active ($backing_val)! Resetting for eMMC protection..."
            write /sys/block/zram0/backing_dev "none"
        fi
    fi
fi

# ═══════════════════════════════════════════════════════════════════════════
# 13. ОТКЛЮЧЕНИЕ LEGACY LMK (ядерный)
# ═══════════════════════════════════════════════════════════════════════════
write /sys/module/lowmemorykiller/parameters/enable_lmk 0
write /sys/module/lowmemorykiller/parameters/enable_adaptive_lmk 0
write /sys/module/process_reclaim/parameters/enable_process_reclaim 0

write /proc/sys/vm/reap_mem_on_sigkill 1
write /sys/module/lowmemorykiller/parameters/oom_reaper 1

# ═══════════════════════════════════════════════════════════════════════════
# 14. KERNEL STABILITY
# ═══════════════════════════════════════════════════════════════════════════
write /proc/sys/kernel/panic 5
write /proc/sys/kernel/printk_devkmsg off
write /proc/sys/kernel/hung_task_timeout_secs 0

# ═══════════════════════════════════════════════════════════════════════════
# 15. SOC INFO & MISC
# ═══════════════════════════════════════════════════════════════════════════
setprop vendor.post_boot.parsed 1

if [ -f /sys/devices/soc0/select_image ]; then
    write /sys/devices/soc0/select_image 10
    write_str /sys/devices/soc0/image_version "10:$(getprop ro.build.id):$(getprop ro.build.version.incremental)"
    write_str /sys/devices/soc0/image_variant "$(getprop ro.product.name)-$(getprop ro.build.type)"
    write_str /sys/devices/soc0/image_crm_version "$(getprop ro.build.version.codename)"
fi

[ "$(getprop persist.vendor.console.silent.config)" = "1" ] && \
    write /proc/sys/kernel/printk 0

misc_link="$(ls -l /dev/block/bootdevice/by-name/misc 2>/dev/null)"
real_path="${misc_link##*>}"
[ -n "$real_path" ] && setprop persist.vendor.mmi.misc_dev_path "$real_path"

setprop vendor.powerhal.init 1
setprop vendor.post_boot.parsed 1

log -t "$LOGTAG" -p i "post_boot complete, Power HAL initiated"
