package com.thht.settings.device.kcal;

import com.thht.settings.device.helpers.StaticMembers;
import android.util.Log;

public class KcalPresets {

    private static final String[] rgb = {"237 237 237", "225 245 255", "250 250 235", "240 240 240", "255 250 251", "250 250 255", "250 250 255", "236 248 255", "255 255 255", "253 246 243", "226 215 255"};
    private static final String[] minRgb = {"35", "35", "35", "35", "35", "35", "35", "35", "35", "35", "35"};
    private static final String[] satIntensity = {"33", "40", "27", "33", "66", "60", "33", "50", "65", "50", "40"};
    private static final String[] scrHue = {"0", "0", "1520", "0", "1526", "0", "0", "0", "0", "0", "10"};
    private static final String[] scrValue = {"127", "127", "112", "127", "136", "117", "117", "123", "114", "123", "119"};
    private static final String[] scrContrast = {"127", "127", "132", "127", "132", "136", "136", "130", "136", "130", "132"};

    public static void setValue(String value) {
        int index = Integer.parseInt(value);
        KcalRGBDialogFragment.setValue(StaticMembers.FILE_LEVEL_KCAL_RGB, rgb[index]);
        KcalExtrasDialogFragment.setValue(StaticMembers.FILE_LEVEL_KCAL_MIN, minRgb[index], 0);
        KcalExtrasDialogFragment.setValue(StaticMembers.FILE_LEVEL_KCAL_SAT, satIntensity[index], 225);
        KcalExtrasDialogFragment.setValue(StaticMembers.FILE_LEVEL_KCAL_HUE, scrHue[index], 0);
        KcalExtrasDialogFragment.setValue(StaticMembers.FILE_LEVEL_KCAL_VAL, scrValue[index], 128);
        KcalExtrasDialogFragment.setValue(StaticMembers.FILE_LEVEL_KCAL_CONT, scrContrast[index], 128);
    }

    enum Presets {
        DEFAULT, VERSION1, VERSION2, VERSION3, TRILUMINOUS, DEEPBW, DEEPND, COOLAMOLED, EXTREMEAMOLED, WARMAMOLED, HYBRIDMAMBA;

        public static Presets toEnum(int index) {
            switch (index) {
                case 0:
                    return DEFAULT;
                case 1:
                    return VERSION1;
                case 2:
                    return VERSION2;
                case 3:
                    return VERSION3;
                case 4:
                    return TRILUMINOUS;
                case 5:
                    return DEEPBW;
                case 6:
                    return DEEPND;
                case 7:
                    return COOLAMOLED;
                case 8:
                    return EXTREMEAMOLED;
                case 9:
                    return WARMAMOLED;
                case 10:
                    return HYBRIDMAMBA;
            }
            return null;
        }
    }
}
