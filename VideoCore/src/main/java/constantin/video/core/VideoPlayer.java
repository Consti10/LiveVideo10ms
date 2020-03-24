package constantin.video.core;

import android.content.Context;
import android.os.Environment;
import android.util.Log;
import android.view.Surface;

import java.io.File;
import java.util.Timer;
import java.util.TimerTask;

import constantin.video.core.VideoNative.INativeVideoParamsChanged;
import constantin.video.core.VideoNative.VideoNative;


//Convenient wrapper around the native functions from VideoNative
public class VideoPlayer implements INativeVideoParamsChanged {
    private static final String TAG="VideoPlayer";
    private final long nativeVideoPlayer;
    private IVideoParamsChanged mVideoParamsChanged;
    private final Context context;
    private Timer timer;

    //Setup as much as possible without creating the decoder
    //It is not recommended to change Settings in the Shared Preferences after instantiating the Video Player
    public VideoPlayer(final Context context,final IVideoParamsChanged iVideoParamsChanged){
        this.mVideoParamsChanged =iVideoParamsChanged;
        this.context=context;
        nativeVideoPlayer= VideoNative.initialize(context,getDirectoryToSaveDataTo());
    }

    public void setIVideoParamsChanged(final IVideoParamsChanged iVideoParamsChanged){
        mVideoParamsChanged=iVideoParamsChanged;
    }

    //We cannot initialize the Decoder until we have SPS and PPS data -
    //when streaming this data will be available at some point in future
    //Therefore we don't allocate the MediaCodec resources yet
    //They will be allocated in the native feedDecoder thread once possible
    //public void prepare(Surface surface){
    //    VideoNative.nativeAddConsumers(nativeVideoPlayer,surface);
    //}

    //Depending on the selected Settings, this starts either
    //a) Receiving RAW over UDP
    //b) Receiving RTP over UDP
    //c) Receiving Data from a resource file (Assets)
    //d) Receiving Data from a file in the phone file system
    public void addAndStartReceiver(Surface surface){
        //VideoNative.nativeStartReceiver(nativeVideoPlayer,context.getAssets());
        VideoNative.nativeStart(nativeVideoPlayer,surface,context.getAssets());
        if(mVideoParamsChanged !=null){
            final INativeVideoParamsChanged interfaceVideoParamsChanged=this;
            Log.d(TAG,"Starting timer");
            //The timer initiates the callback(s), but if no data has changed they are not called (and the timer does almost no work)
            //TODO: proper queue, but how to do synchronization in java ndk ?!
            timer=new Timer();
            timer.schedule(new TimerTask() {
                @Override
                public void run() {
                    VideoNative.nativeCallBack(interfaceVideoParamsChanged,nativeVideoPlayer);
                }
            },0,500);
        }
    }

    //Stop the Receiver
    //Stop the Decoder
    //Free resources
    public void stopAndRemovePlayerReceiver(){
        if(mVideoParamsChanged !=null){
            timer.cancel();
            timer.purge();
            Log.d(TAG,"Stopped timer");
        }
        //VideoNative.nativeStopReceiver(nativeVideoPlayer);
        //release();
        VideoNative.nativeStop(nativeVideoPlayer);
    }

    //Call this to free resources
    //private void release(){
        //VideoNative.nativeRemoveConsumers(nativeVideoPlayer);
    //}

    public long getNativeInstance(){
        return nativeVideoPlayer;
    }

    /**
     * called by native code via NDK
     */
    @Override
    @SuppressWarnings({"UnusedDeclaration"})
    public void onVideoRatioChanged(int videoW, int videoH) {
        if(mVideoParamsChanged !=null){
            mVideoParamsChanged.onVideoRatioChanged(videoW,videoH);
        }
        //System.out.println("Video W and H"+videoW+","+videoH);
    }

    /**
     * called by native code via NDK
     */
    @Override
    @SuppressWarnings({"UnusedDeclaration"})
    public void onDecodingInfoChanged(float currentFPS, float currentKiloBitsPerSecond, float avgParsingTime_ms, float avgWaitForInputBTime_ms, float avgDecodingTime_ms,
                                      int nNALU,int nNALUSFeeded) {
        final DecodingInfo decodingInfo=new DecodingInfo(currentFPS,currentKiloBitsPerSecond,avgParsingTime_ms,avgWaitForInputBTime_ms,avgDecodingTime_ms,nNALU,nNALUSFeeded);
        if(mVideoParamsChanged !=null){
            mVideoParamsChanged.onDecodingInfoChanged(decodingInfo);
        }
        Log.d(TAG,"onDecodingInfoChanged"+decodingInfo.toString());
    }

    @Override
    protected void finalize() throws Throwable {
        try {
            VideoNative.finalize(nativeVideoPlayer);
        } finally {
            super.finalize();
        }
    }

    private static String getDirectoryToSaveDataTo(){
        final String ret= Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DCIM)+"/FPV_VR/Video/";
        File dir = new File(ret);
        if (!dir.exists()) {
            final boolean mkdirs = dir.mkdirs();
            //System.out.println("mkdirs res"+mkdirs);
        }
        return ret;
    }



}
