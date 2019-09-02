package constantin.video.example;

import android.Manifest;
import android.annotation.SuppressLint;
import android.content.Context;
import android.content.Intent;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import constantin.video.core.AVideoSettings;
import constantin.video.core.VideoNative.VideoNative;

import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.Spinner;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class MainActivity extends AppCompatActivity implements View.OnClickListener {
    Spinner spinnerVideoTestFileFromAssets;
    Context context;
    private static final String[] REQUIRED_PERMISSION_LIST = new String[]{
            Manifest.permission.WRITE_EXTERNAL_STORAGE,
            Manifest.permission.READ_EXTERNAL_STORAGE
    };
    private final List<String> missingPermission = new ArrayList<>();
    private static final int REQUEST_PERMISSION_CODE = 12345;

    private static final String[] ASSETS_TEST_VIDEO_FILE_NAMES ={"testVideo.h264","rpi.h264","Recording_360.h264","o2.h264"};



    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        checkAndRequestPermissions();
        VideoNative.initializePreferences(this,false);
        context=this;

        spinnerVideoTestFileFromAssets =findViewById(R.id.s_videoFileSelector);
        ArrayAdapter<CharSequence> adapter2 = ArrayAdapter.createFromResource(this,
                R.array.video_test_files, android.R.layout.simple_spinner_item);
        adapter2.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spinnerVideoTestFileFromAssets.setAdapter(adapter2);
        //
        Button startVideoActivity=findViewById(R.id.b_startVideoActivity);
        startVideoActivity.setOnClickListener(new View.OnClickListener() {
            @SuppressLint("ApplySharedPref")
            @Override
            public void onClick(View v) {
                final int selectedTestFile= spinnerVideoTestFileFromAssets.getSelectedItemPosition();
                SharedPreferences preferences=getSharedPreferences("pref_video",MODE_PRIVATE);
                preferences.edit().putString(getString(R.string.VS_ASSETS_FILENAME_TEST_ONLY),ASSETS_TEST_VIDEO_FILE_NAMES[selectedTestFile]).commit();
                Intent intentVideoActivity = new Intent();
                intentVideoActivity.setClass(context, VideoActivity.class);
                startActivity(intentVideoActivity);
            }
        });
        Button startSettingsActivity=findViewById(R.id.b_startSettingsActivity);
        startSettingsActivity.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                final Intent intent=new Intent();
                intent.setClass(context, AVideoSettings.class);
                intent.putExtra(AVideoSettings.EXTRA_KEY,true);
                startActivity(intent);
            }
        });
    }

    @Override
    protected void onResume() {
        if(getSharedPreferences("pref_video",MODE_PRIVATE).getInt(getString(R.string.VS_SOURCE),0)==2){
            spinnerVideoTestFileFromAssets.setEnabled(true);
        }else{
            spinnerVideoTestFileFromAssets.setEnabled(false);
        }

        super.onResume();
    }

    @Override
    protected void onPause() {
        super.onPause();
    }

    @Override
    public void onClick(View v) {
    }

    private void checkAndRequestPermissions(){
        missingPermission.clear();
        for (String eachPermission : REQUIRED_PERMISSION_LIST) {
            if (ContextCompat.checkSelfPermission(this, eachPermission) != PackageManager.PERMISSION_GRANTED) {
                missingPermission.add(eachPermission);
            }
        }
        if (!missingPermission.isEmpty()) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                final String[] asArray=missingPermission.toArray(new String[0]);
                Log.d("PermissionManager","Request: "+ Arrays.toString(asArray));
                ActivityCompat.requestPermissions(this, asArray, REQUEST_PERMISSION_CODE);
            }
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode,
                                           @NonNull String permissions[], @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);

        // Check for granted permission and remove from missing list
        if (requestCode == REQUEST_PERMISSION_CODE) {
            for (int i = grantResults.length - 1; i >= 0; i--) {
                if (grantResults[i] == PackageManager.PERMISSION_GRANTED) {
                    missingPermission.remove(permissions[i]);
                }
            }
        }
        if (!missingPermission.isEmpty()) {
            checkAndRequestPermissions();
        }

    }
}
