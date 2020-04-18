package constantin.video.core.video_player;

import android.content.Context;
import android.content.res.AssetManager;
import android.graphics.SurfaceTexture;
import android.view.Surface;
import android.view.SurfaceHolder;

import androidx.annotation.Nullable;

import java.util.Timer;
import java.util.TimerTask;

import constantin.video.core.DecodingInfo;
import constantin.video.core.IVideoParamsChanged;
import constantin.video.core.gl.ISurfaceAvailable;


/**
 * All the receiving and decoding is done in .cpp
 * Tied to the lifetime of the java instance a .cpp instance is created
 * And the native functions talk to the native instance
 */
public class VideoPlayer implements INativeVideoParamsChanged {
    private static final String TAG="VideoPlayer";
    private final long nativeVideoPlayer;
    private IVideoParamsChanged mVideoParamsChanged;
    private final Context context;
    // This timer is used to then 'call back' the INativeVideoParamsChanged
    private Timer timer;

    //Setup as much as possible without creating the decoder
    //It is not recommended to change Settings in the Shared Preferences after instantiating the Video Player
    public VideoPlayer(final Context context,@Nullable final IVideoParamsChanged iVideoParamsChanged){
        this.mVideoParamsChanged =iVideoParamsChanged;
        this.context=context;
        nativeVideoPlayer= nativeInitialize(context,VideoSettings.getDirectoryToSaveDataTo());
    }

    public void setIVideoParamsChanged(final IVideoParamsChanged iVideoParamsChanged){
        mVideoParamsChanged=iVideoParamsChanged;
    }

    public long getExternalGroundRecorder(){
        return nativeGetExternalGroundRecorder(nativeVideoPlayer);
    }
    public long getExternalFilePlayer(){
        return nativeGetExternalFileReader(nativeVideoPlayer);
    }

    /**
     * Depending on the selected Settings, this starts either
     * a) Receiving RAW over UDP
     * b) Receiving RTP over UDP
     * c) Receiving Data from a resource file (Assets)
     * d) Receiving Data from a file in the phone file system
     * e) External
     */
    public void addAndStartDecoderReceiver(Surface surface){
        nativeStart(nativeVideoPlayer,surface,context.getAssets());
        //The timer initiates the callback(s), but if no data has changed they are not called (and the timer does almost no work)
        //TODO: proper queue, but how to do synchronization in java ndk ?!
        timer=new Timer();
        timer.schedule(new TimerTask() {
            @Override
            public void run() {
                nativeCallBack(VideoPlayer.this,nativeVideoPlayer);
            }
        },0,200);
    }

    /**
     * Stop the Receiver
     *   Stop the Decoder
     *   Free resources
     */
    public void stopAndRemoveReceiverDecoder(){
        timer.cancel();
        timer.purge();
        nativeStop(nativeVideoPlayer);
    }

    /**
     * Configure for use with SurfaceHolder from a SurfaceVew
     * The callback will handle the lifecycle of the Video player
     * @return Callback that should be added to SurfaceView.Holder
     */
    public SurfaceHolder.Callback configure1(){
        return new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(SurfaceHolder holder) {
                addAndStartDecoderReceiver(holder.getSurface());
            }
            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) { }
            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {
                stopAndRemoveReceiverDecoder();
            }
        };
    }

    /**
     * Configure for use with VideoSurfaceHolder (OpenGL)
     * The callback will handle the lifecycle of the video player
     * @return  Callback that should be added to VideoSurfaceHolder
     */
    public ISurfaceAvailable configure2(){
        return new ISurfaceAvailable() {
            @Override
            public void XSurfaceCreated(SurfaceTexture surfaceTexture, Surface surface) {
                addAndStartDecoderReceiver(surface);
            }

            @Override
            public void XSurfaceDestroyed() {
                stopAndRemoveReceiverDecoder();
            }
        };
    }

    public long getNativeInstance(){
        return nativeVideoPlayer;
    }

    // called by native code via NDK
    @Override
    @SuppressWarnings({"UnusedDeclaration"})
    public void onVideoRatioChanged(int videoW, int videoH) {
        if(mVideoParamsChanged !=null){
            mVideoParamsChanged.onVideoRatioChanged(videoW,videoH);
        }
        //System.out.println("Video W and H"+videoW+","+videoH);
    }

    // called by native code via NDK
    @Override
    @SuppressWarnings({"UnusedDeclaration"})
    public void onDecodingInfoChanged(float currentFPS, float currentKiloBitsPerSecond, float avgParsingTime_ms, float avgWaitForInputBTime_ms, float avgDecodingTime_ms,
                                      int nNALU,int nNALUSFeeded) {
        final DecodingInfo decodingInfo=new DecodingInfo(currentFPS,currentKiloBitsPerSecond,avgParsingTime_ms,avgWaitForInputBTime_ms,avgDecodingTime_ms,nNALU,nNALUSFeeded);
        if(mVideoParamsChanged !=null){
            mVideoParamsChanged.onDecodingInfoChanged(decodingInfo);
        }
        //Log.d(TAG,"onDecodingInfoChanged"+decodingInfo.toString());
    }

    @Override
    protected void finalize() throws Throwable {
        try {
            nativeFinalize(nativeVideoPlayer);
        } finally {
            super.finalize();
        }
    }

    //All the native binding(s)
    static {
        System.loadLibrary("VideoNative");
    }
    public static native long nativeInitialize(Context context, String groundRecordingDirectory);
    public static native void nativeFinalize(long nativeVideoPlayer);

    public static native void nativePassNALUData(long nativeInstance,byte[] b,int offset,int size);

    public static native void nativeStart(long nativeInstance, Surface surface, AssetManager assetManager);
    public static native void nativeStop(long nativeInstance);
    //get members or other information. Some might be only usable in between (nativeStart <-> nativeStop)
    public static native String getVideoInfoString(long nativeInstance);
    public static native boolean anyVideoDataReceived(long nativeInstance);
    public static native boolean anyVideoBytesParsedSinceLastCall(long nativeInstance);
    public static native boolean receivingVideoButCannotParse(long nativeInstance);
    public static native long nativeGetExternalGroundRecorder(long nativeInstance);
    public static native long nativeGetExternalFileReader(long nativeInstance);

    //TODO: Use message queue from cpp for performance
    public static native <T extends INativeVideoParamsChanged> void nativeCallBack(T t, long nativeInstance);

}
