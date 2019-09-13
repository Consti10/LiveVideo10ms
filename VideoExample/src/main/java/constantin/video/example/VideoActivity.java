package constantin.video.example;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Build;
import android.os.Bundle;
import android.util.ArrayMap;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import com.google.android.gms.tasks.OnCompleteListener;
import com.google.android.gms.tasks.OnFailureListener;
import com.google.android.gms.tasks.OnSuccessListener;
import com.google.android.gms.tasks.Task;
import com.google.firebase.firestore.CollectionReference;
import com.google.firebase.firestore.DocumentReference;
import com.google.firebase.firestore.FirebaseFirestore;
import com.google.firebase.firestore.SetOptions;
import com.google.firebase.firestore.WriteBatch;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.HashMap;
import java.util.Map;

import constantin.video.core.DecodingInfo;
import constantin.video.core.External.AspectFrameLayout;
import constantin.video.core.IVideoParamsChanged;
import constantin.video.core.VideoNative.VideoNative;
import constantin.video.core.VideoPlayer;


public class VideoActivity extends AppCompatActivity implements SurfaceHolder.Callback, IVideoParamsChanged {
    private final static String TAG="VIdeoActivity";
    private Context mContext;
    private AspectFrameLayout mAspectFrameLayout;
    private VideoPlayer mVideoPlayer;

    private DecodingInfo mDecodingInfo;
    long lastLogMS=System.currentTimeMillis();
    private int VS_SOURCE;
    private String VS_ASSETS_FILENAME_TEST_ONLY="";


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mContext=this;
        setContentView(R.layout.activity_video);
        //
        SurfaceView mSurfaceView = findViewById(R.id.sv_video);
        mSurfaceView.getHolder().addCallback(this);
        mAspectFrameLayout =  findViewById(R.id.afl_video);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        mVideoPlayer=new VideoPlayer(this,this);
        mVideoPlayer.prepare(holder.getSurface());
        mVideoPlayer.addAndStartReceiver();
        final SharedPreferences preferences=getSharedPreferences("pref_video",MODE_PRIVATE);
        VS_SOURCE=preferences.getInt(getString(R.string.VS_SOURCE),0);
        VS_ASSETS_FILENAME_TEST_ONLY=preferences.getString(getString(R.string.VS_ASSETS_FILENAME_TEST_ONLY),"");
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        mVideoPlayer.stopAndRemovePlayerReceiver();
        mVideoPlayer=null;
    }

    @Override
    public void onVideoRatioChanged(final int videoW,final int videoH) {
        runOnUiThread(new Runnable() {
            public void run() {
                mAspectFrameLayout.setAspectRatio((double) videoW / videoH);
            }
        });
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if(mDecodingInfo!=null && mDecodingInfo.nNALUSFeeded>0){
            final FirebaseFirestore db = FirebaseFirestore.getInstance();
            final Map<String,Object> dataMap=new ArrayMap<>();
            //dataMap.put("Build_MODEL",Build.MODEL);
            //dataMap.put("Build_VERSION_SDK_INT", Build.VERSION.SDK_INT);
            dataMap.put("DATE",getDate());
            dataMap.put("TIME",getTime());
            dataMap.put("EMULATOR",isEmulator());
            System.out.println(getDeviceName());
            //Add all the settings to differentiate which video was decoded
            dataMap.put(getString(R.string.VS_SOURCE),VS_SOURCE);
            dataMap.put(getString(R.string.VS_ASSETS_FILENAME_TEST_ONLY),VS_SOURCE==VideoNative.VS_SOURCE_ASSETS ?
                    VS_ASSETS_FILENAME_TEST_ONLY : "Unknown");
            dataMap.putAll(mDecodingInfo.toMap());
            WriteBatch writeBatch=db.batch();
            final DocumentReference thisDeviceReference = db.collection("Decoding info").document(getDeviceName());
            final Map<String,Object> dummyMap=new ArrayMap<>();
            dummyMap.put(getBuildVersionRelease(),1);
            writeBatch.set(thisDeviceReference,dummyMap);
            final DocumentReference thisDeviceOsNewTestData=thisDeviceReference.collection(getBuildVersionRelease()).document();
            writeBatch.set(thisDeviceOsNewTestData,dataMap);
            writeBatch.commit().addOnSuccessListener(new OnSuccessListener<Void>() {
                @Override
                public void onSuccess(Void aVoid) {
                    Log.d(TAG, "WriteBatch added DocumentSnapshot with id: " + thisDeviceOsNewTestData.getId());
                }
            });
        }
    }

    @Override
    public void onDecodingInfoChanged(DecodingInfo decodingInfo) {
        mDecodingInfo=decodingInfo;
        if(System.currentTimeMillis()-lastLogMS>5*1000){
            System.out.println(mDecodingInfo.toString());
            lastLogMS=System.currentTimeMillis();
        }
    }

    public final DecodingInfo getDecodingInfo(){
        return mDecodingInfo;
    }

    private static String getDate(){
        SimpleDateFormat formatter= new SimpleDateFormat("dd.MM.yyyy");
        Date date = new Date(System.currentTimeMillis());
        return formatter.format(date);
    }
    private static String getTime(){
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
    public static String getDeviceName() {
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
