package com.thht.settings.device.dirac;


import android.content.Context;
import android.os.Bundle;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.preference.TwoStatePreference;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Toast;

import com.thht.settings.device.R;

public class DiracSettingsFragment extends PreferenceFragment {


    public DiracSettingsFragment() {
        // Required empty public constructor
    }
    
    private static final String KEY_MI_SOUND_ENHANCER = "mi_sound_enhancer";
    private static final String KEY_HEADSET_TYPE = "dirac_headsets";
    private static final String KEY_DIRAC_PRESET = "dirac_preset";
    private TwoStatePreference mEnableDisableDirac;
    private ListPreference mHeadsetType, mDiracPreset;
    private Context mContext;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.dirac_settings);
        mContext = getActivity();
        
        // Dirac switch
        mEnableDisableDirac = (TwoStatePreference) findPreference(KEY_MI_SOUND_ENHANCER);
        if (AudioEnhancerService.du.hasInitialized()) {
            mEnableDisableDirac.setChecked(AudioEnhancerService.du.isEnabled(mContext));
        }
        mEnableDisableDirac.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener() {
            public boolean onPreferenceChange(Preference preference, Object newValue) {
                if(((TwoStatePreference) preference).isChecked() != (Boolean) newValue) {
                    AudioEnhancerService.du.setEnabled(mContext, (Boolean) newValue);
                }
                return true;
            }
        });

        // Dirac headset type
        // Dirac preset type
        mHeadsetType = (ListPreference) findPreference(KEY_HEADSET_TYPE);
        mDiracPreset = (ListPreference) findPreference(KEY_DIRAC_PRESET);

        if (AudioEnhancerService.du.hasInitialized()) {
            mHeadsetType.setValue(Integer.toString(AudioEnhancerService.du.getHeadsetType(mContext)));
        }

        mHeadsetType.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener() {
            public boolean onPreferenceChange(Preference preference, Object newValue) {
                Toast.makeText(mContext,"Applied HeadSetType : " + String.valueOf(newValue),Toast.LENGTH_SHORT).show();
                int val = Integer.parseInt(newValue.toString());
                AudioEnhancerService.du.setHeadsetType(mContext, val);
                return true;
            }
        });
        mDiracPreset.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener() {
            public boolean onPreferenceChange(Preference preference, Object newValue) {
                Toast.makeText(mContext,"Applied Level : " + String.valueOf(newValue),Toast.LENGTH_SHORT).show();
                AudioEnhancerService.du.setLevel(mContext, String.valueOf(newValue));
                return true;
            }
        });
    }
}
