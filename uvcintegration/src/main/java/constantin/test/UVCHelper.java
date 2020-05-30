package constantin.test;

import android.content.Intent;
import android.hardware.usb.UsbDevice;
import android.util.Log;
import android.widget.Toast;

import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;

public class UVCHelper {
    public static final int DEVICE_CLASS_ROTG02=239;
    public static final int DEVICE_SUBCLASS_ROTG02=2;
    private static final String android_hardware_usb_action_USB_DEVICE_ATTACHED="android.hardware.usb.action.USB_DEVICE_ATTACHED";

    private static final String TAG="UVCHelper";
    public static ArrayList<UsbDevice> filterFOrUVC(final HashMap<String,UsbDevice> deviceList){
        final ArrayList<UsbDevice> ret=new ArrayList<>();
        for (final String key : deviceList.keySet()) {
            final UsbDevice device = deviceList.get(key);
            assert device!=null;
            if (device.getDeviceClass() == DEVICE_CLASS_ROTG02 && device.getDeviceSubclass() == DEVICE_SUBCLASS_ROTG02) {
                Log.d(TAG, "Found okay device");
                ret.add(device);
            } else {
                Log.d(TAG, "Not UVC cl" + device.getDeviceClass() + " subcl " + device.getDeviceSubclass());
            }
        }
        return ret;
    }

    public static void printDeviceInfo(final HashMap<String,UsbDevice> deviceList){
        Log.d(TAG,"There are "+deviceList.size()+" devices connected");
        for (final String key : deviceList.keySet()) {
            final UsbDevice device = deviceList.get(key);
            assert device!=null;
            Log.d(TAG,"Connected device "+device.getDeviceName()+" class"+device.getDeviceClass()+" subcl"+device.getDeviceSubclass());
        }
    }

    // Returns true if the activity was started via the intent-filter -> action usb device attached
    // declared in the manifest
    public static boolean startedViaIntentFilterActionUSB(final AppCompatActivity activity){
        final Intent intent=activity.getIntent();
        if (intent != null) {
            final String action=intent.getAction();
            if(action!=null && action.contentEquals(android_hardware_usb_action_USB_DEVICE_ATTACHED)){
                return true;
            }
        }
        return false;
    }
}
