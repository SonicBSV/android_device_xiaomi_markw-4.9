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
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
*/
package com.markw.settings.device;

import android.content.ContentResolver;
import android.content.Context;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.database.ContentObserver;
import android.preference.SeekBarDialogPreference;
import android.util.AttributeSet;
import android.view.View;
import android.widget.SeekBar;
import android.widget.TextView;
import android.os.Bundle;
import android.util.Log;

import java.util.List;

public class YellowTorchBrightnessPreference extends SeekBarDialogPreference implements
        SeekBar.OnSeekBarChangeListener {

    private SeekBar mSeekBar;
    private int mOldBrightness;
    private int mMinValue;
    private int mMaxValue;
    private float offset;
    private TextView mValueText;

    private static final String FILE_BRIGHTNESS = "/sys/class/leds/led:torch_1/max_brightness";

    public YellowTorchBrightnessPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        mMinValue = 0;
        mMaxValue = 200;
        offset = mMaxValue / 100f;

        setDialogLayoutResource(R.layout.preference_dialog_torch_brightness);
    }

    @Override
    protected void showDialog(Bundle state) {
        super.showDialog(state);
    }

    @Override
    protected void onBindDialogView(View view) {
        super.onBindDialogView(view);

        mOldBrightness = Integer.parseInt(getValue(getContext()));
        mSeekBar = getSeekBar(view);
        mSeekBar.setMax(mMaxValue - mMinValue);
        mSeekBar.setProgress(mOldBrightness - mMinValue);
        mValueText = (TextView) view.findViewById(R.id.current_value);
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

    @Override
    protected void onDialogClosed(boolean positiveResult) {
        super.onDialogClosed(positiveResult);

        if (positiveResult) {
            final int value = mSeekBar.getProgress() + mMinValue;
            setValue(String.valueOf(value));
            SharedPreferences.Editor editor = PreferenceManager.getDefaultSharedPreferences(getContext()).edit();
            editor.putString(DeviceSettings.KEY_YELLOW_TORCH_BRIGHTNESS, String.valueOf(value));
            editor.commit();
        } else {
            restoreOldState();
        }
    }

    private void restoreOldState() {
        setValue(String.valueOf(mOldBrightness));
    }
}

