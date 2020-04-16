package constantin.video.core.VideoPlayer;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.Environment;
import android.preference.PreferenceManager;

import java.io.File;

import constantin.video.core.R;

import static android.content.Context.MODE_PRIVATE;

//Provides conv
public class VideoSettings {
    public enum VS_SOURCE{UDP,FILE,ASSETS,FFMPEG,EXTERNAL}

    public static boolean PLAYBACK_FLE_EXISTS(final Context context){
        SharedPreferences sharedPreferences=context.getSharedPreferences("pref_video", MODE_PRIVATE);
        final String filename=sharedPreferences.getString(context.getString(R.string.VS_PLAYBACK_FILENAME),"");
        File tempFile = new File(filename);
        return tempFile.exists();
    }

    public static VS_SOURCE getVS_SOURCE(final Context context){
        SharedPreferences sharedPreferences=context.getSharedPreferences("pref_video", MODE_PRIVATE);
        final int val=sharedPreferences.getInt(context.getString(R.string.VS_SOURCE),0);
        VS_SOURCE ret= VS_SOURCE.values()[val];
        sharedPreferences.getInt(context.getString(R.string.VS_SOURCE),0);
        return ret;
    }

    @SuppressLint("ApplySharedPref")
    public static void setVS_SOURCE(final Context context, final VS_SOURCE val){
        SharedPreferences sharedPreferences=context.getSharedPreferences("pref_video", MODE_PRIVATE);
        sharedPreferences.edit().putInt(context.getString(R.string.VS_SOURCE),val.ordinal()).commit();
    }

    @SuppressLint("ApplySharedPref")
    public static void setVS_ASSETS_FILENAME_TEST_ONLY(final Context context, final String filename){
        SharedPreferences sharedPreferences=context.getSharedPreferences("pref_video", MODE_PRIVATE);
        sharedPreferences.edit().putString(context.getString(R.string.VS_ASSETS_FILENAME_TEST_ONLY),filename).commit();
    }
    @SuppressLint("ApplySharedPref")
    public static void setVS_FILE_ONLY_LIMIT_FPS(final Context context, final int limitFPS){
        SharedPreferences sharedPreferences=context.getSharedPreferences("pref_video", MODE_PRIVATE);
        sharedPreferences.edit().putInt(context.getString(R.string.VS_FILE_ONLY_LIMIT_FPS),limitFPS).commit();
    }

    public static String getVS_PLAYBACK_FILENAME(final Context context){
        final String tmp=context.getSharedPreferences("pref_video",Context.MODE_PRIVATE).
                getString(context.getString(R.string.VS_PLAYBACK_FILENAME),context.getString(R.string.VS_PLAYBACK_FILENAME_DEFAULT_VALUE));
        return tmp;
    }

    @SuppressLint("ApplySharedPref")
    public static void setVS_PLAYBACK_FILENAME(final Context context, final String pathAndFilename){
        context.getSharedPreferences("pref_video",Context.MODE_PRIVATE).edit().
                putString(context.getString(R.string.VS_PLAYBACK_FILENAME),pathAndFilename).commit();
    }

    @SuppressLint("ApplySharedPref")
    public static void setVS_GROUND_RECORDING(final Context context,final boolean enable){
        context.getSharedPreferences("pref_video",Context.MODE_PRIVATE).edit().
                putBoolean(context.getString(R.string.VS_GROUND_RECORDING),enable).commit();
    }


    //0=normal
    //1=stereo
    //2=equirectangular 360 sphere
    public static int videoMode(final Context c){
        final SharedPreferences pref_video=c.getSharedPreferences("pref_video",MODE_PRIVATE);
        return pref_video.getInt(c.getString(R.string.VS_VIDEO_VIEW_TYPE),0);
    }

    @SuppressLint("ApplySharedPref")
    public static void initializePreferences(final Context context,final boolean readAgain){
        PreferenceManager.setDefaultValues(context,"pref_video",MODE_PRIVATE,R.xml.pref_video,readAgain);
        final SharedPreferences pref_video=context.getSharedPreferences("pref_video", MODE_PRIVATE);
        final String filename=pref_video.getString(context.getString(R.string.VS_PLAYBACK_FILENAME),context.getString(R.string.VS_PLAYBACK_FILENAME_DEFAULT_VALUE));
        if(filename.equals(context.getString(R.string.VS_PLAYBACK_FILENAME_DEFAULT_VALUE))){
            pref_video.edit().putString(context.getString(R.string.VS_PLAYBACK_FILENAME),
                    Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DCIM)+"/FPV_VR/"+"Video/"+"filename.fpv").commit();
        }
    }

    public static String getDirectoryToSaveDataTo(){
        final String ret= Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DCIM)+"/FPV_VR/VideoTelemetry/";
        File dir = new File(ret);
        if (!dir.exists()) {
            final boolean mkdirs = dir.mkdirs();
            //System.out.println("mkdirs res"+mkdirs);
        }
        return ret;
    }
}
