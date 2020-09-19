package constantin.telemetry.core;

import android.content.SharedPreferences;
import android.os.Bundle;

import androidx.appcompat.app.AppCompatActivity;
import androidx.preference.Preference;
import androidx.preference.PreferenceFragmentCompat;
import androidx.preference.PreferenceManager;


public class ASettingsTelemetry extends AppCompatActivity {
    public static final String EXTRA_KEY="SHOW_ADVANCED_SETTINGS";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        final Bundle bundle=getIntent().getExtras();
        final MSettingsFragment fragment=new MSettingsFragment();
        if(bundle!=null){
            fragment.showAdvanced=bundle.getBoolean(EXTRA_KEY,false);
        }
        getSupportFragmentManager().beginTransaction()
                .replace(android.R.id.content, fragment)
                .commit();
    }

    public static class MSettingsFragment extends PreferenceFragmentCompat implements SharedPreferences.OnSharedPreferenceChangeListener{
        public boolean showAdvanced=false;

        @Override
        public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
            PreferenceManager preferenceManager=getPreferenceManager();
            preferenceManager.setSharedPreferencesName("pref_telemetry");
            addPreferencesFromResource(R.xml.pref_telemetry);
            //Log.d("LOL",""+rootKey);
            if(showAdvanced){
                Preference p1=findPreference(getString(R.string.T_PLAYBACK_FILENAME));
                Preference p2=findPreference(getString(R.string.T_SOURCE));
                Preference p3=findPreference(getString(R.string.T_GROUND_RECORDING));
                p1.setEnabled(true);
                p2.setEnabled(true);
                p3.setEnabled(true);
            }
        }

        @Override
        public void onResume(){
            super.onResume();
            getPreferenceScreen().getSharedPreferences().registerOnSharedPreferenceChangeListener(this);
            enableOrDisablePreferences_TelemetryProtocol(getPreferenceScreen().getSharedPreferences());
        }

        @Override
        public void onPause(){
            super.onPause();
            getPreferenceScreen().getSharedPreferences().unregisterOnSharedPreferenceChangeListener(this);
        }

        @Override
        public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
            if(key.contentEquals(getActivity().getString(R.string.T_PROTOCOL))){
                enableOrDisablePreferences_TelemetryProtocol(sharedPreferences);
            }
        }

        private void enableOrDisablePreferences_TelemetryProtocol(SharedPreferences sharedPreferences){
            final int val= sharedPreferences.getInt(getActivity().getString(R.string.T_PROTOCOL),0);
            Preference LTMPort=findPreference(getActivity().getString(R.string.T_LTMPort));
            Preference MAVLINKPort=findPreference(getActivity().getString(R.string.T_MAVLINKPort));
            Preference SMARTPORTPort=findPreference(getActivity().getString(R.string.T_SMARTPORTPort));
            Preference FRSKYPort=findPreference(getActivity().getString(R.string.T_FRSKYPort));
            LTMPort.setEnabled(val==1);
            MAVLINKPort.setEnabled(val==2);
            SMARTPORTPort.setEnabled(val==3);
            FRSKYPort.setEnabled(val==4);
        }


    }
}
