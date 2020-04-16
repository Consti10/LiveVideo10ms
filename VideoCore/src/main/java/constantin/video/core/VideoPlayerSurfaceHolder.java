package constantin.video.core;

import android.content.Context;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import constantin.video.core.video_player.VideoPlayer;

public class VideoPlayerSurfaceHolder implements SurfaceHolder.Callback{

    private final VideoPlayer videoPlayer;

    public VideoPlayerSurfaceHolder(final Context context, final SurfaceView surfaceView){
        videoPlayer=new VideoPlayer(context,null);
        surfaceView.getHolder().addCallback(this);
    }

    public long GetExternalGroundRecorder(){
        return VideoPlayer.nativeGetExternalGroundRecorder(videoPlayer.getNativeInstance());
    }
    public void setIVideoParamsChanged(final IVideoParamsChanged vpc){
        videoPlayer.setIVideoParamsChanged(vpc);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        final Surface surface=holder.getSurface();
        videoPlayer.addAndStartDecoderReceiver(surface);
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        //Log.d(TAG, "Video surfaceChanged fmt=" + format + " size=" + width + "x" + height);
        //format 4= rgb565
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        videoPlayer.stopAndRemoveReceiverDecoder();
    }

}
