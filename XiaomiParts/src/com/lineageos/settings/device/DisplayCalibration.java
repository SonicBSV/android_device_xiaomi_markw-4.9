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
package com.lineageos.settings.device;

import android.content.Context;
import android.content.SharedPreferences;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.PreferenceActivity;
import android.preference.PreferenceManager;
import android.preference.SwitchPreference;
import android.view.MenuItem;
import android.widget.ImageView;
import android.widget.SeekBar;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuInflater;

import android.app.ActionBar;
import com.lineageos.settings.device.utils.SeekBarPreference;
import com.lineageos.settings.device.R;

public class DisplayCalibration extends PreferenceActivity implements
        OnPreferenceChangeListener {

    public static final String KEY_KCAL_ENABLED = "kcal_enabled";
    public static final String KEY_KCAL_RED = "kcal_red";
    public static final String KEY_KCAL_GREEN = "kcal_green";
    public static final String KEY_KCAL_BLUE = "kcal_blue";
    public static final String KEY_KCAL_SATURATION = "kcal_saturation";
    public static final String KEY_KCAL_CONTRAST = "kcal_contrast";
    public static final String KEY_KCAL_VALUE = "kcal_value";
    public static final String KEY_KCAL_HUE = "kcal_hue";
    public static final String KEY_KCAL_INVERT = "kcal_invert";
    public static final String KEY_KCAL_GREYSCALE = "kcal_greyscale";

    private SeekBarPreference mKcalRed;
    private SeekBarPreference mKcalBlue;
    private SeekBarPreference mKcalGreen;
    private SeekBarPreference mKcalSaturation;
    private SeekBarPreference mKcalContrast;
    private SeekBarPreference mKcalValue;
    private SeekBarPreference mKcalHue;
    private SharedPreferences mPrefs;
    private SwitchPreference mKcalEnabled;
    private SwitchPreference mKcalInvert;
    private SwitchPreference mKcalGreyscale;
    private boolean mEnabled;

    private String mRed;
    private String mBlue;
    private String mGreen;

    private static final String COLOR_FILE = "/sys/devices/platform/kcal_ctrl.0/kcal";
    private static final String COLOR_FILE_CONTRAST = "/sys/devices/platform/kcal_ctrl.0/kcal_cont";
    private static final String COLOR_FILE_SATURATION = "/sys/devices/platform/kcal_ctrl.0/kcal_sat";
    private static final String COLOR_FILE_VALUE = "/sys/devices/platform/kcal_ctrl.0/kcal_val";
    private static final String COLOR_FILE_HUE = "/sys/devices/platform/kcal_ctrl.0/kcal_hue";
    private static final String COLOR_FILE_ENABLE = "/sys/devices/platform/kcal_ctrl.0/kcal_enable";
    private static final String COLOR_FILE_INVERT = "/sys/devices/platform/kcal_ctrl.0/kcal_invert_obsolete";

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mPrefs = PreferenceManager.getDefaultSharedPreferences(this);

        getActionBar().setDisplayHomeAsUpEnabled(true);
        setContentView(R.layout.display_cal);

        ImageView imageView = (ImageView) findViewById(R.id.calibration_pic);
        imageView.setImageResource(R.drawable.calibration_png);

        addPreferencesFromResource(R.xml.display_calibration);

        mKcalEnabled = (SwitchPreference) findPreference(KEY_KCAL_ENABLED);
        mKcalEnabled.setChecked(mPrefs.getBoolean(DisplayCalibration.KEY_KCAL_ENABLED, false));
        mKcalEnabled.setOnPreferenceChangeListener(this);

        mKcalRed = (SeekBarPreference) findPreference(KEY_KCAL_RED);
        mKcalRed.setInitValue(mPrefs.getInt(KEY_KCAL_RED, mKcalRed.def));
        mKcalRed.setOnPreferenceChangeListener(this);

        mKcalGreen = (SeekBarPreference) findPreference(KEY_KCAL_GREEN);
        mKcalGreen.setInitValue(mPrefs.getInt(KEY_KCAL_GREEN, mKcalGreen.def));
        mKcalGreen.setOnPreferenceChangeListener(this);

        mKcalBlue = (SeekBarPreference) findPreference(KEY_KCAL_BLUE);
        mKcalBlue.setInitValue(mPrefs.getInt(KEY_KCAL_BLUE, mKcalBlue.def));
        mKcalBlue.setOnPreferenceChangeListener(this);

        mKcalSaturation = (SeekBarPreference) findPreference(KEY_KCAL_SATURATION);
        mKcalSaturation.setInitValue(mPrefs.getInt(KEY_KCAL_SATURATION, mKcalSaturation.def));
        mKcalSaturation.setOnPreferenceChangeListener(this);

        mKcalContrast = (SeekBarPreference) findPreference(KEY_KCAL_CONTRAST);
        mKcalContrast.setInitValue(mPrefs.getInt(KEY_KCAL_CONTRAST, mKcalContrast.def));
        mKcalContrast.setOnPreferenceChangeListener(this);

        mKcalValue = (SeekBarPreference) findPreference(KEY_KCAL_VALUE);
        mKcalValue.setInitValue(mPrefs.getInt(KEY_KCAL_VALUE, mKcalValue.def));
        mKcalValue.setOnPreferenceChangeListener(this);

        mKcalHue = (SeekBarPreference) findPreference(KEY_KCAL_HUE);
        mKcalHue.setInitValue(mPrefs.getInt(KEY_KCAL_HUE, mKcalHue.def));
        mKcalHue.setOnPreferenceChangeListener(this);

        mKcalInvert = (SwitchPreference) findPreference(KEY_KCAL_INVERT);
        mKcalInvert.setChecked(mPrefs.getBoolean(DisplayCalibration.KEY_KCAL_INVERT, false));
        mKcalInvert.setOnPreferenceChangeListener(this);

        mKcalGreyscale = (SwitchPreference) findPreference(KEY_KCAL_GREYSCALE);
        mKcalGreyscale.setChecked(mPrefs.getBoolean(DisplayCalibration.KEY_KCAL_GREYSCALE, false));
        mKcalGreyscale.setOnPreferenceChangeListener(this);

        mRed = String.valueOf(mPrefs.getInt(KEY_KCAL_RED, mKcalRed.def));
        mGreen = String.valueOf(mPrefs.getInt(KEY_KCAL_GREEN, mKcalGreen.def));
        mBlue = String.valueOf(mPrefs.getInt(KEY_KCAL_BLUE, mKcalBlue.def));

    }

    private boolean isSupported(String file) {
        return Utils.fileWritable(file);
    }

    public static void restore(Context context) {
       boolean storeEnabled = PreferenceManager
                .getDefaultSharedPreferences(context).getBoolean(DisplayCalibration.KEY_KCAL_ENABLED, false);
       if (storeEnabled) {
           Utils.writeValue(COLOR_FILE_ENABLE, "1");
           Utils.writeValue(COLOR_FILE, "1");
           int storedRed = PreferenceManager
                   .getDefaultSharedPreferences(context).getInt(DisplayCalibration.KEY_KCAL_RED, 256);
           int storedGreen = PreferenceManager
                   .getDefaultSharedPreferences(context).getInt(DisplayCalibration.KEY_KCAL_GREEN, 256);
           int storedBlue = PreferenceManager
                   .getDefaultSharedPreferences(context).getInt(DisplayCalibration.KEY_KCAL_BLUE, 256);
           int storedSaturation = PreferenceManager
                   .getDefaultSharedPreferences(context).getInt(DisplayCalibration.KEY_KCAL_SATURATION, 255);
           int storedContrast = PreferenceManager
                   .getDefaultSharedPreferences(context).getInt(DisplayCalibration.KEY_KCAL_CONTRAST, 255);
           int storedValue = PreferenceManager
                   .getDefaultSharedPreferences(context).getInt(DisplayCalibration.KEY_KCAL_VALUE, 255);
           int storedHue = PreferenceManager
                   .getDefaultSharedPreferences(context).getInt(DisplayCalibration.KEY_KCAL_HUE, 0);
           boolean storedInvert = PreferenceManager
                   .getDefaultSharedPreferences(context).getBoolean(DisplayCalibration.KEY_KCAL_INVERT, false);
           boolean storedGreyscale = PreferenceManager
                   .getDefaultSharedPreferences(context).getBoolean(DisplayCalibration.KEY_KCAL_GREYSCALE, false);
           String storedRGBValue = ((String) String.valueOf(storedRed)
                   + " " + String.valueOf(storedGreen) + " " +  String.valueOf(storedBlue));
           Utils.writeValue(COLOR_FILE, storedRGBValue);
           Utils.writeValue(COLOR_FILE_CONTRAST, String.valueOf(storedContrast));
           Utils.writeValue(COLOR_FILE_SATURATION, storedGreyscale ? "128" : String.valueOf(storedSaturation));
           Utils.writeValue(COLOR_FILE_VALUE, String.valueOf(storedValue));
           Utils.writeValue(COLOR_FILE_HUE, String.valueOf(storedHue));
           Utils.writeValue(COLOR_FILE_INVERT, storedInvert ? "1" : "0");
       }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.kcal_menu, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected (MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home:
                finish();
                return true;
            case R.id.menu_reset:
                reset();
                return true;
            default:
                break;
        }
        return super.onOptionsItemSelected(item);
    }

    public void reset() {
        int red = mKcalRed.reset();
        int green = mKcalGreen.reset();
        int blue = mKcalBlue.reset();
        int saturation = mKcalSaturation.reset();
        int contrast = mKcalContrast.reset();
        int value = mKcalValue.reset();
        int hue = mKcalHue.reset();
        boolean invert = false;
        boolean greyscale = false;

        mKcalInvert.setChecked(invert);
        mKcalGreyscale.setChecked(greyscale);

        mPrefs.edit().putInt(KEY_KCAL_RED, red).commit();
        mPrefs.edit().putInt(KEY_KCAL_GREEN, green).commit();
        mPrefs.edit().putInt(KEY_KCAL_BLUE, blue).commit();
        mPrefs.edit().putInt(KEY_KCAL_SATURATION, saturation).commit();
        mPrefs.edit().putInt(KEY_KCAL_CONTRAST, contrast).commit();
        mPrefs.edit().putInt(KEY_KCAL_VALUE, value).commit();
        mPrefs.edit().putInt(KEY_KCAL_HUE, hue).commit();
        mPrefs.edit().putBoolean(KEY_KCAL_INVERT, invert).commit();
        mPrefs.edit().putBoolean(KEY_KCAL_GREYSCALE, greyscale).commit();

        String storedRGBValue = Integer.toString(red) + " " + Integer.toString(green) + " " +  Integer.toString(blue);

        Utils.writeValue(COLOR_FILE, storedRGBValue);
        Utils.writeValue(COLOR_FILE_SATURATION, Integer.toString(saturation));
        Utils.writeValue(COLOR_FILE_CONTRAST, Integer.toString(contrast));
        Utils.writeValue(COLOR_FILE_VALUE, String.valueOf(value));
        Utils.writeValue(COLOR_FILE_HUE, String.valueOf(hue));
        Utils.writeValue(COLOR_FILE_INVERT, invert ? "1" : "0");
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        if (preference == mKcalEnabled) {
            Boolean enabled = (Boolean) newValue;
            mPrefs.edit().putBoolean(KEY_KCAL_ENABLED, enabled).commit();
            mRed = String.valueOf(mPrefs.getInt(KEY_KCAL_RED, 256));
            mBlue = String.valueOf(mPrefs.getInt(KEY_KCAL_BLUE, 256));
            mGreen = String.valueOf(mPrefs.getInt(KEY_KCAL_GREEN, 256));
            String storedValue = ((String) String.valueOf(mRed)
                   + " " + String.valueOf(mGreen) + " " +  String.valueOf(mBlue));
            String mSaturation = String.valueOf(mPrefs.getInt(KEY_KCAL_SATURATION, 255));
            String mContrast = String.valueOf(mPrefs.getInt(KEY_KCAL_CONTRAST, 255));
            String mValue = String.valueOf(mPrefs.getInt(KEY_KCAL_VALUE, 255));
            String mHue = String.valueOf(mPrefs.getInt(KEY_KCAL_HUE, 0));
            Boolean mInvert = mPrefs.getBoolean(DisplayCalibration.KEY_KCAL_INVERT, false);
            Boolean mGreyscale = mPrefs.getBoolean(DisplayCalibration.KEY_KCAL_GREYSCALE, false);
            Utils.writeValue(COLOR_FILE_ENABLE, enabled ? "1" : "0");
            Utils.writeValue(COLOR_FILE, storedValue);
            Utils.writeValue(COLOR_FILE_SATURATION, mGreyscale ? "128" : mSaturation);
            Utils.writeValue(COLOR_FILE_CONTRAST, mContrast);
            Utils.writeValue(COLOR_FILE_VALUE, mValue);
            Utils.writeValue(COLOR_FILE_HUE, mHue);
            Utils.writeValue(COLOR_FILE_INVERT, mInvert ? "1" : "0");
            return true;
        } else if (preference == mKcalRed) {
            float val = Float.parseFloat((String) newValue);
            mPrefs.edit().putInt(KEY_KCAL_RED, (int) val).commit();
            mGreen = String.valueOf(mPrefs.getInt(KEY_KCAL_GREEN, 256));
            mBlue = String.valueOf(mPrefs.getInt(KEY_KCAL_BLUE, 256));
            String strVal = ((String) newValue + " " + mGreen + " " +mBlue);
            Utils.writeValue(COLOR_FILE, strVal);
            return true;
        } else if (preference == mKcalGreen) {
            float val = Float.parseFloat((String) newValue);
            mPrefs.edit().putInt(KEY_KCAL_GREEN, (int) val).commit();
            mRed = String.valueOf(mPrefs.getInt(KEY_KCAL_RED, 256));
            mBlue = String.valueOf(mPrefs.getInt(KEY_KCAL_BLUE, 256));
            String strVal = ((String) mRed + " " + newValue + " " +mBlue);
            Utils.writeValue(COLOR_FILE, strVal);
            return true;
        } else if (preference == mKcalBlue) {
            float val = Float.parseFloat((String) newValue);
            mPrefs.edit().putInt(KEY_KCAL_BLUE, (int) val).commit();
            mRed = String.valueOf(mPrefs.getInt(KEY_KCAL_RED, 256));
            mGreen = String.valueOf(mPrefs.getInt(KEY_KCAL_GREEN, 256));
            String strVal = ((String) mRed + " " + mGreen + " " +newValue);
            Utils.writeValue(COLOR_FILE, strVal);
            return true;
        } else if (preference == mKcalSaturation) {
            float val = Float.parseFloat((String) newValue);
            mPrefs.edit().putInt(KEY_KCAL_SATURATION, (int) val).commit();
            String strVal = (String) newValue;
            Utils.writeValue(COLOR_FILE_SATURATION, strVal);
            return true;
        } else if (preference == mKcalContrast) {
            float val = Float.parseFloat((String) newValue);
            mPrefs.edit().putInt(KEY_KCAL_CONTRAST, (int) val).commit();
            String strVal = (String) newValue;
            Utils.writeValue(COLOR_FILE_CONTRAST, strVal);
            return true;
        } else if (preference == mKcalValue) {
            float val = Float.parseFloat((String) newValue);
            mPrefs.edit().putInt(KEY_KCAL_VALUE, (int) val).commit();
            String strVal = (String) newValue;
            Utils.writeValue(COLOR_FILE_VALUE, strVal);
            return true;
        } else if (preference == mKcalHue) {
            float val = Float.parseFloat((String) newValue);
            mPrefs.edit().putInt(KEY_KCAL_HUE, (int) val).commit();
            String strVal = (String) newValue;
            Utils.writeValue(COLOR_FILE_HUE, strVal);
            return true;
        } else if (preference == mKcalInvert) {
            Boolean invertEnabled = (Boolean) newValue;
            mPrefs.edit().putBoolean(KEY_KCAL_INVERT, invertEnabled).commit();
            Utils.writeValue(COLOR_FILE_INVERT, invertEnabled ? "1" : "0");
            return true;
        } else if (preference == mKcalGreyscale) {
            Boolean greyscaleEnabled = (Boolean) newValue;
            mPrefs.edit().putBoolean(KEY_KCAL_GREYSCALE, greyscaleEnabled).commit();
            String storedSaturation = String.valueOf(mPrefs.getInt(KEY_KCAL_SATURATION, 255));
            Utils.writeValue(COLOR_FILE_SATURATION, greyscaleEnabled ? "128" : String.valueOf(storedSaturation));
            return true;
        }
        return false;
    }
}
