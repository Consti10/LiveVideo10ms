package videotester;

import android.content.Context;
import android.content.Intent;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import constantin.lowlagvideo.DecodingInfo;
import constantin.lowlagvideo.External.AspectFrameLayout;
import constantin.lowlagvideo.FileVideoReceiver;
import constantin.lowlagvideo.IVideoParamsChanged;
import constantin.lowlagvideo.VideoPlayer;


public class VideoActivity extends AppCompatActivity implements SurfaceHolder.Callback, IVideoParamsChanged {

    private Context mContext;
    private AspectFrameLayout mAspectFrameLayout;
    private VideoPlayer mVideoPlayer;
    private int selected;
    private DecodingInfo mDecodingInfo;

    private final String[] ASSETS_FILE_NAMES={"testVideo.h264","rpi.h264","Recording_360.h264","o2.h264"};


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        final Intent intent = getIntent();
        selected = intent.getIntExtra(MainActivity.INNTENT_EXTRA_NAME, 0);
        System.out.println("Selected:"+selected);
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
        mVideoPlayer.addAndStartFileReceiver(mContext, FileVideoReceiver.REC_MODE.ASSETS,ASSETS_FILE_NAMES[selected]);
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        mVideoPlayer.stopAndRemoveFileReceiver();
        mVideoPlayer.release();
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
