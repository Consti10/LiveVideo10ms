package constantin.video.core;

import androidx.appcompat.app.AppCompatActivity;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.preference.PreferenceManager;

public class AVideoSettings extends AppCompatActivity {
    public static final String EXTRA_KEY="SHOW_ADVANCED_SETTINGS";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        final Bundle bundle=getIntent().getExtras();
        final MSettingsFragment fragment=new MSettingsFragment();
        if(bundle!=null){
            fragment.showAdvanced=bundle.getBoolean(EXTRA_KEY,false);
        }
        getFragmentManager().beginTransaction()
                .replace(android.R.id.content, fragment)
                .commit();
    }

    public static class MSettingsFragment extends PreferenceFragment{
        public boolean showAdvanced=false;

        @Override
        public void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            PreferenceManager preferenceManager=getPreferenceManager();
            preferenceManager.setSharedPreferencesName("pref_video");
            addPreferencesFromResource(R.xml.pref_video);
            if(showAdvanced){
                Preference p1=findPreference(getString(R.string.VS_PLAYBACK_FILENAME));
                Preference p2=findPreference(getString(R.string.VS_SOURCE));
                Preference p3=findPreference(getString(R.string.VS_ASSETS_FILENAME_TEST_ONLY));
                p1.setEnabled(true);
                p2.setEnabled(true);
                p3.setEnabled(true);
            }
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
