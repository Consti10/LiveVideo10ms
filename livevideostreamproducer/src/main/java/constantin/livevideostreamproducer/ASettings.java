package constantin.livevideostreamproducer;

import android.annotation.SuppressLint;
import android.content.Context;
import android.os.Bundle;

import androidx.appcompat.app.AppCompatActivity;
import androidx.preference.Preference;
import androidx.preference.PreferenceFragmentCompat;
import androidx.preference.PreferenceManager;

public class ASettings extends AppCompatActivity {
    public static final String EXTRA_KEY="SHOW_ADVANCED_SETTINGS";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        final MSettingsFragment fragment=new MSettingsFragment();
        getSupportFragmentManager().beginTransaction()
                .replace(android.R.id.content, fragment)
                .commit();
    }

    public static int getSTREAM_MODE(final Context context){
        return PreferenceManager.getDefaultSharedPreferences(context).
                getInt(context.getString(R.string.KEY_STREAM_MODE),0);
    }

    @SuppressLint("ApplySharedPref")
    public static void setSP_UDP_IP(final Context context, final String ip){
        PreferenceManager.getDefaultSharedPreferences(context).edit().
                putString(context.getString(R.string.KEY_SP_UDP_IP),ip).commit();
    }
    public static String getSP_UDP_IP(final Context context){
        return PreferenceManager.getDefaultSharedPreferences(context).
                getString(context.getString(R.string.KEY_SP_UDP_IP),"192.168.1.1");
    }

    public static int getSP_ENCODER_BITRATE_MBITS(final Context context){
        return PreferenceManager.getDefaultSharedPreferences(context).
                getInt(context.getString(R.string.KEY_SP_ENCODER_BITRATE_MBITS),5);
    }

    public static class MSettingsFragment extends PreferenceFragmentCompat {

        @Override
        public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
            PreferenceManager preferenceManager=getPreferenceManager();
            //preferenceManager.setSharedPreferencesName("pref_stream");
            addPreferencesFromResource(R.xml.pref_stream);
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
