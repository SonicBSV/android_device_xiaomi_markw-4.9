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
import com.thht.settings.device.torch.TorchBrightnessDialogFragment;
import com.thht.settings.device.vibrator.VibratorStrengthDialogFragment;

import android.util.Log;

public class CustomPreferenceFragment extends PreferenceFragment implements
        Preference.OnPreferenceChangeListener, Preference.OnPreferenceClickListener {
    // UI elements
    Preference mDiracPref;
    Preference mVibratorPref, mYTorchPref, mWTorchPref;
    SwitchPreference mRestorePref, mWakeUpPref;

    // SharedPreferences
    Boolean mShouldRestore;
    Boolean mShouldRestorePreset;
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
        // wakeup fix
        else if (preference == mWakeUpPref) {
            boolean value = (Boolean) newValue;
            editor.putBoolean(StaticMembers.KEY_SLOW_WAKEUP_FIX, value);
            setSlowWakeupFix(value);
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
        // preferences
        mDiracPref = findPreference(StaticMembers.KEY_DIRAC);
        mYTorchPref = findPreference(StaticMembers.KEY_YELLOW_TORCH_BRIGHTNESS);
        mWTorchPref = findPreference(StaticMembers.KEY_WHITE_TORCH_BRIGHTNESS);
        mVibratorPref = findPreference(StaticMembers.KEY_VIB_STRENGTH);

        // switchpreferences
        mRestorePref = (SwitchPreference) findPreference(StaticMembers.KEY_RESTORE_ON_BOOT);
        mWakeUpPref = (SwitchPreference) findPreference(StaticMembers.KEY_SLOW_WAKEUP_FIX);

        // sharedPreferences
        mShouldRestore = PreferenceManager.getDefaultSharedPreferences(getContext()).
                getBoolean(StaticMembers.KEY_RESTORE_ON_BOOT, false);
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

        // wakeup
        mWakeUpPref.setEnabled(Utils.fileWritable(StaticMembers.FILE_LEVEL_WAKEUP));
    }

    // Set current state
    private void setCurrentState() {
        // restore on boot
        mRestorePref.setChecked(mShouldRestore);

        // wakeup fix
        if (mWakeUpPref.isEnabled()) {
            mWakeUpPref.setChecked(mShouldFixSlowWakeUp);
        }
    }

    // Set change listeners
    private void setListeners() {
        // preference change listeners
        mRestorePref.setOnPreferenceChangeListener(this);
        mWakeUpPref.setOnPreferenceChangeListener(this);

        // preference click listeners
        mVibratorPref.setOnPreferenceClickListener(this);
        mYTorchPref.setOnPreferenceClickListener(this);
        mWTorchPref.setOnPreferenceClickListener(this);
        mDiracPref.setOnPreferenceClickListener(this);
    }

    // toggle wakeup hack
    public static void setSlowWakeupFix(boolean value) {
        if (value)
            Utils.writeValue(StaticMembers.FILE_LEVEL_WAKEUP, "1");
        else
            Utils.writeValue(StaticMembers.FILE_LEVEL_WAKEUP, "0");
    }
}
