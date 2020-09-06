package constantin.telemetry.core;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.Environment;

import androidx.preference.PreferenceManager;

import java.io.File;

import static android.content.Context.MODE_PRIVATE;

public class TelemetrySettings {
    public static final int SOURCE_TYPE_UDP=0;
    public static final int SOURCE_TYPE_FILE=1;
    public static final int SOURCE_TYPE_ASSETS=2;
    public static final int SOURCE_TYPE_EXTERNAL_DJI=3;

    public static String getT_PLAYBACK_FILENAME(final Context context){
        return context.getSharedPreferences("pref_telemetry",Context.MODE_PRIVATE).
                getString(context.getString(R.string.T_PLAYBACK_FILENAME), context.getString(R.string.T_PLAYBACK_FILENAME_DEFAULT_VALUE));
    }

    @SuppressLint("ApplySharedPref")
    public static void setT_PLAYBACK_FILENAME(final Context context, final String pathAndFilename){
        context.getSharedPreferences("pref_telemetry",Context.MODE_PRIVATE).edit().
                putString(context.getString(R.string.T_PLAYBACK_FILENAME),pathAndFilename).commit();
    }

    @SuppressLint("ApplySharedPref")
    public static void setT_GROUND_RECORDING(final Context context,final boolean enable){
        context.getSharedPreferences("pref_telemetry",Context.MODE_PRIVATE).edit().
                putBoolean(context.getString(R.string.T_GROUND_RECORDING),enable).commit();
    }

    @SuppressLint("ApplySharedPref")
    public static void setT_SOURCE(final Context context,final int type){
        context.getSharedPreferences("pref_telemetry",Context.MODE_PRIVATE).edit().
                putInt(context.getString(R.string.T_SOURCE),type).commit();
    }

    public static int getT_SOURCE(final Context context){
        return context.getSharedPreferences("pref_telemetry",Context.MODE_PRIVATE).
                getInt(context.getString(R.string.T_SOURCE),SOURCE_TYPE_UDP);
    }

    //
    //also create directory if not already existing
    public static String getDirectoryToSaveDataTo(){
        final String ret= Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_MOVIES)+"/FPV_VR/";
        File dir = new File(ret);
        if (!dir.exists()) {
            final boolean mkdirs = dir.mkdirs();
            //System.out.println("mkdirs res"+mkdirs);
        }
        return ret;
    }

    @SuppressLint("ApplySharedPref")
    public static void initializePreferences(final Context context,final boolean readAgain){
        PreferenceManager.setDefaultValues(context,"pref_telemetry",MODE_PRIVATE,R.xml.pref_telemetry,readAgain);
        final SharedPreferences pref_telemetry=context.getSharedPreferences("pref_telemetry", MODE_PRIVATE);
        final String filename=pref_telemetry.getString(context.getString(R.string.T_PLAYBACK_FILENAME),context.getString(R.string.T_PLAYBACK_FILENAME_DEFAULT_VALUE));
        if(filename.equals(context.getString(R.string.T_PLAYBACK_FILENAME_DEFAULT_VALUE))){
            pref_telemetry.edit().putString(context.getString(R.string.T_PLAYBACK_FILENAME),
                    getDirectoryToSaveDataTo()+"filename.format").commit();
        }
    }
}
