package constantin.telemetry.core;


import android.content.Context;
import android.location.Location;

import androidx.appcompat.app.AppCompatActivity;
import androidx.lifecycle.Lifecycle;
import androidx.lifecycle.LifecycleObserver;
import androidx.lifecycle.LifecycleOwner;
import androidx.lifecycle.OnLifecycleEvent;

/*
 * Pattern native -> ndk:
 * createInstance() creates a new native instance and return the pointer to it
 * This pointer is hold by java
 * All other native functions take a pointer to a native instance
 */

/**
 * Optionally: Also handles Sending telemetry data
 */

@SuppressWarnings("WeakerAccess")
public class TelemetryReceiver implements HomeLocation.IHomeLocationChanged, LifecycleObserver {
    static {
        System.loadLibrary("TelemetryReceiver");
    }
    private static native long createInstance(Context context,String groundRecordingDirectory,long externalGroundRecorder,long externalFileReader);
    private static native void deleteInstance(long instance);
    private static native void startReceiving(long instance,Context context);
    private static native void stopReceiving(long instance,Context context);
    //set values from java
    private static native void setDecodingInfo(long instance,float currentFPS, float currentKiloBitsPerSecond,float avgParsingTime_ms,
                                               float avgWaitForInputBTime_ms,float avgDecodingTime_ms);
    protected static native void setHomeLocation(long instance,double latitude,double longitude,double attitude);
    //For debugging/testing
    private static native String getStatisticsAsString(long testRecN);
    private static native String getEZWBDataAsString(long testRecN);
    private static native String getTelemetryDataAsString(long testRecN);
    private static native boolean anyTelemetryDataReceived(long testRecN);
    private static native boolean isEZWBIpAvailable(long testRecN);
    private static native String getEZWBIPAdress(long testRecN);
    private static native boolean receivingEZWBButCannotParse(long testRecN);
    //new
    protected static native void setDJIValues(long instance,double Latitude_dDeg,double Longitude_dDeg,float AltitudeX_m,float Roll_Deg,float Pitch_Deg,
                                            float SpeedClimb_KPH,float SpeedGround_KPH,int SatsInUse,float Heading_Deg);
    protected static native void setDJIBatteryValues(long instance,float BatteryPack_P,float BatteryPack_A,float BatteryPack_V);
    protected static native void setDJISignalQuality(long instance,int qualityUpPercentage,int qualityDownPercentage);
    protected static native void setDJIFunctionButtonClicked(long instance);

    protected final long nativeInstance;
    protected final Context context;


    //Only use with AppCombatActivity for lifecycle listener
    //receives data in between onPause()<-->onResume()
    public TelemetryReceiver(final AppCompatActivity parent, long externalGroundRecorder, long externalFileReader){
        context=parent;
        nativeInstance=createInstance(parent,TelemetrySettings.getDirectoryToSaveDataTo(),externalGroundRecorder,externalFileReader);
        //Home location handles lifecycle itself
        final HomeLocation mHomeLocation = new HomeLocation(parent, this);
        parent.getLifecycle().addObserver(this);
    }

    @OnLifecycleEvent(Lifecycle.Event.ON_START)
    private void startReceiving(){
        startReceiving(nativeInstance,context);
    }

    @OnLifecycleEvent(Lifecycle.Event.ON_STOP)
    private void stopReceiving(){
        stopReceiving(nativeInstance,context);
    }

    public final long getNativeInstance(){
        return nativeInstance;
    }

    public boolean anyTelemetryDataReceived(){
        return anyTelemetryDataReceived(nativeInstance);
    }
    public String getStatisticsAsString(){
        return getStatisticsAsString(nativeInstance);
    }
    public String getEZWBDataAsString(){
        return getEZWBDataAsString(nativeInstance);
    }
    public boolean isEZWBIpAvailable(){
        return isEZWBIpAvailable(nativeInstance);
    }
    public String getEZWBIPAdress(){
        return getEZWBIPAdress(nativeInstance);
    }
    public String getTelemetryDataAsString(){
        return getTelemetryDataAsString(nativeInstance);
    }
    public boolean receivingEZWBButCannotParse(){
        return receivingEZWBButCannotParse(nativeInstance);
    }

    public void setDecodingInfo(float currentFPS, float currentKiloBitsPerSecond,float avgParsingTime_ms,float avgWaitForInputBTime_ms,float avgDecodingTime_ms) {
        setDecodingInfo(nativeInstance,currentFPS,currentKiloBitsPerSecond,avgParsingTime_ms,avgWaitForInputBTime_ms,avgDecodingTime_ms);
    }

    @Override
    public void onHomeLocationChanged(Location location) {
        setHomeLocation(nativeInstance,location.getLatitude(),location.getLongitude(),location.getAltitude());
    }

    @Override
    protected void finalize() throws Throwable {
        try {
            deleteInstance(nativeInstance);
        } finally {
            super.finalize();
        }
    }



}
