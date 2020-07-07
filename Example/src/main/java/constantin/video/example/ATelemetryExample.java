package constantin.video.example;

import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.opengl.EGL14;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import constantin.telemetry.core.ASettingsTelemetry;
import constantin.telemetry.core.RequestPermissionHelper;
import constantin.telemetry.core.TelemetrySettings;
import constantin.telemetry.core.TestReceiverTelemetry;

public class ATelemetryExample extends AppCompatActivity {

    private final RequestPermissionHelper requestPermissionHelper=new RequestPermissionHelper(new String[]{
            Manifest.permission.WRITE_EXTERNAL_STORAGE,
            Manifest.permission.READ_EXTERNAL_STORAGE,
            Manifest.permission.ACCESS_FINE_LOCATION
    });

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_telemetry);
        requestPermissionHelper.checkAndRequestPermissions(this);
        TelemetrySettings.initializePreferences(this,false);
        Button button=findViewById(R.id.button);
        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                final Intent intent=new Intent();
                intent.putExtra(ASettingsTelemetry.EXTRA_KEY,true);
                intent.setClass(getApplicationContext(), ASettingsTelemetry.class);
                //i.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TASK);
                startActivity(intent);
            }
        });
        TextView tv1 = findViewById(R.id.textView);
        TextView tv2 = findViewById(R.id.textView2);
        TextView tv3 = findViewById(R.id.textView3);
        TestReceiverTelemetry testReceiverTelemetry = new TestReceiverTelemetry(this);
        testReceiverTelemetry.setViews( tv1, tv2, tv3);
    }

    @Override
    protected void onResume(){
        super.onResume();
    }

    @Override
    protected void onPause(){
        super.onPause();
    }


    @Override
    public void onRequestPermissionsResult(int requestCode,
                                           @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        requestPermissionHelper.onRequestPermissionsResult(requestCode,permissions,grantResults);
    }
}
