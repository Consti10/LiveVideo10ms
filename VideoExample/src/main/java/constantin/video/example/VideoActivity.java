package constantin.video.example;

import android.content.Context;
import android.content.Intent;

import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import constantin.video.core.DecodingInfo;
import constantin.video.core.External.AspectFrameLayout;
import constantin.video.core.IVideoParamsChanged;
import constantin.video.core.VideoPlayer;


public class VideoActivity extends AppCompatActivity implements SurfaceHolder.Callback, IVideoParamsChanged {

    private Context mContext;
    private AspectFrameLayout mAspectFrameLayout;
    private VideoPlayer mVideoPlayer;

    private DecodingInfo mDecodingInfo;
    long lastLogMS=System.currentTimeMillis();


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        final Intent intent = getIntent();
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
