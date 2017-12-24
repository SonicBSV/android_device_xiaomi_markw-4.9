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

import android.content.ContentResolver;
import android.content.Context;
import android.content.SharedPreferences;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceManager;
import android.support.v7.preference.PreferenceViewHolder;
import android.database.ContentObserver;
import android.util.AttributeSet;
import android.view.View;
import android.widget.SeekBar;
import android.widget.TextView;
import android.os.Bundle;
import android.util.Log;

import java.util.List;

public class YellowTorchBrightnessPreference extends Preference implements
        SeekBar.OnSeekBarChangeListener {

    private SeekBar mSeekBar;
    private int mOldBrightness;
    private int mMinValue;
    private int mMaxValue;
    private float offset;
    private TextView mValueText;

    private static final String FILE_BRIGHTNESS = "/sys/devices/soc/qpnp-flash-led-25/leds/led:torch_1/max_brightness";

    public YellowTorchBrightnessPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        mMinValue = 0;
        mMaxValue = 200;
        offset = mMaxValue / 100f;

        setLayoutResource(R.layout.preference_seek_bar);
    }

    @Override
    public void onBindViewHolder(PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);

        mOldBrightness = Integer.parseInt(getValue(getContext()));
        mSeekBar = (SeekBar) holder.findViewById(R.id.seekbar);
        mSeekBar.setMax(mMaxValue - mMinValue);
        mSeekBar.setProgress(mOldBrightness - mMinValue);
        mValueText = (TextView) holder.findViewById(R.id.current_value);
        mValueText.setText(Integer.toString(Math.round(mOldBrightness / offset)) + "%");
        mSeekBar.setOnSeekBarChangeListener(this);
    }

    public static boolean isSupported() {
        return Utils.fileWritable(FILE_BRIGHTNESS);
    }

    public static String getValue(Context context) {
        return Utils.getFileValue(FILE_BRIGHTNESS, "200");
    }

    private void setValue(String newValue) {
        Utils.writeValue(FILE_BRIGHTNESS, newValue);
        SharedPreferences.Editor editor = PreferenceManager.getDefaultSharedPreferences(getContext()).edit();
        editor.putString(DeviceSettings.KEY_YELLOW_TORCH_BRIGHTNESS, newValue);
        editor.commit();
    }

    public static void restore(Context context) {
        if (!isSupported()) {
            return;
        }

        String storedValue = PreferenceManager.getDefaultSharedPreferences(context).getString(DeviceSettings.KEY_YELLOW_TORCH_BRIGHTNESS, "200");
        Utils.writeValue(FILE_BRIGHTNESS, storedValue);
    }

    public void onProgressChanged(SeekBar seekBar, int progress,
            boolean fromTouch) {
        setValue(String.valueOf(progress + mMinValue));
        mValueText.setText(Integer.toString(Math.round((progress + mMinValue) / offset)) + "%");
    }

    public void onStartTrackingTouch(SeekBar seekBar) {
        // NA
    }

    public void onStopTrackingTouch(SeekBar seekBar) {
        // NA
    }
}

