package constantin.video.impl;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import constantin.video.core.player.DecodingInfo;
import constantin.video.core.player.IVideoParamsChanged;
import constantin.video.core.databinding.ActivitySimpleVideoBinding;
import constantin.video.core.external.AspectFrameLayout;
import constantin.video.core.player.VideoPlayer;

// Most basic implementation of an activity that uses VideoCore to stream or play back video
// Into an Android Surface View
public class SimpleVideoActivity extends AppCompatActivity implements  IVideoParamsChanged {
    private ActivitySimpleVideoBinding binding;
    protected SurfaceView surfaceView;
    private AspectFrameLayout aspectFrameLayout;
    private VideoPlayer videoPlayer;
    private TextView textViewStatistics;
    protected DecodingInfo mDecodingInfo;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        binding = ActivitySimpleVideoBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        surfaceView=binding.svVideo;

        videoPlayer=new VideoPlayer(this);
        videoPlayer.setIVideoParamsChanged(this);
        surfaceView.getHolder().addCallback(videoPlayer.configure1());

       aspectFrameLayout=binding.aflVideo;
       textViewStatistics=binding.tvDecodingStats;
       final Button toggleButton=binding.tbShowDecodingInfo;
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
        mDecodingInfo=decodingInfo;
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                textViewStatistics.setText(decodingInfo.toString(true));
            }
        });
    }
}