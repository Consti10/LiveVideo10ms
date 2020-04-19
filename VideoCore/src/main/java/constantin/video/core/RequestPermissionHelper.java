package constantin.video.core;

import android.content.DialogInterface;
import android.content.pm.PackageManager;
import android.util.Log;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

// if any of the declared permissions are not granted, when calling checkAndRequestPermissions() they are requested.
// When also forwarding onRequestPermissionsResult() they are requested again until granted
// When the user denies the permissions , on the second time a Alert dialog is shown before requesting the permissions
public class RequestPermissionHelper implements ActivityCompat.OnRequestPermissionsResultCallback{
    private static final String TAG="RequestPermissionHelper";
    private final String[] REQUIRED_PERMISSION_LIST;
    private final List<String> missingPermission = new ArrayList<>();
    private static final int REQUEST_PERMISSION_CODE = 12345;
    private AppCompatActivity activity;
    private int nRequests=0;

    public RequestPermissionHelper(final String[] requiredPermissionsList){
        REQUIRED_PERMISSION_LIST=requiredPermissionsList;
    }

    /**
     * Call this on onCreate
     */
    public void checkAndRequestPermissions(final AppCompatActivity activity){
        this.activity=activity;
        missingPermission.clear();
        for (final String permission : REQUIRED_PERMISSION_LIST) {
            if (ContextCompat.checkSelfPermission(activity,permission) != PackageManager.PERMISSION_GRANTED) {
                //if(ActivityCompat.shouldShowRequestPermissionRationale(activity,permission)){
                    //Toast.makeText(activity,"App won't work without required permissions",Toast.LENGTH_LONG).show();
                //}
                missingPermission.add(permission);
            }
        }
        if (!missingPermission.isEmpty()) {
            nRequests++;
            if(nRequests==1){
                // First time just request permissions
                requestMissingPermissions();
            }else if(nRequests==2 || nRequests==3){
                // Second and third time use alert dialog to show severity
                AlertDialog.Builder builder = new AlertDialog.Builder(activity);
                builder.setCancelable(false);
                builder.setMessage("Permissions are essential for app");
                builder.setPositiveButton("Okay", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                        requestMissingPermissions();
                    }
                });
                AlertDialog dialog = builder.create();
                dialog.show();
            }else{
                // If we land here something went wrong - crash so I see it in the console
                // I do not expect that to happen
                Toast.makeText(activity,"Essential permissions not granted",Toast.LENGTH_LONG).show();
                throw new RuntimeException(TAG+"Permissions not granted");
            }
        }
    }

    // Call this if missingPermissions is not empty
    private void requestMissingPermissions(){
        final String[] asArray=missingPermission.toArray(new String[0]);
        Log.d(TAG,"Request: "+ Arrays.toString(asArray));
        ActivityCompat.requestPermissions(activity, asArray, REQUEST_PERMISSION_CODE);
    }

    /**
     * Override your activity and forward
     * Unfortunately there is no 'setCallback' functionality - overriding is mandatory
     */
    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        // Check for granted permission and remove from missing list
        Log.d(TAG,"onRequestPermissionsResult");
        if (requestCode == REQUEST_PERMISSION_CODE) {
            for (int i = grantResults.length - 1; i >= 0; i--) {
                if (grantResults[i] == PackageManager.PERMISSION_GRANTED) {
                    missingPermission.remove(permissions[i]);
                }
            }
        }
        if (!missingPermission.isEmpty()) {
            checkAndRequestPermissions(activity);
        }
    }
}
