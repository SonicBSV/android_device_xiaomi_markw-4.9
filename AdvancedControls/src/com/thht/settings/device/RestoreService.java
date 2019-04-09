package com.thht.settings.device;

import android.app.IntentService;
import android.content.Intent;
import android.preference.PreferenceManager;

import com.thht.settings.device.helpers.StaticMembers;
import com.thht.settings.device.kcal.KcalExtrasDialogFragment;
import com.thht.settings.device.kcal.KcalPresets;
import com.thht.settings.device.kcal.KcalRGBDialogFragment;
import com.thht.settings.device.torch.TorchBrightnessDialogFragment;
import com.thht.settings.device.vibrator.VibratorStrengthDialogFragment;

public class RestoreService extends IntentService {

    private static final String TAG = "RestoreService";

    public RestoreService() {
        super(RestoreService.class.getName());
    }

    @Override
    protected void onHandleIntent(Intent intent) {

        // restore vibrator strength
        VibratorStrengthDialogFragment.restore(this,
                StaticMembers.KEY_VIB_STRENGTH,
                String.valueOf(StaticMembers.DEFAULT_VALUE_VIB_STRENGTH));
        // restore white torch brightness
        TorchBrightnessDialogFragment.restore(StaticMembers.FILE_LEVEL_TORCH_WHITE,
                this,
                StaticMembers.KEY_WHITE_TORCH_BRIGHTNESS,
                String.valueOf(StaticMembers.DEFAULT_VALUE_TORCH_WHITE));
        // restore yellow torch brightness
        TorchBrightnessDialogFragment.restore(StaticMembers.FILE_LEVEL_TORCH_YELLOW,
                this,
                StaticMembers.KEY_YELLOW_TORCH_BRIGHTNESS,
                String.valueOf(StaticMembers.DEFAULT_VALUE_TORCH_YELLOW));

        // restore wakeup fix
        Boolean shouldFixSlowWakeUp = intent.getExtras().getBoolean(StaticMembers.KEY_SLOW_WAKEUP_FIX, false);
        CustomPreferenceFragment.setSlowWakeupFix(shouldFixSlowWakeUp);

        // restore kcal
        Boolean shouldRestorePreset = intent.getExtras().getBoolean(StaticMembers.KEY_KCAL_PRESETS, false);

        if (shouldRestorePreset) {
            String kcalPresetsValue = PreferenceManager.getDefaultSharedPreferences(this).
                    getString(StaticMembers.KEY_KCAL_PRESETS_LIST, "0");
            KcalPresets.setValue(kcalPresetsValue);
        } else {
            KcalExtrasDialogFragment.restore(StaticMembers.FILE_LEVEL_KCAL_MIN, this, StaticMembers.KEY_KCAL_RGB_MIN, StaticMembers.DEFAULT_VALUE_KCAL_MIN);
            KcalExtrasDialogFragment.restore(StaticMembers.FILE_LEVEL_KCAL_SAT, this, StaticMembers.KEY_KCAL_SAT_INTENSITY, StaticMembers.DEFAULT_VALUE_KCAL_SAT);
            KcalExtrasDialogFragment.restore(StaticMembers.FILE_LEVEL_KCAL_HUE, this, StaticMembers.KEY_KCAL_SCR_HUE, StaticMembers.DEFAULT_VALUE_KCAL_HUE);
            KcalExtrasDialogFragment.restore(StaticMembers.FILE_LEVEL_KCAL_VAL, this, StaticMembers.KEY_KCAL_SCR_VAL, StaticMembers.DEFAULT_VALUE_KCAL_VAL);
            KcalExtrasDialogFragment.restore(StaticMembers.FILE_LEVEL_KCAL_CONT, this, StaticMembers.KEY_KCAL_SCR_CONT, StaticMembers.DEFAULT_VALUE_KCAL_CONT);
            KcalRGBDialogFragment.restoreRGBAfterBoot(this);
        }

    }
}

