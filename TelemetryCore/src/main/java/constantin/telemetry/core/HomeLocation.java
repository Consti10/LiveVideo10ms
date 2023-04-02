package constantin.telemetry.core;


import android.annotation.SuppressLint;
import android.content.Context;
import android.content.SharedPreferences;
import android.location.Location;
import android.os.Looper;

import androidx.annotation.NonNull;
import androidx.lifecycle.Lifecycle;
import androidx.lifecycle.LifecycleObserver;
import androidx.lifecycle.LifecycleOwner;
import androidx.lifecycle.OnLifecycleEvent;

import com.google.android.gms.location.FusedLocationProviderClient;
import com.google.android.gms.location.LocationCallback;
import com.google.android.gms.location.LocationRequest;
import com.google.android.gms.location.LocationResult;
import com.google.android.gms.location.LocationServices;

import static android.content.Context.MODE_PRIVATE;

/**
 * After resume(), onHomeLocationChanged() is called successive until ether pause() or a
 * accuracy of SUFFICIENT_ACCURACY_M is achieved
 */

public class HomeLocation implements LifecycleObserver {
    private final FusedLocationProviderClient mFusedLocationClient;
    private final LocationCallback mLocationCallback;
    private final IHomeLocationChanged mIHomeLocationChanged;
    private final int SUFFICIENT_ACCURACY_M=10;
    private final boolean OSD_ORIGIN_POSITION_ANDROID;

    public <T extends Context & LifecycleOwner> HomeLocation(final T t, IHomeLocationChanged homeLocationChanged){
        t.getLifecycle().addObserver(this);
        mIHomeLocationChanged=homeLocationChanged;
        mFusedLocationClient = LocationServices.getFusedLocationProviderClient(t);
        mLocationCallback = new LocationCallback() {
            @Override
            public void onLocationResult(LocationResult locationResult) {
                super.onLocationResult(locationResult);
                final Location mCurrentLocation = locationResult.getLastLocation();
                newLocation(mCurrentLocation);
                if(mCurrentLocation.hasAccuracy()&&mCurrentLocation.getAccuracy()<SUFFICIENT_ACCURACY_M){
                    mFusedLocationClient.removeLocationUpdates(mLocationCallback);
                }
            }
        };
        final SharedPreferences pref_telemetry = t.getSharedPreferences("pref_telemetry",MODE_PRIVATE);
        OSD_ORIGIN_POSITION_ANDROID=pref_telemetry.getBoolean(t.getString(R.string.T_ORIGIN_POSITION_ANDROID),false);
    }


    @SuppressLint("MissingPermission")
    @OnLifecycleEvent(Lifecycle.Event.ON_RESUME)
    private void resume(){
        if(!OSD_ORIGIN_POSITION_ANDROID)return;
        LocationRequest mLocationRequest = LocationRequest.create();
        mLocationRequest.setInterval(500); // in ms
        mLocationRequest.setFastestInterval(500);
        mLocationRequest.setPriority(LocationRequest.PRIORITY_HIGH_ACCURACY);
        if(mFusedLocationClient!=null){
            mFusedLocationClient.requestLocationUpdates(mLocationRequest, mLocationCallback, Looper.myLooper());
        }
    }

    @OnLifecycleEvent(Lifecycle.Event.ON_PAUSE)
    private void pause(){
        if(mFusedLocationClient!=null){
            mFusedLocationClient.removeLocationUpdates(mLocationCallback);
            mFusedLocationClient.flushLocations();
        }
    }

    private void newLocation(@NonNull final Location location){
        printLocation(location);
        if(mIHomeLocationChanged !=null){
            mIHomeLocationChanged.onHomeLocationChanged(location);
        }
    }

    private void printLocation(final Location mCurrentHomeLocation){
        System.out.println("Lat:"+mCurrentHomeLocation.getLatitude()+" Lon:"+mCurrentHomeLocation.getLongitude()+" Alt:"
                +mCurrentHomeLocation.getAltitude()+" Accuracy:"+mCurrentHomeLocation.getAccuracy()+" Provider:"
                +mCurrentHomeLocation.getProvider());
    }

    public interface IHomeLocationChanged {
        void onHomeLocationChanged(Location location);
    }
}
