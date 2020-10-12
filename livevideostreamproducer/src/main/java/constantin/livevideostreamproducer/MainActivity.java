package constantin.livevideostreamproducer;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.preference.PreferenceManager;

import android.Manifest;
import android.annotation.SuppressLint;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.EditText;
import android.widget.Toast;

import com.mapzen.prefsplusx.DefaultPreferenceFragment;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

//Request all permissions
//start AVideoStream activity if button is clicked

public class MainActivity extends AppCompatActivity {
    private static final String[] REQUIRED_PERMISSION_LIST = new String[]{
            Manifest.permission.WRITE_EXTERNAL_STORAGE,
            Manifest.permission.ACCESS_FINE_LOCATION,
            Manifest.permission.CAMERA,
    };
    private final List<String> missingPermission = new ArrayList<>();
    private static final int REQUEST_PERMISSION_CODE = 12345;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);


        checkAndRequestPermissions();
        final Context context=this;
        findViewById(R.id.start_stream).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent=new Intent().setClass(context,AVideoStream.class);
                startActivity(intent);
            }
        });
        final EditText editText=findViewById(R.id.ip_address_edit_text);
        editText.setText(getIpAddress(context));
        editText.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {
            }

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
            }

            @SuppressLint("ApplySharedPref")
            @Override
            public void afterTextChanged(Editable s) {
                writeIpAddress(context,s.toString());
                Toast.makeText(context,"Set IP "+s.toString(),Toast.LENGTH_SHORT).show();
            }
        });
        findViewById(R.id.autofill_button).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                final List<String> ips=IPResolver.findAllPossibleClientIps(context);
                if(ips.size()!=0){
                    final CharSequence[] items =new CharSequence[ips.size()];
                    for(int i=0;i<ips.size();i++){
                        items[i]=ips.get(i)+"";
                    }
                    AlertDialog.Builder builder = new AlertDialog.Builder(MainActivity.this);
                    builder.setTitle("Select destination IP");
                    builder.setItems(items, new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int item) {
                            final String ip=ips.get(item);
                            editText.setText(ip);
                            writeIpAddress(context,ip);
                            Toast.makeText(context,"Set ip to "+ip,Toast.LENGTH_LONG).show();
                        }
                    });
                    AlertDialog alert = builder.create();
                    alert.show();
                }else{
                    Toast.makeText(context,"Cannot autofill ip",Toast.LENGTH_LONG).show();
                }
            }
        });
    }

    @SuppressLint("ApplySharedPref")
    private static void writeIpAddress(final Context context, final String ip){
        PreferenceManager.getDefaultSharedPreferences(context).edit().
                putString(context.getString(R.string.KEY_SP_UDP_IP),ip).commit();
    }
    @SuppressLint("ApplySharedPref")
    private static String getIpAddress(final Context context){
        return PreferenceManager.getDefaultSharedPreferences(context).
                getString(context.getString(R.string.KEY_SP_UDP_IP),"192.168.1.1");
    }

    //Permissions stuff
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
                                           @NonNull String[] permissions, @NonNull int[] grantResults) {
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

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.settings, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.action_settings:
                // User chose the "Settings" item, show the app settings UI...
                Intent i=new Intent();
                i.setClass(this,ASettings.class);
                startActivity(i);
                return true;
            default:
                // If we got here, the user's action was not recognized.
                // Invoke the superclass to handle it.
                return super.onOptionsItemSelected(item);

        }
    }

}
