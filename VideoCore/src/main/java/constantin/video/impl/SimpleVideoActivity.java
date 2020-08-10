package constantin.video.impl;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.widget.ToggleButton;

import constantin.video.core.DecodingInfo;
import constantin.video.core.IVideoParamsChanged;
import constantin.video.core.R;
import constantin.video.core.external.AspectFrameLayout;
import constantin.video.core.video_player.VideoPlayer;

// Most basic implementation of an activity that uses LiveVideo to stream or play back video
// Into an Android Surface View
public class SimpleVideoActivity extends AppCompatActivity implements  IVideoParamsChanged {
    protected SurfaceView surfaceView;
    private AspectFrameLayout aspectFrameLayout;
    private VideoPlayer videoPlayer;
    private TextView textViewStatistics;
    protected DecodingInfo mDecodingInfo;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_simple_video);

        surfaceView=findViewById(R.id.sv_video);

        videoPlayer=new VideoPlayer(this);
        videoPlayer.setIVideoParamsChanged(this);
        surfaceView.getHolder().addCallback(videoPlayer.configure1());

       aspectFrameLayout=findViewById(R.id.afl_video);
       textViewStatistics=findViewById(R.id.tv_decoding_stats);
       final Button toggleButton=findViewById(R.id.tb_show_decoding_info);
       toggleButton.setOnClickListener(new View.OnClickListener() {
           @Override
           public void onClick(View v) {
               textViewStatistics.setVisibility(View.VISIBLE);
               textViewStatistics.bringToFront();
               toggleButton.setVisibility(View.INVISIBLE);
           }
       });
       textViewStatistics.setOnClickListener(new View.OnClickListener() {
           @Override
           public void onClick(View v) {
               textViewStatistics.setVisibility(View.INVISIBLE);
               toggleButton.setVisibility(View.VISIBLE);
           }
       });
    }


    @Override
    public void onVideoRatioChanged(final int videoW,final int videoH) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                aspectFrameLayout.setAspectRatio((double)videoW / videoH);
            }
        });
    }

    @Override
    public void onDecodingInfoChanged(final DecodingInfo decodingInfo) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                textViewStatistics.setText(decodingInfo.toString(true));
            }
        });
        mDecodingInfo=decodingInfo;
    }
}