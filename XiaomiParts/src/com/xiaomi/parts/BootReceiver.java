package com.xiaomi.parts;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import androidx.preference.PreferenceManager;
import android.provider.Settings;

import com.xiaomi.parts.ambient.SensorsDozeService;

public class BootReceiver extends BroadcastReceiver {

    public static final  String HEADPHONE_GAIN_PATH = "/sys/kernel/sound_control/headphone_gain";
    public static final  String MICROPHONE_GAIN_PATH = "/sys/kernel/sound_control/mic_gain";

    public void onReceive(Context context, Intent intent) {

        SharedPreferences sharedPrefs = PreferenceManager.getDefaultSharedPreferences(context);

        int gain = Settings.Secure.getInt(context.getContentResolver(),
                DeviceSettings.PREF_HEADPHONE_GAIN, 5);
        FileUtils.setValue(HEADPHONE_GAIN_PATH, gain + " " + gain);
        FileUtils.setValue(MICROPHONE_GAIN_PATH, Settings.Secure.getInt(context.getContentResolver(),
                DeviceSettings.PREF_MICROPHONE_GAIN, 0));

	//Ambient
        context.startService(new Intent(context, SensorsDozeService.class));

    }
}
