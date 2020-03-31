package constantin.video.core;

import android.content.Context;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import constantin.video.core.VideoPlayer.VideoPlayer;

/**
 * In contrast to VideoPlayerSurfaceTexture, here the lifecycle is tied to the SurfaceHolder backing a android SurfaceView
 * Since the surface is created / destroyed when pausing / resuming the app there is no need
 * for a Lifecycle Observer or in other words the LifecycleObserver is replaced by the SurfaceHolder.Callback
 */
public class VideoPlayerSurfaceHolder implements SurfaceHolder.Callback{

    private final VideoPlayer videoPlayer;

    public VideoPlayerSurfaceHolder(final Context context, final SurfaceView surfaceView, final IVideoParamsChanged iVideoParamsChanged){
        videoPlayer=new VideoPlayer(context,iVideoParamsChanged);
        surfaceView.getHolder().addCallback(this);
    }

    public long GetExternalGroundRecorder(){
        return VideoPlayer.nativeGetExternalGroundRecorder(videoPlayer.getNativeInstance());
        //return 0;
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
