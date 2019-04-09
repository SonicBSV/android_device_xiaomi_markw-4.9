package com.thht.settings.device;

import android.app.IntentService;
import android.content.Intent;
import android.preference.PreferenceManager;

import com.thht.settings.device.helpers.StaticMembers;
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

    }
}

