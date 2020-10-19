package constantin.video.example;

import android.content.Context;
import android.os.Build;
import android.widget.Spinner;

import java.text.SimpleDateFormat;
import java.util.Date;

public class Helper {

    public static String getDate(){
        SimpleDateFormat formatter= new SimpleDateFormat("dd.MM.yyyy");
        Date date = new Date(System.currentTimeMillis());
        return formatter.format(date);
    }

    public static String getTime(){
        SimpleDateFormat formatter= new SimpleDateFormat("HH:mm:ss");
        Date date = new Date(System.currentTimeMillis());
        return formatter.format(date);
    }

    public static boolean isEmulator() {
        return Build.FINGERPRINT.startsWith("generic")
                || Build.FINGERPRINT.startsWith("unknown")
                || Build.MODEL.contains("google_sdk")
                || Build.MODEL.contains("Emulator")
                || Build.MODEL.contains("Android SDK built for x86")
                || Build.MANUFACTURER.contains("Genymotion")
                || (Build.BRAND.startsWith("generic") && Build.DEVICE.startsWith("generic"))
                || "google_sdk".equals(Build.PRODUCT);
    }

    public static String getManufacturerAndDeviceName() {
        String manufacturer = Build.MANUFACTURER;
        String model = Build.MODEL;
        String ret;
        if (model.toLowerCase().startsWith(manufacturer.toLowerCase())) {
            ret=capitalize(model);
        } else {
            ret=capitalize(manufacturer) + " " + model;
        }
        if(ret.length()>0){
            return ret;
        }
        return "Unknown";
    }

    private static String capitalize(String s) {
        if (s == null || s.length() == 0) {
            return "";
        }
        char first = s.charAt(0);
        if (Character.isUpperCase(first)) {
            return s;
        } else {
            return Character.toUpperCase(first) + s.substring(1);
        }
    }

    public static String getBuildVersionRelease(){
        final String ret=Build.VERSION.RELEASE;
        return ret==null ? "Unknown" : ret;
    }
}
