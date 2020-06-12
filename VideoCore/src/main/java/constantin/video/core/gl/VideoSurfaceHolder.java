package constantin.video.core.gl;

import android.graphics.SurfaceTexture;
import android.opengl.GLES20;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.Trace;
import android.view.Surface;

import androidx.appcompat.app.AppCompatActivity;
import androidx.lifecycle.Lifecycle;
import androidx.lifecycle.LifecycleObserver;
import androidx.lifecycle.OnLifecycleEvent;

// Inspired by SurfaceHolder and SurfaceHolder.Callback() this class holds a android Surface backed by an OpenGL SurfaceTexture
// It simplifies starting / stopping a Video player in sync with the applications lifecycle
// (e.g. pause player in onPause , resume player in onResume() )
// But also handles the edge case when onResume() is called but the OpenGL thread has not
// created the SurfaceTexture yet
// Make sure to call addCallBack in your activity's onCreate
public class VideoSurfaceHolder implements LifecycleObserver {
    private final AppCompatActivity parent;
    private ISurfaceAvailable iSurfaceAvailable;
    //These members are created on the OpenGL thread
    private int mGLTextureVideo;
    private SurfaceTexture surfaceTexture;
    //This surface is created/ destroyed on the UI thread
    private Surface surface;

    public VideoSurfaceHolder(final AppCompatActivity activity){
        this.parent =activity;
        activity.getLifecycle().addObserver(this);
    }

    // Not calling this in onCreate will result in app crash
    public void setCallBack(final ISurfaceAvailable iSurfaceAvailable){
        this.iSurfaceAvailable=iSurfaceAvailable;
    }

    public int getTextureId(){
        return mGLTextureVideo;
    }

    public SurfaceTexture getSurfaceTexture(){
        return surfaceTexture;
    }

    // Call this on the OpenGl thread, e.g. in onSurfaceCreated()
    public void createSurfaceTextureGL(){
        int[] videoTexture=new int[1];
        GLES20.glGenTextures(1, videoTexture, 0);

        mGLTextureVideo = videoTexture[0];
        surfaceTexture=new SurfaceTexture(mGLTextureVideo,false);

        Looper.prepare();
        Looper looper=Looper.myLooper();

        final Handler handler= new Handler(looper, null) {
            @Override
            public void handleMessage(Message msg) {
                Trace.beginSection("Callback onFrameAvailable2");
                long latency=System.nanoTime()-surfaceTexture.getTimestamp();
                System.out.println("Latency is "+(latency/1000)/1000.0f);
                Trace.endSection();
            }
        };
        Message m=Message.obtain();

        surfaceTexture.setOnFrameAvailableListener(new SurfaceTexture.OnFrameAvailableListener() {
            @Override
            public void onFrameAvailable(SurfaceTexture surfaceTexture) {
                // the latency of the callback is truly horrible and cam be between 1 and 100 ms
                // DANG: I have to update texture first before using the timestamp to measure latency
                Trace.beginSection("Callback onFrameAvailable");
                long latency=System.nanoTime()-surfaceTexture.getTimestamp();
                System.out.println("Latency is "+(latency/1000)/1000.0f);
                Trace.endSection();
            }
        },null);

        parent.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                //If the callback gets called after the application was paused / destroyed
                //(which is possible because the callback was originally not invoked on the UI thread )
                //only create the Surface for later use. The next onResume() event will re-start the video
                surface=new Surface(surfaceTexture);
                if(VideoSurfaceHolder.this.parent.getLifecycle().getCurrentState().isAtLeast(Lifecycle.State.RESUMED)){
                    iSurfaceAvailable.XSurfaceCreated(surfaceTexture,surface);
                }
            }
        });
    }

    @OnLifecycleEvent(Lifecycle.Event.ON_RESUME)
    private void resume(){
        if(iSurfaceAvailable==null){
            throw new RuntimeException("iSurfaceAvailable==null");
        }
        if(surface!=null){
            iSurfaceAvailable.XSurfaceCreated(surfaceTexture,surface);
        }
    }

    @OnLifecycleEvent(Lifecycle.Event.ON_PAUSE)
    private void pause(){
        if(surface!=null){
            iSurfaceAvailable.XSurfaceDestroyed();
        }
    }

    @OnLifecycleEvent(Lifecycle.Event.ON_DESTROY)
    private void destroy(){
        if(surface!=null){
            surface.release();
        }
    }

}
