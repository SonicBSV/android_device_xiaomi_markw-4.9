/*
 * Copyright (C) 2013 The OmniROM Project
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

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.preference.PreferenceManager;
import android.util.Log;

import com.thht.settings.device.dirac.AudioEnhancerService;
import com.thht.settings.device.helpers.StaticMembers;


public class Startup extends BroadcastReceiver {

    private static final String TAG = "AdvancedControls";

    @Override
    public void onReceive(final Context context, final Intent bootintent) {

        // start restore service for kcal, vibrator, torch
        Boolean shouldRestore = PreferenceManager.getDefaultSharedPreferences(context).getBoolean(StaticMembers.KEY_RESTORE_ON_BOOT, false);
        final Boolean shouldRestorePreset = PreferenceManager.getDefaultSharedPreferences(context).getBoolean(StaticMembers.KEY_KCAL_PRESETS, false);
        final Boolean shouldFixSlowWakeup = PreferenceManager.getDefaultSharedPreferences(context).getBoolean(StaticMembers.KEY_SLOW_WAKEUP_FIX, false);
        Log.e(TAG, Boolean.toString(shouldRestore));
        if (bootintent.getAction().equals("android.intent.action.BOOT_COMPLETED") && shouldRestore) {
            new Handler().postDelayed(new Runnable() {
                @Override
                public void run() {
                    Intent in = new Intent(context, RestoreService.class);
                    in.putExtra(StaticMembers.KEY_KCAL_PRESETS, shouldRestorePreset);
                    in.putExtra(StaticMembers.KEY_SLOW_WAKEUP_FIX, shouldFixSlowWakeup);
                    context.startService(in);
                }
            }, 0);
        }
        
        // Start dirac service
        context.startService(new Intent(context, AudioEnhancerService.class));
    }
}
