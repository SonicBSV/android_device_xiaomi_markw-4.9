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
import android.os.Vibrator;

import java.util.List;

public class VibratorStrengthPreference extends Preference implements
        SeekBar.OnSeekBarChangeListener {

    private SeekBar mSeekBar;
    private int mOldStrength;
    private int mMinValue;
    private int mMaxValue;
    private float offset;
    private Vibrator mVibrator;
    private TextView mValueText;

    private static final String FILE_LEVEL = "/sys/class/timed_output/vibrator/vtg_level";
    private static final long testVibrationPattern[] = {0,250};

    public VibratorStrengthPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        // from drivers/platform/msm/qpnp-haptic.c
        // #define QPNP_HAP_VMAX_MIN_MV		116
        // #define QPNP_HAP_VMAX_MAX_MV		3596
        mMinValue = 116;
        mMaxValue = 3596;
        offset = mMaxValue / 100f;

        mVibrator = (Vibrator) context.getSystemService(Context.VIBRATOR_SERVICE);
        setLayoutResource(R.layout.preference_seek_bar);
    }

    @Override
    public void onBindViewHolder(PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);

        mOldStrength = Integer.parseInt(getValue(getContext()));
        mSeekBar = (SeekBar) holder.findViewById(R.id.seekbar);
        mSeekBar.setMax(mMaxValue - mMinValue);
        mSeekBar.setProgress(mOldStrength - mMinValue);
        mValueText = (TextView) holder.findViewById(R.id.current_value);
        mValueText.setText(Integer.toString(Math.round(mOldStrength / offset)) + "%");
        mSeekBar.setOnSeekBarChangeListener(this);
    }

    public static boolean isSupported() {
        return Utils.fileWritable(FILE_LEVEL);
    }

    public static String getValue(Context context) {
        return Utils.getFileValue(FILE_LEVEL, "2700");
    }

    private void setValue(String newValue, boolean withFeedback) {
        Utils.writeValue(FILE_LEVEL, newValue);
        SharedPreferences.Editor editor = PreferenceManager.getDefaultSharedPreferences(getContext()).edit();
        editor.putString(DeviceSettings.KEY_VIBSTRENGTH, newValue);
        editor.commit();
	    if (withFeedback) {
            mVibrator.vibrate(testVibrationPattern, -1);
        }
    }

    public static void restore(Context context) {
        if (!isSupported()) {
            return;
        }

        String storedValue = PreferenceManager.getDefaultSharedPreferences(context).getString(DeviceSettings.KEY_VIBSTRENGTH, "2700");
        Utils.writeValue(FILE_LEVEL, storedValue);
    }

    public void onProgressChanged(SeekBar seekBar, int progress,
            boolean fromTouch) {
        setValue(String.valueOf(progress + mMinValue), true);
        mValueText.setText(Integer.toString(Math.round((progress + mMinValue) / offset)) + "%");
    }

    public void onStartTrackingTouch(SeekBar seekBar) {
        // NA
    }

    public void onStopTrackingTouch(SeekBar seekBar) {
        // NA
    }
}

