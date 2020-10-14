package constantin.video.transmitter;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;

import androidx.appcompat.app.AppCompatActivity;
import androidx.preference.PreferenceFragmentCompat;
import androidx.preference.PreferenceManager;

import constantin.video.core.R;


// Video stream provider settings
public class AVideoTransmitterSettings extends AppCompatActivity {
    public static final String EXTRA_KEY="SHOW_ADVANCED_SETTINGS";
    public static final String SHARED_PREFERENCES_NAME="pref_video_transmitter";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        final MSettingsFragment fragment=new MSettingsFragment();
        getSupportFragmentManager().beginTransaction()
                .replace(android.R.id.content, fragment)
                .commit();
    }

    public static SharedPreferences getSharedPreferences(final Context c){
        return c.getSharedPreferences(SHARED_PREFERENCES_NAME,MODE_PRIVATE);
    }

    public static int getSTREAM_MODE(final Context context){
        return getSharedPreferences(context).
                getInt(context.getString(R.string.VIDEO_TRANSMITTER_STREAM_MODE),0);
    }

    @SuppressLint("ApplySharedPref")
    public static void setSP_UDP_IP(final Context context, final String ip){
        getSharedPreferences(context).edit().
                putString(context.getString(R.string.VIDEO_TRANSMITTER_UDP_IP),ip).commit();
    }
    public static String getSP_UDP_IP(final Context context){
        return getSharedPreferences(context).
                getString(context.getString(R.string.VIDEO_TRANSMITTER_UDP_IP),"192.168.1.1");
    }

    public static int getSP_ENCODER_BITRATE_MBITS(final Context context){
        return getSharedPreferences(context).
                getInt(context.getString(R.string.VIDEO_TRANSMITTER_ENCODER_BITRATE_MBITS),5);
    }

    public static class MSettingsFragment extends PreferenceFragmentCompat {

        @Override
        public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
            PreferenceManager preferenceManager=getPreferenceManager();
            preferenceManager.setSharedPreferencesName(SHARED_PREFERENCES_NAME);
            addPreferencesFromResource(R.xml.pref_video_transmitter);
        }

        @Override
        public void onActivityCreated(Bundle savedInstanceState){
            super.onActivityCreated(savedInstanceState);
        }

        @Override
        public void onResume(){
            super.onResume();
        }

        @Override
        public void onPause(){
            super.onPause();
        }
    }
}
