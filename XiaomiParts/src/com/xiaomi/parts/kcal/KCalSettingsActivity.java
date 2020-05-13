package com.xiaomi.parts.kcal;

import android.app.Activity;
import android.app.Fragment;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;

import com.xiaomi.parts.R;

public class KCalSettingsActivity extends Activity implements Utils {

    private KCalSettings mKCalSettingsFragment;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_kcal);

        Fragment fragment = getFragmentManager().findFragmentById(R.id.fragment_kcal);
        if (fragment == null) {
            mKCalSettingsFragment = new KCalSettings();
            getFragmentManager().beginTransaction()
                    .add(R.id.fragment_kcal, mKCalSettingsFragment)
                    .commit();
        } else {
            mKCalSettingsFragment = (KCalSettings) fragment;
        }
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home:
                finish();
                return true;

            case R.id.action_reset:
                mKCalSettingsFragment.applyValues(RED_DEFAULT + " " +
                        GREEN_DEFAULT + " " +
                        BLUE_DEFAULT + " " +
                        MINIMUM_DEFAULT + " " +
                        SATURATION_DEFAULT + " " +
                        VALUE_DEFAULT + " " +
                        CONTRAST_DEFAULT + " " +
                        HUE_DEFAULT);
                mKCalSettingsFragment.setmGrayscale(GRAYSCALE_DEFAULT);
                mKCalSettingsFragment.setmSetOnBoot(SETONBOOT_DEFAULT);
                return true;

            case R.id.action_preset:
                new PresetDialog().show(getFragmentManager(),
                        KCalSettingsActivity.class.getName(), mKCalSettingsFragment);
                return true;

            default:
                break;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater menuInflater = getMenuInflater();
        menuInflater.inflate(R.menu.menu_reset, menu);
        return true;
    }
}
