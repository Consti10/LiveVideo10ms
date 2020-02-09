package constantin.video.core;

import android.content.Context;
import android.os.Environment;
import android.view.Surface;

import java.io.File;

import constantin.video.core.VideoNative.NativeInterfaceVideoParamsChanged;
import constantin.video.core.VideoNative.VideoNative;
import constantin.video.core.IVideoParamsChanged;
import constantin.video.core.DecodingInfo;


//Convenient wrapper around the native functions from VideoNative
public class VideoPlayer implements NativeInterfaceVideoParamsChanged {
    private final long nativeVideoPlayer;
    private final IVideoParamsChanged videoParamsChanged;
    private final Context context;

    //Setup as much as possible without creating the decoder
    //It is not recommended to change Settings in the Shared Preferences after instantiating the Video Player
    public VideoPlayer(final Context context,final IVideoParamsChanged iVideoParamsChanged){
        this.videoParamsChanged=iVideoParamsChanged;
        this.context=context;
        nativeVideoPlayer= VideoNative.initialize(this,context,getDirectoryToSaveDataTo());
    }

    //We cannot initialize the Decoder until we have SPS and PPS data -
    //when streaming this data will be available at some point in future
    //Therefore we don't allocate the MediaCodec resources yet
    //They will be allocated in the native feedDecoder thread once possible
    public void prepare(Surface surface){
        VideoNative.nativeAddConsumers(nativeVideoPlayer,surface);
    }

    //Depending on the selected Settings, this starts either
    //a) Receiving RAW over UDP
    //b) Receiving RTP over UDP
    //c) Receiving Data from a resource file (Assets)
    //d) Receiving Data from a file in the phone file system
    public void addAndStartReceiver(){
        VideoNative.nativeStartReceiver(nativeVideoPlayer,context.getAssets());
    }

    //Stop the Receiver
    //Stop the Decoder
    //Free resources
    public void stopAndRemovePlayerReceiver(){
        VideoNative.nativeStopReceiver(nativeVideoPlayer);
        release();
    }

    //Call this to free resources
    private void release(){
        VideoNative.nativeRemoveConsumers(nativeVideoPlayer);
    }

    public long getNativeInstance(){
        return nativeVideoPlayer;
    }

    @Override
    public void onVideoRatioChanged(int videoW, int videoH) {
        if(videoParamsChanged!=null){
            videoParamsChanged.onVideoRatioChanged(videoW,videoH);
        }
        //System.out.println("Video W and H"+videoW+","+videoH);
    }

    @Override
    public void onDecodingInfoChanged(float currentFPS, float currentKiloBitsPerSecond, float avgParsingTime_ms, float avgWaitForInputBTime_ms, float avgDecodingTime_ms,
                                      int nNALU,int nNALUSFeeded) {
        if(videoParamsChanged!=null){
            final DecodingInfo decodingInfo=new DecodingInfo(currentFPS,currentKiloBitsPerSecond,avgParsingTime_ms,avgWaitForInputBTime_ms,avgDecodingTime_ms,nNALU,nNALUSFeeded);
            videoParamsChanged.onDecodingInfoChanged(decodingInfo);
        }
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
