package constantin.video.example;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.util.ArrayMap;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import androidx.appcompat.app.AppCompatActivity;

import com.google.android.gms.tasks.OnSuccessListener;
import com.google.firebase.firestore.DocumentReference;
import com.google.firebase.firestore.FirebaseFirestore;
import com.google.firebase.firestore.SetOptions;
import com.google.firebase.firestore.WriteBatch;

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
            dataMap.put("DATE", Helper.getDate());
            dataMap.put("TIME", Helper.getTime());
            dataMap.put("EMULATOR", Helper.isEmulator());
            System.out.println(Helper.getDeviceName());
            //Add all the settings to differentiate which video was decoded
            dataMap.put(getString(R.string.VS_SOURCE),VS_SOURCE);
            dataMap.put(getString(R.string.VS_ASSETS_FILENAME_TEST_ONLY),VS_SOURCE==VideoNative.VS_SOURCE_ASSETS ?
                    VS_ASSETS_FILENAME_TEST_ONLY : "Unknown");
            dataMap.putAll(mDecodingInfo.toMap());
            WriteBatch writeBatch=db.batch();
            final DocumentReference thisDeviceReference = db.collection("Decoding info").document(Helper.getDeviceName());
            final Map<String,Object> dummyMap=new ArrayMap<>();
            dummyMap.put(Helper.getBuildVersionRelease(),1);
            writeBatch.set(thisDeviceReference,dummyMap,SetOptions.merge());
            final DocumentReference thisDeviceOsNewTestData=thisDeviceReference.collection(Helper.getBuildVersionRelease()).document();
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

}
