package constantin.telemetry.core;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.graphics.Color;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.ColorInt;
import androidx.appcompat.app.AppCompatActivity;
import androidx.lifecycle.Lifecycle;
import androidx.lifecycle.LifecycleObserver;
import androidx.lifecycle.LifecycleOwner;
import androidx.lifecycle.OnLifecycleEvent;

//creates a new thread that -in between onResume() / onPause()
//constantly reads from telemetryReceiver and updates the appropriate ui elements
//if they are not null

public class TestReceiverTelemetry implements Runnable, LifecycleObserver {
    private final AppCompatActivity activity;
    private TextView receivedTelemetryDataTV=null;
    private TextView ezwbForwardDataTV=null;
    private TextView dataAsStringTV=null;
    private final TelemetryReceiver telemetryReceiver;
    private Thread mUpateThread;

    public  TestReceiverTelemetry(final AppCompatActivity parent){
        this.activity=parent;
        telemetryReceiver =new TelemetryReceiver(parent,0,0,0);
        parent.getLifecycle().addObserver(this);
    }

    public void setViews(TextView receivedTelemetryDataTV, TextView ezwbForwardDataTV, TextView telemetryValuesAsString){
        this.receivedTelemetryDataTV=receivedTelemetryDataTV;
        this.ezwbForwardDataTV=ezwbForwardDataTV;
        this.dataAsStringTV=telemetryValuesAsString;
    }

    @OnLifecycleEvent(Lifecycle.Event.ON_RESUME)
    private void startUiUpdates(){
        mUpateThread =new Thread(this);
        mUpateThread.setName("T_TestR");
        mUpateThread.start();
    }

    @OnLifecycleEvent(Lifecycle.Event.ON_PAUSE)
    private void stopUiUpdates(){
        mUpateThread.interrupt();
        try {
            mUpateThread.join();}
        catch (InterruptedException e) {e.printStackTrace();}
    }

    @Override
    public void run() {
        long lastCheckMS = System.currentTimeMillis() - 2*1000;
        while (!Thread.currentThread().isInterrupted()){
            //if any of the TV are not null, we update its content
            //with the corresponding string, and optionally change color
            if(receivedTelemetryDataTV !=null){
                final String s= telemetryReceiver.getStatisticsAsString();
                final int newColor=telemetryReceiver.anyTelemetryDataReceived() ? Color.GREEN : Color.RED;
                updateViewIfStringChanged(receivedTelemetryDataTV,s,true,newColor);
            }
            if(ezwbForwardDataTV!=null){
                final String s= telemetryReceiver.getEZWBDataAsString();
                updateViewIfStringChanged(ezwbForwardDataTV,s,false,0);
            }
            if(dataAsStringTV!=null){
                final String s = telemetryReceiver.getTelemetryDataAsString();
                updateViewIfStringChanged(dataAsStringTV,s,false,0);
            }
            if(telemetryReceiver.isEZWBIpAvailable()){
                onEZWBIpDetected(telemetryReceiver.getEZWBIPAdress());
            }
            //Every N s we check if we are receiving ez-wb data, but cannot parse the data. Probably the user did mix up
            //ezwb-versions
            if(System.currentTimeMillis()- lastCheckMS >=3000){
                final boolean errorEZ_WB= telemetryReceiver.receivingEZWBButCannotParse();
                lastCheckMS =System.currentTimeMillis();
                if(errorEZ_WB){
                    makeToastOnUI("You are receiving ez-wifibroadcast telemetry data, but FPV-VR cannot parse it. Probably you are using" +
                            " app version 1.5 or higher with ez-wb. 1.6 or lower. Please upgrade to ez-wb 2.0. This also allows you to read all useful" +
                            " telemetry data from your EZ-WB rx pi on android.",Toast.LENGTH_SHORT);
                }
            }
            //Refresh every 200ms
            try {Thread.sleep(200);} catch (InterruptedException e) {return;}
        }
    }

    @SuppressLint("ApplySharedPref")
    private void onEZWBIpDetected(String ip){
        //System.out.println("Called from native:"+ip);
        /*if(SJ.EnableAHT(context) && !SJ.UplinkEZWBIpAddress(context).contentEquals(ip)){
            Toaster.makeToast(context,"Head tracking IP address set to:"+ip,true);
            SharedPreferences pref_connect = context.getSharedPreferences("pref_connect", MODE_PRIVATE);
            SharedPreferences.Editor editor=pref_connect.edit();
            editor.putString(context.getString(R.string.UplinkEZWBIpAddress),ip).commit();
        }*/
    }

    //to have as less work on the UI thread and GPU, we check if the content of the string has changed
    //before updating the TV.
    private void updateViewIfStringChanged(final TextView tv, final String newContent,final boolean changeColor,@ColorInt final int newColor){
        final String prev=tv.getText().toString();
        if(!prev.contentEquals(newContent)){
            activity.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    tv.setText(newContent);
                    if(changeColor){
                        if(tv.getCurrentTextColor()!=newColor){
                            tv.setTextColor(newColor);
                        }
                    }
                }
            });
        }
    }

    private void makeToastOnUI(final String s,final int length){
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Toast.makeText(activity,s,length).show();
            }
        });
    }

    public interface EZWBIpAddressDetected{
        void onEZWBIpDetected(String ip);
    }

}
