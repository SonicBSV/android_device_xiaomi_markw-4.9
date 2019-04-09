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
package com.thht.settings.device;

import android.app.ActivityManager;
import android.content.Context;
import android.content.om.IOverlayManager;
import android.os.Bundle;
import android.os.ServiceManager;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.app.AppCompatDelegate;
import com.android.internal.statusbar.ThemeAccentUtils;

public class DeviceSettings extends AppCompatActivity {

    // Use dark mode for now, will make this user selectable later
    //static {
    //    AppCompatDelegate.setDefaultNightMode(
    //            AppCompatDelegate.MODE_NIGHT_FOLLOW_SYSTEM);
    //}

    @Override
    public void onCreate(Bundle savedInstanceState) {
        
        // Set theme
        IOverlayManager overlayManager;
        overlayManager = IOverlayManager.Stub.asInterface(
                ServiceManager.getService(Context.OVERLAY_SERVICE));
        boolean useDarkTheme = ThemeAccentUtils.isUsingDarkTheme(
                overlayManager, ActivityManager.getCurrentUser());
        //boolean useBlackTheme = ThemeAccentUtils.isUsingBlackTheme(
        //        overlayManager, ActivityManager.getCurrentUser());
        //if (useDarkTheme || useBlackTheme) {
        if (useDarkTheme) {
            AppCompatDelegate.setDefaultNightMode(
                AppCompatDelegate.MODE_NIGHT_YES);
        }
        
        super.onCreate(savedInstanceState);
        setContentView(R.layout.device_settings);
        getFragmentManager().beginTransaction().
                replace(R.id.container, new CustomPreferenceFragment()).
                commit();
    }

}

