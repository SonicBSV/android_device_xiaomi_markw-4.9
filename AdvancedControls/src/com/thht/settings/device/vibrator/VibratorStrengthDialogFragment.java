package com.thht.settings.device.vibrator;

import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.Context;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.preference.PreferenceManager;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.SeekBar;
import android.widget.TextView;

import com.thht.settings.device.R;
import com.thht.settings.device.helpers.Utils;

public class VibratorStrengthDialogFragment extends DialogFragment implements SeekBar.OnSeekBarChangeListener {
    public static final String FILE_LEVEL_VIB_STRENGTH = "/sys/devices/virtual/timed_output/vibrator/vtg_level";
    // the different layout elements
    private SeekBar mSeekBar;
    private TextView mValueText;
    private Button mPlusOneButton;
    private Button mMinusOneButton;
    private Button mRestoreDefaultButton;

    // control values
    private int mOldStrength;
    private static int mMinValue = 119;
    private static int mMaxValue = 3596;
    private static float mOffset = 3596 / 100f;
    private static String mPreferenceKey;
    private static final int mDuration = 150;

    private static String mFileLevel = FILE_LEVEL_VIB_STRENGTH; // path to kernel node
    private static int mDefaultValue = 2873;

    // vibrator
    private Vibrator mVibrator;

    // get instance of dialog
    public static VibratorStrengthDialogFragment newInstance(String preferenceKey) {
        mPreferenceKey = preferenceKey;

        return new VibratorStrengthDialogFragment();
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        // Use the Builder class for convenient dialog construction
        AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
        LayoutInflater inflater = getActivity().getLayoutInflater();
        View customView = inflater.inflate(R.layout.custom_dialog, null);
        initialise(customView);
        setUpDialog();
        builder.setView(customView);
        builder.setPositiveButton("OK", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
                // set the new values
                setNewState();
            }
        })
                .setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                        // restore old values
                        restoreOldState();
                        mVibrator.cancel();
                    }
                });
        // Create the AlertDialog object and return it
        return builder.create();
    }

    // change value of node with slide of progressbar
    @Override
    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
        setValue(String.valueOf(progress + mMinValue));
        mValueText.setText(Integer.toString(Math.round((progress + mMinValue) / mOffset)) + "%");
    }

    @Override
    public void onStartTrackingTouch(SeekBar seekBar) {

    }

    @Override
    public void onStopTrackingTouch(SeekBar seekBar) {
        if (mVibrator.hasVibrator()) {
            int progress = seekBar.getProgress();
            int amplitude = scale(progress);
            mVibrator.vibrate(VibrationEffect.createOneShot(mDuration, amplitude));
        }
    }

    // setup different layout elements
    private void setUpDialog() {
        // backup existing value
        mOldStrength = Integer.parseInt(getValue());

        // set up seekbar
        mSeekBar.setMax(mMaxValue - mMinValue);
        mSeekBar.setProgress(mOldStrength - mMinValue);
        mSeekBar.setOnSeekBarChangeListener(this);

        // set up text
        mValueText.setText(Integer.toString(Math.round(mOldStrength / mOffset)) + "%");

        // set up buttons
        mPlusOneButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (v.getId() == R.id.plus_one) {
                    singleStepPlus();
                }
            }
        });
        mMinusOneButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (v.getId() == R.id.minus_one) {
                    singleStepMinus();
                }
            }
        });
        mRestoreDefaultButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (v.getId() == R.id.restore_default) {
                    restoreDefault();
                }
            }
        });
    }

    // initialise different layout elements
    private void initialise(View view) {
        mVibrator = (Vibrator) getActivity().getSystemService(Context.VIBRATOR_SERVICE);
        mSeekBar = view.findViewById(R.id.seekBar);
        mValueText = view.findViewById(R.id.current_value);
        mPlusOneButton = view.findViewById(R.id.plus_one);
        mMinusOneButton = view.findViewById(R.id.minus_one);
        mRestoreDefaultButton = view.findViewById(R.id.restore_default);
    }

    // whether the feature is suorted by kernel
    public static boolean isSupported() {
        return Utils.fileWritable(mFileLevel);
    }

    // return value from kernel node
    public static String getValue() {
        return Utils.getFileValue(mFileLevel, String.valueOf(mDefaultValue));
    }

    // change value of kernel node
    public static void setValue(String newValue) {
        Utils.writeValue(mFileLevel, newValue);
    }

    // restore stored value of kernel node 
    public static void restore(Context context, String defaultValue) {
        if (!isSupported()) {
            return;
        }
        String storedValue = PreferenceManager.getDefaultSharedPreferences(context).getString(mPreferenceKey, defaultValue);
        Utils.writeValue(mFileLevel, storedValue);
    }
    
    public static void restore(Context context, String preferenceKey, String defaultValue) {
        if (!isSupported()) {
            return;
        }
        String storedValue = PreferenceManager.getDefaultSharedPreferences(context).getString(preferenceKey, defaultValue);
        Utils.writeValue(mFileLevel, storedValue);
    }

    // restore old state of node
    private void restoreOldState() {
        setValue(String.valueOf(mOldStrength));
    }

    // set new values on dialog close
    private void setNewState() {
        final int value = mSeekBar.getProgress() + mMinValue;
        setValue(String.valueOf(value));
        SharedPreferences.Editor editor = PreferenceManager.getDefaultSharedPreferences(getActivity()).edit();
        editor.putString(mPreferenceKey, String.valueOf(value));
        editor.apply();
    }

    // click action : plus one
    private void singleStepPlus() {
        int currentValue = mSeekBar.getProgress();
        if (currentValue < mMaxValue) {
            mSeekBar.setProgress(currentValue + Math.round(mOffset));
        }
    }

    // click action : minus one
    private void singleStepMinus() {
        int currentValue = mSeekBar.getProgress();
        if (currentValue > mMinValue) {
            mSeekBar.setProgress(currentValue - Math.round(mOffset));
        }
    }

    // click action : restore deafults
    private void restoreDefault() {
        mSeekBar.setProgress(mDefaultValue);
    }

    private int scale(int value) {
        return (int) (127f / 1740f * 3596) - (int) (112f / 15f);
    }
}
