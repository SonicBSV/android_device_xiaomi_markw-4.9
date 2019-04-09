package com.thht.settings.device;

import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceCategory;
import android.preference.PreferenceFragment;
import android.preference.PreferenceManager;
import android.preference.PreferenceScreen;
import android.preference.SwitchPreference;
import android.view.MenuItem;

import com.thht.settings.device.dirac.DiracSettingsFragment;
import com.thht.settings.device.helpers.StaticMembers;
import com.thht.settings.device.helpers.Utils;
import com.thht.settings.device.kcal.KcalExtrasDialogFragment;
import com.thht.settings.device.kcal.KcalPresets;
import com.thht.settings.device.kcal.KcalRGBDialogFragment;
import com.thht.settings.device.torch.TorchBrightnessDialogFragment;
import com.thht.settings.device.vibrator.VibratorStrengthDialogFragment;

import android.util.Log;

public class CustomPreferenceFragment extends PreferenceFragment implements
        Preference.OnPreferenceChangeListener, Preference.OnPreferenceClickListener {
    // UI elements
    Preference mDiracPref;
    Preference mVibratorPref, mYTorchPref, mWTorchPref;
    Preference mKCALRPref, mKCALGPref, mKCALBPref;
    Preference mKCALMinPref, mKCALSatPref, mKCALHuePref;
    Preference mKCALValPref, mKCALContPref;
    ListPreference mKCALPresetListPref;
    SwitchPreference mKCALPresetPref;
    SwitchPreference mRestorePref, mWakeUpPref;
    PreferenceCategory mKCALScrCat, mKCALExtrasCat;

    // SharedPreferences
    Boolean mShouldRestore;
    Boolean mShouldRestorePreset;
    String mKcalPresetsValue;
    Boolean mShouldFixSlowWakeUp;

    public CustomPreferenceFragment() {
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.main);

        // initialise ui elements
        initialise();
        // disable unsupported elements
        sanitise();

        setCurrentState();
        setListeners();
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        return super.onOptionsItemSelected(item);
    }

    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen, Preference preference) {
        return super.onPreferenceTreeClick(preferenceScreen, preference);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {

        SharedPreferences.Editor editor = PreferenceManager.getDefaultSharedPreferences(getContext()).edit();

        // restore on boot
        if (preference == mRestorePref) {
            boolean value = (Boolean) newValue;
            editor.putBoolean(StaticMembers.KEY_RESTORE_ON_BOOT, value);
        }
        // kcal preset switch
        else if (preference == mKCALPresetPref) {
            boolean value = (Boolean) newValue;
            editor.putBoolean(StaticMembers.KEY_KCAL_PRESETS, value);
            setKcalPresetsDependents(value);
        }
        // wakeup fix
        else if (preference == mWakeUpPref) {
            boolean value = (Boolean) newValue;
            editor.putBoolean(StaticMembers.KEY_SLOW_WAKEUP_FIX, value);
            setSlowWakeupFix(value);
        }
        // kcal profile
        else if (preference == mKCALPresetListPref) {
            String currValue = (String) newValue;
            editor.putString(StaticMembers.KEY_KCAL_PRESETS_LIST, currValue);
            KcalPresets.setValue(currValue);
        }

        editor.apply();

        return true;
    }

    // launch corresponding fragments on preference click
    // dirac preference is launched statically from xml
    @Override
    public boolean onPreferenceClick(Preference preference) {
        if (preference == mYTorchPref) {
            TorchBrightnessDialogFragment.newInstance(StaticMembers.FILE_LEVEL_TORCH_YELLOW,
                    StaticMembers.KEY_YELLOW_TORCH_BRIGHTNESS).
                    show(getFragmentManager(), "YellowTorch");
        } else if (preference == mWTorchPref) {
            TorchBrightnessDialogFragment.newInstance(StaticMembers.FILE_LEVEL_TORCH_WHITE,
                    StaticMembers.KEY_WHITE_TORCH_BRIGHTNESS).
                    show(getFragmentManager(), "WhiteTorch");
        } else if (preference == mVibratorPref) {
            VibratorStrengthDialogFragment.newInstance(StaticMembers.KEY_VIB_STRENGTH).
                    show(getFragmentManager(), "VibratorStrength");
        } else if (preference == mKCALRPref) {
            KcalRGBDialogFragment.newInstance(0, StaticMembers.KEY_KCAL_RGB_RED).
                    show(getFragmentManager(), "KcalRed");
        } else if (preference == mKCALGPref) {
            KcalRGBDialogFragment.newInstance(1, StaticMembers.KEY_KCAL_RGB_GREEN).
                    show(getFragmentManager(), "KcalGreen");
        } else if (preference == mKCALBPref) {
            KcalRGBDialogFragment.newInstance(2, StaticMembers.KEY_KCAL_RGB_BLUE).
                    show(getFragmentManager(), "KcalBlue");
        } else if (preference == mKCALMinPref) {
            KcalExtrasDialogFragment.newInstance(StaticMembers.FILE_LEVEL_KCAL_MIN,
                    StaticMembers.DEFAULT_VALUE_KCAL_MIN,
                    0, 255, 0,
                    StaticMembers.KEY_KCAL_RGB_MIN).
                    show(getFragmentManager(), "KcalMin");
        } else if (preference == mKCALSatPref) {
            KcalExtrasDialogFragment.newInstance(StaticMembers.FILE_LEVEL_KCAL_SAT,
                    StaticMembers.DEFAULT_VALUE_KCAL_SAT,
                    0, 158, 225,
                    StaticMembers.KEY_KCAL_SAT_INTENSITY).
                    show(getFragmentManager(), "KcalSat");
        } else if (preference == mKCALHuePref) {
            KcalExtrasDialogFragment.newInstance(StaticMembers.FILE_LEVEL_KCAL_HUE,
                    StaticMembers.DEFAULT_VALUE_KCAL_HUE,
                    0, 1536, 0,
                    StaticMembers.KEY_KCAL_SCR_HUE).
                    show(getFragmentManager(), "KcalHue");
        } else if (preference == mKCALValPref) {
            KcalExtrasDialogFragment.newInstance(StaticMembers.FILE_LEVEL_KCAL_VAL,
                    StaticMembers.DEFAULT_VALUE_KCAL_VAL,
                    0, 255, 128,
                    StaticMembers.KEY_KCAL_SCR_VAL).
                    show(getFragmentManager(), "KcalVal");
        } else if (preference == mKCALContPref) {
            KcalExtrasDialogFragment.newInstance(StaticMembers.FILE_LEVEL_KCAL_CONT,
                    StaticMembers.DEFAULT_VALUE_KCAL_CONT,
                    0, 255, 128,
                    StaticMembers.KEY_KCAL_SCR_CONT).
                    show(getFragmentManager(), "KcalCont");
        } else if (preference == mDiracPref) {
            getFragmentManager().beginTransaction().
                    replace(R.id.container, new DiracSettingsFragment()).
                    addToBackStack("AdvCntrlDirac").
                    commit();
        }

        return true;
    }

    // Initialise the UI elements
    private void initialise() {
        // categories
        mKCALExtrasCat = (PreferenceCategory) findPreference(StaticMembers.KEY_CATEGORY_KCAL_EXTRAS);
        mKCALScrCat = (PreferenceCategory) findPreference(StaticMembers.KEY_CATEGORY_SCREEN_COLOR);
        
        // preferences
        mDiracPref = findPreference(StaticMembers.KEY_DIRAC);
        mYTorchPref = findPreference(StaticMembers.KEY_YELLOW_TORCH_BRIGHTNESS);
        mWTorchPref = findPreference(StaticMembers.KEY_WHITE_TORCH_BRIGHTNESS);
        mVibratorPref = findPreference(StaticMembers.KEY_VIB_STRENGTH);
        mKCALRPref = findPreference(StaticMembers.KEY_KCAL_RGB_RED);
        mKCALGPref = findPreference(StaticMembers.KEY_KCAL_RGB_GREEN);
        mKCALBPref = findPreference(StaticMembers.KEY_KCAL_RGB_BLUE);
        mKCALMinPref = findPreference(StaticMembers.KEY_KCAL_RGB_MIN);
        mKCALSatPref = findPreference(StaticMembers.KEY_KCAL_SAT_INTENSITY);
        mKCALHuePref = findPreference(StaticMembers.KEY_KCAL_SCR_HUE);
        mKCALValPref = findPreference(StaticMembers.KEY_KCAL_SCR_VAL);
        mKCALContPref = findPreference(StaticMembers.KEY_KCAL_SCR_CONT);

        // listpreference
        mKCALPresetListPref = (ListPreference) findPreference(StaticMembers.KEY_KCAL_PRESETS_LIST);

        // switchpreferences
        mKCALPresetPref = (SwitchPreference) findPreference(StaticMembers.KEY_KCAL_PRESETS);
        mRestorePref = (SwitchPreference) findPreference(StaticMembers.KEY_RESTORE_ON_BOOT);
        mWakeUpPref = (SwitchPreference) findPreference(StaticMembers.KEY_SLOW_WAKEUP_FIX);

        // sharedPreferences
        mShouldRestore = PreferenceManager.getDefaultSharedPreferences(getContext()).
                getBoolean(StaticMembers.KEY_RESTORE_ON_BOOT, false);
        mShouldRestorePreset = PreferenceManager.getDefaultSharedPreferences(getContext()).
                getBoolean(StaticMembers.KEY_KCAL_PRESETS, false);
        mKcalPresetsValue = mShouldRestore && mShouldRestorePreset ?
                PreferenceManager.getDefaultSharedPreferences(getContext()).
                        getString(StaticMembers.KEY_KCAL_PRESETS_LIST, "0") : "0";
        mShouldFixSlowWakeUp = PreferenceManager.getDefaultSharedPreferences(getContext()).
                getBoolean(StaticMembers.KEY_SLOW_WAKEUP_FIX, false);
    }

    // Disable unsupported elements
    private void sanitise() {

        // torch
        mYTorchPref.setEnabled(TorchBrightnessDialogFragment.isSupported(StaticMembers.FILE_LEVEL_TORCH_YELLOW));
        mWTorchPref.setEnabled(TorchBrightnessDialogFragment.isSupported(StaticMembers.FILE_LEVEL_TORCH_WHITE));

        // vibrator
        mVibratorPref.setEnabled(VibratorStrengthDialogFragment.isSupported());

        // kcal screen
        boolean isKcalSupported = KcalRGBDialogFragment.isSupported();
        mKCALRPref.setEnabled(isKcalSupported);
        mKCALGPref.setEnabled(isKcalSupported);
        mKCALBPref.setEnabled(isKcalSupported);

        // kcal extras
        mKCALMinPref.setEnabled(isKcalSupported);
        mKCALSatPref.setEnabled(isKcalSupported);
        mKCALHuePref.setEnabled(isKcalSupported);
        mKCALValPref.setEnabled(isKcalSupported);
        mKCALContPref.setEnabled(isKcalSupported);

        // kcal presets
        mKCALPresetPref.setEnabled(isKcalSupported);
        mKCALPresetListPref.setEnabled(isKcalSupported);

        // wakeup
        mWakeUpPref.setEnabled(Utils.fileWritable(StaticMembers.FILE_LEVEL_WAKEUP));
    }

    // Set current state
    private void setCurrentState() {
        // restore on boot
        mRestorePref.setChecked(mShouldRestore);

        // kcal presets
        if (KcalRGBDialogFragment.isSupported()) {
            mKCALPresetPref.setChecked(mShouldRestorePreset);
            setKcalPresetsDependents(mShouldRestorePreset);
            if (mShouldRestore && mShouldRestorePreset)
                mKCALPresetListPref.setValue(mKcalPresetsValue);
        }

        // wakeup fix
        if (mWakeUpPref.isEnabled()) {
            mWakeUpPref.setChecked(mShouldFixSlowWakeUp);
        }
    }

    // Set change listeners
    private void setListeners() {
        // preference change listeners
        mRestorePref.setOnPreferenceChangeListener(this);
        mKCALPresetPref.setOnPreferenceChangeListener(this);
        mKCALPresetListPref.setOnPreferenceChangeListener(this);
        mWakeUpPref.setOnPreferenceChangeListener(this);

        // preference click listeners
        mVibratorPref.setOnPreferenceClickListener(this);
        mYTorchPref.setOnPreferenceClickListener(this);
        mWTorchPref.setOnPreferenceClickListener(this);
        mKCALRPref.setOnPreferenceClickListener(this);
        mKCALGPref.setOnPreferenceClickListener(this);
        mKCALBPref.setOnPreferenceClickListener(this);
        mKCALMinPref.setOnPreferenceClickListener(this);
        mKCALSatPref.setOnPreferenceClickListener(this);
        mKCALHuePref.setOnPreferenceClickListener(this);
        mKCALValPref.setOnPreferenceClickListener(this);
        mKCALContPref.setOnPreferenceClickListener(this);
        mDiracPref.setOnPreferenceClickListener(this);
    }

    // disable kcal manual settings when using a profile
    private void setKcalPresetsDependents(boolean value) {
        mKCALPresetListPref.setEnabled(value);
        mKCALScrCat.setEnabled(!value);
        mKCALExtrasCat.setEnabled(!value);
    }

    // toggle wakeup hack
    public static void setSlowWakeupFix(boolean value) {
        if (value)
            Utils.writeValue(StaticMembers.FILE_LEVEL_WAKEUP, "1");
        else
            Utils.writeValue(StaticMembers.FILE_LEVEL_WAKEUP, "0");
    }
}
