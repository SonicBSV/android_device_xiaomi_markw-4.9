package com.xiaomi.parts.util;

import android.content.ContentResolver;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.util.Log;

import java.util.ArrayList;
import java.util.List;


public class Utils {

    public static final String TAG = "CorvusActions";

    public static final String PREFERENCES = "CorvusPreferences";
    public static final String AMBIENT_GESTURE_HAPTIC_FEEDBACK =
            "AMBIENT_GESTURE_HAPTIC_FEEDBACK";

    private static final String SETTINGS_METADATA_NAME = "com.android.settings";

    public static int getIntSystem(Context context, ContentResolver cr, String name, int def) {
        int ret = getInt(context, name, def);
        return ret;
    }

    public static int getInt(Context context, String name, int def) {
        SharedPreferences settings = context.getSharedPreferences(PREFERENCES, 0);
        return settings.getInt(name, def);
    }

    public static boolean putIntSystem(Context context, ContentResolver cr, String name, int value) {
        boolean ret = putInt(context, name, value);
        return ret;
    }

    public static boolean putInt(Context context, String name, int value) {
        SharedPreferences settings = context.getSharedPreferences(PREFERENCES, 0);
        SharedPreferences.Editor editor = settings.edit();
        editor.putInt(name, value);
        return editor.commit();
    }

    public static void registerPreferenceChangeListener(Context context,
        SharedPreferences.OnSharedPreferenceChangeListener preferenceListener) {
        SharedPreferences settings = context.getSharedPreferences(PREFERENCES, 0);
        settings.registerOnSharedPreferenceChangeListener(preferenceListener);
    }

    public static void unregisterPreferenceChangeListener(Context context,
        SharedPreferences.OnSharedPreferenceChangeListener preferenceListener) {
        SharedPreferences settings = context.getSharedPreferences(PREFERENCES, 0);
        settings.unregisterOnSharedPreferenceChangeListener(preferenceListener);
    }
}
