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
    // kcal presets
    public static final String KEY_CATEGORY_KCAL_PRESETS = "key_category_kcal_presets";
    public static final String KEY_KCAL_PRESETS = "key_kcal_presets";
    public static final String KEY_KCAL_PRESETS_LIST = "key_kcal_presets_list";
    // kcal rgb
    public static final String KEY_CATEGORY_SCREEN_COLOR = "key_category_screen_color";
    public static final String KEY_KCAL_RGB_RED = "key_kcal_rgb_red";
    public static final String KEY_KCAL_RGB_BLUE = "key_kcal_rgb_blue";
    public static final String KEY_KCAL_RGB_GREEN = "key_kcal_rgb_green";
    // kcal extras
    public static final String KEY_CATEGORY_KCAL_EXTRAS = "key_category_kcal_extras";
    public static final String KEY_KCAL_RGB_MIN = "key_kcal_rgb_min";
    public static final String KEY_KCAL_SAT_INTENSITY = "key_kcal_sat_intensity";
    public static final String KEY_KCAL_SCR_CONT = "key_kcal_scr_cont";
    public static final String KEY_KCAL_SCR_VAL = "key_kcal_scr_val";
    public static final String KEY_KCAL_SCR_HUE = "key_kcal_scr_hue";
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
    // kcal controls
    public static final String FILE_LEVEL_KCAL_RGB = "/sys/devices/platform/kcal_ctrl.0/kcal";
    public static final String FILE_LEVEL_KCAL_MIN = "/sys/devices/platform/kcal_ctrl.0/kcal_min";
    public static final String FILE_LEVEL_KCAL_SAT = "/sys/devices/platform/kcal_ctrl.0/kcal_sat";
    public static final String FILE_LEVEL_KCAL_CONT = "/sys/devices/platform/kcal_ctrl.0/kcal_cont";
    public static final String FILE_LEVEL_KCAL_HUE = "/sys/devices/platform/kcal_ctrl.0/kcal_hue";
    public static final String FILE_LEVEL_KCAL_VAL = "/sys/devices/platform/kcal_ctrl.0/kcal_val";
    // torch
    public static final String FILE_LEVEL_TORCH_WHITE = "/sys/devices/soc/qpnp-flash-led-24/leds/led:torch_0/max_brightness";
    public static final String FILE_LEVEL_TORCH_YELLOW = "/sys/devices/soc/qpnp-flash-led-24/leds/led:torch_1/max_brightness";
    // vibrator
    public static final String FILE_LEVEL_VIB_STRENGTH = "/sys/devices/virtual/timed_output/vibrator/vtg_level";

    /* Default Values */
    // kcal
    public static final String DEFAULT_VALUE_KCAL_RGB = "237 237 237";
    public static final String DEFAULT_VALUE_KCAL_MIN = "35";
    public static final String DEFAULT_VALUE_KCAL_SAT = "33";
    public static final String DEFAULT_VALUE_KCAL_CONT = "127";
    public static final String DEFAULT_VALUE_KCAL_HUE = "0";
    public static final String DEFAULT_VALUE_KCAL_VAL = "127";
    // vibrator
    public static final int DEFAULT_VALUE_VIB_STRENGTH = 2873;
    // torch
    public static final int DEFAULT_VALUE_TORCH_WHITE = 200;
    public static final int DEFAULT_VALUE_TORCH_YELLOW = 200;
}
