package com.thht.settings.device.kcal;

import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.Context;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.SeekBar;
import android.widget.TextView;

import com.thht.settings.device.R;
import com.thht.settings.device.helpers.StaticMembers;
import com.thht.settings.device.helpers.Utils;

public class KcalRGBDialogFragment extends DialogFragment implements SeekBar.OnSeekBarChangeListener {
    // public elements
    public static final String FILE_LEVEL_KCAL_RGB = "/sys/devices/platform/kcal_ctrl.0/kcal";
    public static final String DEFAULT_VALUE_KCAL_RGB = "237 237 237";
    // path to kernel node
    private static final String mFileLevel = FILE_LEVEL_KCAL_RGB;
    // default value
    private static final String mDefaultValue = DEFAULT_VALUE_KCAL_RGB;
    private static int mMinValue = 0;
    private static int mMaxValue = 255;
    private static int mIndex;
    private static String mPreferenceKey;
    // the different layout elements
    private SeekBar mSeekBar;
    private TextView mValueText;
    private Button mPlusOneButton;
    private Button mMinusOneButton;
    private Button mRestoreDefaultButton;
    // control values
    private int mOldStrength;

    // get instance of dialog
    public static KcalRGBDialogFragment newInstance(int index,
                                                    String preferenceKey) {
        mIndex = index;
        mPreferenceKey = preferenceKey;

        return new KcalRGBDialogFragment();
    }

    // whether the feature is suorted by kernel
    public static boolean isSupported() {
        return Utils.fileWritable(mFileLevel);
    }

    // return value from kernel node
    public static String getValue() {
        return Utils.getFileValue(mFileLevel, mDefaultValue);
    }

    // change value of kernel node
    public static void setValue(String newValue) {
        Utils.writeValue(mFileLevel, newValue);
    }

    public static void setValue(String fileName, String newValue) {
        Utils.writeValue(fileName, newValue);
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

    // string parser
    private static String[] getIndividualRGB(String rgb) {
        return rgb.split(" ", 3);
    }

    // string parser
    private static String combineIndividualRGB(String[] rgb) {
        return String.join(" ", rgb);
    }

    public static void restoreRGBAfterBoot(Context context) {

        if (!isSupported()) {
            return;
        }

        String[] redStoredValue = getIndividualRGB(
                PreferenceManager.getDefaultSharedPreferences(context).getString(
                        StaticMembers.KEY_KCAL_RGB_RED, StaticMembers.DEFAULT_VALUE_KCAL_RGB));

        String[] greenStoredValue = getIndividualRGB(
                PreferenceManager.getDefaultSharedPreferences(context).getString(
                        StaticMembers.KEY_KCAL_RGB_GREEN, StaticMembers.DEFAULT_VALUE_KCAL_RGB));

        String[] blueStoredValue = getIndividualRGB(
                PreferenceManager.getDefaultSharedPreferences(context).getString(
                        StaticMembers.KEY_KCAL_RGB_BLUE, StaticMembers.DEFAULT_VALUE_KCAL_RGB));

        String[] combinedValue = {redStoredValue[0], greenStoredValue[1], blueStoredValue[2]};
        String finalValue = combineIndividualRGB(combinedValue);

        Utils.writeValue(FILE_LEVEL_KCAL_RGB, finalValue);

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
                    }
                });
        // Create the AlertDialog object and return it
        return builder.create();
    }

    // change value of node with slide of progressbar
    @Override
    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
        String value = String.valueOf(progress + mMinValue);
        String[] rgb = getIndividualRGB(getValue());
        rgb[mIndex] = value;
        String finalValue = combineIndividualRGB(rgb);
        setValue(finalValue);
        mValueText.setText(value);
    }

    @Override
    public void onStartTrackingTouch(SeekBar seekBar) {

    }

    @Override
    public void onStopTrackingTouch(SeekBar seekBar) {

    }

    // setup different layout elements
    private void setUpDialog() {
        // backup existing value
        String value = getValue();
        String[] rgb = getIndividualRGB(value);
        mOldStrength = Integer.parseInt(rgb[mIndex]);

        // set up seekbar
        mSeekBar.setMax(mMaxValue - mMinValue);
        mSeekBar.setProgress(mOldStrength - mMinValue);
        mSeekBar.setOnSeekBarChangeListener(this);

        // set up text
        mValueText.setText(String.valueOf(rgb[mIndex]));

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
        mSeekBar = view.findViewById(R.id.seekBar);
        mValueText = view.findViewById(R.id.current_value);
        mPlusOneButton = view.findViewById(R.id.plus_one);
        mMinusOneButton = view.findViewById(R.id.minus_one);
        mRestoreDefaultButton = view.findViewById(R.id.restore_default);
    }

    // restore old state of node
    private void restoreOldState() {
        String[] rgb = getIndividualRGB(getValue());
        rgb[mIndex] = String.valueOf(mOldStrength);
        setValue(combineIndividualRGB(rgb));
    }

    // set new values on dialog close
    private void setNewState() {
        String value = String.valueOf(mSeekBar.getProgress() + mMinValue);
        String[] rgb = getIndividualRGB(getValue());
        rgb[mIndex] = value;
        String finalValue = combineIndividualRGB(rgb);
        setValue(finalValue);
        SharedPreferences.Editor editor = PreferenceManager.getDefaultSharedPreferences(getActivity()).edit();
        editor.putString(mPreferenceKey, finalValue);
        editor.apply();
    }

    // click action : plus one
    private void singleStepPlus() {
        int currentValue = mSeekBar.getProgress();
        if (currentValue < mMaxValue) {
            mSeekBar.setProgress(currentValue + 1);
        }
    }

    // click action : minus one
    private void singleStepMinus() {
        int currentValue = mSeekBar.getProgress();
        if (currentValue > mMinValue) {
            mSeekBar.setProgress(currentValue - 1);
        }
    }

    // click action : restore deafults
    private void restoreDefault() {
        String[] rgb = getIndividualRGB(mDefaultValue);
        int defaultValue = Integer.parseInt(rgb[mIndex]);
        mSeekBar.setProgress(defaultValue);
    }
}
