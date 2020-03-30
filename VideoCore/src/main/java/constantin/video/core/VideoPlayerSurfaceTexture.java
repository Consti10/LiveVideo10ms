package constantin.video.core;


import android.graphics.SurfaceTexture;
import android.view.Surface;
import androidx.appcompat.app.AppCompatActivity;
import androidx.lifecycle.Lifecycle;
import androidx.lifecycle.LifecycleObserver;
import androidx.lifecycle.OnLifecycleEvent;

import constantin.video.core.VideoPlayer.VideoSettings;
import constantin.video.core.VideoPlayer.VideoPlayer;


/**
 * When using a OpenGL texture & android SurfaceTexture for video playback
 * This player properly pauses() / resumes() video playback with the android lifecycle
 * and also allows setting the SurfaceTexture once it becomes available via the ISurfaceTextureAvailable callback
 */
public class VideoPlayerSurfaceTexture implements LifecycleObserver, ISurfaceTextureAvailable {
    //VideoPlayer from https://github.com/Consti10/LiveVideo10ms
    private final VideoPlayer videoPlayer;
    //Used for Android lifecycle and executing callback on the UI thread
    private final AppCompatActivity parent;
    //null in the beginning, becomes valid in the future via onSurfaceTextureAvailable
    //(Constructor of Surface takes SurfaceTexture)
    private Surface mVideoSurface;

    public VideoPlayerSurfaceTexture(final AppCompatActivity parent, final IVideoParamsChanged vpc, final String assetsFilename){
        VideoSettings.setVS_SOURCE(parent, VideoPlayer.VS_SOURCE.ASSETS);
        VideoSettings.setVS_ASSETS_FILENAME_TEST_ONLY(parent,assetsFilename);
        VideoSettings.setVS_FILE_ONLY_LIMIT_FPS(parent,40);
        this.parent=parent;
        videoPlayer=new VideoPlayer(parent,vpc);
        parent.getLifecycle().addObserver(this);
    }

    public VideoPlayerSurfaceTexture(final AppCompatActivity parent){
        this.parent=parent;
        videoPlayer=new VideoPlayer(parent,null);
        parent.getLifecycle().addObserver(this);
    }

    public long GetExternalGroundRecorder(){
        return VideoPlayer.nativeGetExternalGroundRecorder(videoPlayer.getNativeInstance());
    }

    public void setIVideoParamsChanged(final IVideoParamsChanged vpc){
        videoPlayer.setIVideoParamsChanged(vpc);
    }

    //This one is called by the OpenGL Thread !
    @Override
    public void onSurfaceTextureAvailable(final SurfaceTexture surfaceTexture) {
        final VideoPlayerSurfaceTexture reference=this;
        //To avoid race conditions always start and stop the Video player on the UI thread
        parent.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                //If the callback gets called after the application was paused / destroyed
                //(which is possible because the callback was originally not invoked on the UI thread )
                //only create the Surface for later use. The next onResume() event will re-start the video
                mVideoSurface=new Surface(surfaceTexture);
                if(reference.parent.getLifecycle().getCurrentState().isAtLeast(Lifecycle.State.RESUMED)){
                    videoPlayer.addAndStartDecoderReceiver(mVideoSurface);
                }
            }
        });
    }

    @OnLifecycleEvent(Lifecycle.Event.ON_RESUME)
    private void resume(){
        if(mVideoSurface!=null){
            videoPlayer.addAndStartDecoderReceiver(mVideoSurface);
        }
    }

    @OnLifecycleEvent(Lifecycle.Event.ON_PAUSE)
    private void pause(){
        if(mVideoSurface!=null){
            videoPlayer.stopAndRemoveReceiverDecoder();
        }
    }

    @OnLifecycleEvent(Lifecycle.Event.ON_DESTROY)
    private void destroy(){
        if(mVideoSurface!=null){
            mVideoSurface.release();
        }
    }


}
