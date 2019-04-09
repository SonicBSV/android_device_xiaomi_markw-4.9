package com.thht.settings.device.helpers;

public class StaticMembers {
    /* App tag */
    public static final String TAG = "thht7848";

    /* Keys for layout componenets */
    // vibrator
    public static final String KEY_CATEGORY_VIBRATOR = "key_category_vibrator";
    public static final String KEY_VIB_STRENGTH = "key_vib_strength";
    // torch
    public static final String KEY_CATEGORY_TORCH = "key_category_torch";
    public static final String KEY_YELLOW_TORCH_BRIGHTNESS = "key_yellow_torch_brightness";
    public static final String KEY_WHITE_TORCH_BRIGHTNESS = "key_white_torch_brightness";

    // device parts
    public static final String KEY_CATEGORY_SOUND = "key_category_sound";
    public static final String KEY_DIRAC = "key_dirac";
    // misc
    public static final String KEY_CATEGORY_MISC = "key_category_misc";
    public static final String KEY_RESTORE_ON_BOOT = "key_restore_on_boot";
    public static final String KEY_SLOW_WAKEUP_FIX = "key_slow_wakeup_fix";

    /* Name of kernel nodes */
    // wakeup hack
    public static final String FILE_LEVEL_WAKEUP = "/sys/devices/soc/qpnp-smbcharger-18/power_supply/battery/subsystem/bms/hi_power";

    // torch
    public static final String FILE_LEVEL_TORCH_WHITE = "/sys/devices/soc/qpnp-flash-led-24/leds/led:torch_0/max_brightness";
    public static final String FILE_LEVEL_TORCH_YELLOW = "/sys/devices/soc/qpnp-flash-led-24/leds/led:torch_1/max_brightness";
    // vibrator
    public static final String FILE_LEVEL_VIB_STRENGTH = "/sys/devices/virtual/timed_output/vibrator/vtg_level";

    /* Default Values */
    // vibrator
    public static final int DEFAULT_VALUE_VIB_STRENGTH = 2873;
    // torch
    public static final int DEFAULT_VALUE_TORCH_WHITE = 200;
    public static final int DEFAULT_VALUE_TORCH_YELLOW = 200;
}
