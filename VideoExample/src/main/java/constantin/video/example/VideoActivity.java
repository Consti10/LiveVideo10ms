package constantin.video.example;

import android.content.Context;
import android.content.Intent;

import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import constantin.video.core.DecodingInfo;
import constantin.video.core.External.AspectFrameLayout;
import constantin.video.core.FileVideoReceiver;
import constantin.video.core.IVideoParamsChanged;
import constantin.video.core.VideoPlayer;


public class VideoActivity extends AppCompatActivity implements SurfaceHolder.Callback, IVideoParamsChanged {

    private Context mContext;
    private AspectFrameLayout mAspectFrameLayout;
    private VideoPlayer mVideoPlayer;

    private int selectedMode;
    private int selectedTestFile;

    private DecodingInfo mDecodingInfo;

    private static final String[] ASSETS_TEST_VIDEO_FILE_NAMES ={"testVideo.h264","rpi.h264","Recording_360.h264","o2.h264"};


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        final Intent intent = getIntent();
        selectedMode=intent.getIntExtra(MainActivity.INNTENT_EXTRA_VIDEO_MODE,0);
        selectedTestFile = intent.getIntExtra(MainActivity.INNTENT_EXTRA_VIDEO_TEST_FILE, 0);
        mContext=this;
        setContentView(R.layout.activity_video);
        //
        SurfaceView mSurfaceView = findViewById(R.id.sv_video);
        mSurfaceView.getHolder().addCallback(this);
        mAspectFrameLayout =  findViewById(R.id.afl_video);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        mVideoPlayer=new VideoPlayer(this);
        mVideoPlayer.prepare(holder.getSurface(),null);
        if(selectedMode==0){
            mVideoPlayer.addAndStartReceiver(mContext, FileVideoReceiver.REC_MODE.ASSETS, ASSETS_TEST_VIDEO_FILE_NAMES[selectedTestFile]);
        }else{
            mVideoPlayer.addAndStartReceiver(5600,true);
        }
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
    }


    public final DecodingInfo getDecodingInfo(){
        return mDecodingInfo;
    }

}
