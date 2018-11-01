/*
* Copyright (C) 2016 The OmniROM Project
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.com/licenses/>.
*
*/
package com.lineageos.settings.device;

import android.content.Context;
import android.content.SharedPreferences;
import android.support.v7.preference.PreferenceManager;
import android.support.v7.preference.PreferenceViewHolder;
import android.util.AttributeSet;

import java.util.List;

public class YellowTorchBrightnessPreference extends ProperSeekBarPreference {

    private static int mMinVal = 0;
    private static int mMaxVal = 200;
    private static int mDefVal = mMaxVal;

    private static final String FILE_BRIGHTNESS = "/sys/devices/soc/qpnp-flash-led-25/leds/led:torch_1/max_brightness";

    public YellowTorchBrightnessPreference(Context context, AttributeSet attrs) {
        super(context, attrs);

        mInterval = 10;
        mShowSign = false;
        mUnits = "";
        mContinuousUpdates = false;
        mMinValue = mMinVal;
        mMaxValue = mMaxVal;
        mDefaultValueExists = true;
        mDefaultValue = mDefVal;
        mValue = Integer.parseInt(loadValue());

        setPersistent(false);
    }

    public static boolean isSupported() {
        return Utils.fileWritable(FILE_BRIGHTNESS);
    }

    public static void restore(Context context) {
        if (!isSupported()) {
            return;
        }

        String storedValue = PreferenceManager.getDefaultSharedPreferences(context).getString(DeviceSettings.KEY_YELLOW_TORCH_BRIGHTNESS, String.valueOf(mDefVal));
        Utils.writeValue(FILE_BRIGHTNESS, storedValue);
    }

    public static String loadValue() {
        return Utils.getFileValue(FILE_BRIGHTNESS, String.valueOf(mDefVal));
    }

    private void saveValue(String newValue) {
        Utils.writeValue(FILE_BRIGHTNESS, newValue);
        SharedPreferences.Editor editor = PreferenceManager.getDefaultSharedPreferences(getContext()).edit();
        editor.putString(DeviceSettings.KEY_YELLOW_TORCH_BRIGHTNESS, newValue);
        editor.commit();
    }

    @Override
    protected void changeValue(int newValue) {
        saveValue(String.valueOf(newValue));
    }
}

