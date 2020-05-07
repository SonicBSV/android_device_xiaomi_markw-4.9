/*
 * Copyright (C) 2015-2016 The CyanogenMod Project
 *           (C) 2017 The LineageOS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.xiaomi.parts.ambient;

import android.app.ActionBar;
import android.content.Context;
import android.os.Bundle;
import android.provider.Settings;
import androidx.preference.PreferenceFragment;
import androidx.preference.SwitchPreference;
import androidx.preference.Preference;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CompoundButton;
import android.widget.Switch;
import android.widget.TextView;
import com.xiaomi.parts.util.Utils;
import com.xiaomi.parts.R;

public class AmbientGesturePreferenceFragment extends PreferenceFragment {

    private static final String KEY_GESTURE_HAND_WAVE = "gesture_hand_wave";
    private static final String KEY_GESTURE_PICK_UP = "gesture_pick_up";
    private static final String KEY_GESTURE_POCKET = "gesture_pocket";
    private static final String KEY_HAPTIC_FEEDBACK = "ambient_gesture_haptic_feedback";
    private static final String KEY_PROXIMITY_WAKE = "proximity_wake_enable";

    private TextView mSwitchBarText;

    private Switch mAmbientDisplaySwitch;
    private SwitchPreference mHandwavePreference;
    private SwitchPreference mHapticFeedback;
    private SwitchPreference mPickupPreference;
    private SwitchPreference mPocketPreference;
    private SwitchPreference mProximityWakePreference;

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        ActionBar actionbar = getActivity().getActionBar();
        actionbar.setDisplayHomeAsUpEnabled(true);
        actionbar.setTitle(R.string.ambient_display_enable_title);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        final View view = LayoutInflater.from(getContext()).inflate(R.layout.doze, container, false);
        ((ViewGroup) view).addView(super.onCreateView(inflater, container, savedInstanceState));
        return view;
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        boolean dozeEnabled = isDozeEnabled();

        View switchBar = view.findViewById(R.id.switch_bar);
        mAmbientDisplaySwitch = (Switch) switchBar.findViewById(android.R.id.switch_widget);
        mAmbientDisplaySwitch.setChecked(dozeEnabled);
        mAmbientDisplaySwitch.setOnCheckedChangeListener(mAmbientDisplayPrefListener);

        switchBar.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                mAmbientDisplaySwitch.toggle();
            }
        });

        mSwitchBarText = switchBar.findViewById(R.id.switch_text);
        mSwitchBarText.setText(dozeEnabled ? R.string.switch_bar_on :
                R.string.switch_bar_off);
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        addPreferencesFromResource(R.xml.ambient_gesture_panel);

        boolean dozeEnabled = isDozeEnabled();

        mHandwavePreference = (SwitchPreference) findPreference(KEY_GESTURE_HAND_WAVE);
        mHandwavePreference.setEnabled(dozeEnabled);
        mHandwavePreference.setOnPreferenceChangeListener(mGesturePrefListener);

        mPickupPreference = (SwitchPreference) findPreference(KEY_GESTURE_PICK_UP);
        mPickupPreference.setEnabled(dozeEnabled);

        mPocketPreference = (SwitchPreference) findPreference(KEY_GESTURE_POCKET);
        mPocketPreference.setEnabled(dozeEnabled);

        mProximityWakePreference = (SwitchPreference) findPreference(KEY_PROXIMITY_WAKE);
        mProximityWakePreference.setOnPreferenceChangeListener(mGesturePrefListener);

        mHapticFeedback = (SwitchPreference) findPreference(KEY_HAPTIC_FEEDBACK);
        mHapticFeedback.setOnPreferenceChangeListener(mHapticPrefListener);
    }

    @Override
    public void onResume() {
        super.onResume();
        mHapticFeedback.setChecked(Utils.getIntSystem(getContext(), getActivity().
                getContentResolver(), Utils.AMBIENT_GESTURE_HAPTIC_FEEDBACK, 1) != 0);
        getListView().setPadding(0, 0, 0, 0);
    }

    private boolean enableDoze(boolean enable) {
        return Settings.Secure.putInt(getActivity().getContentResolver(),
                Settings.Secure.DOZE_ENABLED, enable ? 1 : 0);
    }

    private boolean isDozeEnabled() {
        return Settings.Secure.getInt(getActivity().getContentResolver(),
                Settings.Secure.DOZE_ENABLED, 1) != 0;
    }

    private CompoundButton.OnCheckedChangeListener mAmbientDisplayPrefListener =
        new CompoundButton.OnCheckedChangeListener() {
        @Override
        public void onCheckedChanged(CompoundButton compoundButton, boolean enable) {
            if (enableDoze(enable)) {
                mHandwavePreference.setEnabled(enable);
                mPickupPreference.setEnabled(enable);
                mPocketPreference.setEnabled(enable);
                mSwitchBarText.setText(enable ? R.string.switch_bar_on :
                        R.string.switch_bar_off);
            }
        }
    };

    private Preference.OnPreferenceChangeListener mGesturePrefListener =
        new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange(Preference preference, Object newValue) {
            if ((boolean) newValue) {
                final String key = preference.getKey();
                if (KEY_GESTURE_HAND_WAVE.equals(key)) {
                    mProximityWakePreference.setChecked(false);
                } else if (KEY_PROXIMITY_WAKE.equals(key)) {
                    mHandwavePreference.setChecked(false);
                }
            }
            return true;
        }
    };

    private Preference.OnPreferenceChangeListener mHapticPrefListener =
        new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange(Preference preference, Object newValue) {
            final String key = preference.getKey();
            if (KEY_HAPTIC_FEEDBACK.equals(key)) {
                final boolean value = (boolean) newValue;
                Utils.putIntSystem(getContext(), getActivity().getContentResolver(),
                        Utils.AMBIENT_GESTURE_HAPTIC_FEEDBACK, value ? 1 : 0);
                return true;
            }
            return false;
        }
    };
}
