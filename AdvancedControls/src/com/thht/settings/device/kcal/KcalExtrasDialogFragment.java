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
import com.thht.settings.device.helpers.Utils;

public class KcalExtrasDialogFragment extends DialogFragment implements SeekBar.OnSeekBarChangeListener {
    // the different layout elements
    private SeekBar mSeekBar;
    private TextView mValueText;
    private Button mPlusOneButton;
    private Button mMinusOneButton;
    private Button mRestoreDefaultButton;

    // control values
    private int mOldStrength;
    private static int mMinValue;
    private static int mMaxValue;
    private static int mIndex;
    private static int mOffset;
    private static String mPreferenceKey;

    private static String mFileLevel; // path to kernel node
    private static String mDefaultValue;

    // get instance of dialog
    public static KcalExtrasDialogFragment newInstance(String fileLevel,
                                                       String defaultValue,
                                                       int minValue,
                                                       int maxValue,
                                                       int offSet,
                                                       String preferenceKey) {
        mFileLevel = fileLevel;
        mDefaultValue = defaultValue;
        mMinValue = minValue;
        mMaxValue = maxValue;
        mOffset = offSet;
        mPreferenceKey = preferenceKey;

        return new KcalExtrasDialogFragment();
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
        setValue(value);
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
        mOldStrength = Integer.parseInt(getValue());
        // set up seekbar
        mSeekBar.setMax(mMaxValue - mMinValue);
        mSeekBar.setProgress(mOldStrength - mMinValue);
        mSeekBar.setOnSeekBarChangeListener(this);

        // set up text
        mValueText.setText(String.valueOf(mOldStrength));

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

    // whether the feature is suorted by kernel
    public static boolean isSupported() {
        return Utils.fileWritable(mFileLevel);
    }

    // return value from kernel node
    public static String getValue() {
        int value = Integer.parseInt(Utils.getFileValue(mFileLevel, mDefaultValue));
        return String.valueOf(translate(value, true));
    }

    // change value of kernel node
    public static void setValue(String newValue) {
        String value = String.valueOf(translate(Integer.parseInt(newValue), false));
        Utils.writeValue(mFileLevel, value);
    }

    public static void setValue(String fileName, String newValue) {
        String value = String.valueOf(translate(Integer.parseInt(newValue), false));
        Utils.writeValue(fileName, value);
    }
    
    public static void setValue(String fileName, String newValue, int offset) {
        mOffset = offset;
        String value = String.valueOf(translate(Integer.parseInt(newValue), false));
        Utils.writeValue(fileName, value);
    }

    // restore stored value of kernel node
    public static void restore(String fileLevel, Context context, String defaultValue) {
        mFileLevel = fileLevel;
        if (!isSupported()) {
            return;
        }
        String storedValue = PreferenceManager.getDefaultSharedPreferences(context).getString(mPreferenceKey, defaultValue);
        String value = String.valueOf(translate(Integer.parseInt(storedValue), false));
        Utils.writeValue(fileLevel, value);
    }
    
    // restore stored value of kernel node
    public static void restore(String fileLevel, Context context, String preferenceKey, String defaultValue) {
        mFileLevel = fileLevel;
        if (!isSupported()) {
            return;
        }
        String storedValue = PreferenceManager.getDefaultSharedPreferences(context).getString(preferenceKey, defaultValue);
        String value = String.valueOf(translate(Integer.parseInt(storedValue), false));
        Utils.writeValue(fileLevel, value);
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
        int defaultValue = Integer.parseInt(mDefaultValue);
        mSeekBar.setProgress(defaultValue);
    }

    // scale value
    private static int translate(int value, boolean read) {
        if (!read)
            return value + mOffset;
        else
            return value - mOffset;
    }
}
