package constantin.lowlagvideo;

import android.content.Context;
import android.view.Surface;

import constantin.lowlagvideo.VideoNative.Video;

public class VideoPlayer implements FileVideoReceiver.IVideoDataRaw, Video.IVideoParamsChangedNative {

    private final long nativeVideoPlayer;
    private final IVideoParamsChanged videoParamsChanged;

    private FileVideoReceiver mFileVideoReceiver;

    public VideoPlayer(final IVideoParamsChanged iVideoParamsChanged){
        videoParamsChanged=iVideoParamsChanged;
        nativeVideoPlayer= Video.initialize(this);
    }

    public void prepare(Surface surface,String groundRecordingFile){
        Video.nativeAddConsumers(nativeVideoPlayer,surface,groundRecordingFile);
    }

    public void release(){
        Video.nativeRemoveConsumers(nativeVideoPlayer);
    }


    public void addAndStartUDPReceiver(int port, boolean useRTSP){
        Video.nativeStartUDPReceiver(nativeVideoPlayer,port,useRTSP);
    }

    public void stopAndRemoveUDPReceiver(){
        Video.nativeStopUDPReceiver(nativeVideoPlayer);
    }

    public void addAndStartFileReceiver(final Context context,final FileVideoReceiver.REC_MODE mode,final String filename){
        mFileVideoReceiver=new FileVideoReceiver(context,this,mode,filename);
        mFileVideoReceiver.startReceiving();
    }

    public void stopAndRemoveFileReceiver(){
        mFileVideoReceiver.stopReceivingAndWait();
        mFileVideoReceiver=null;
    }

    @Override
    public void onNewVideoData(byte[] buffer, int offset, int length) {
        Video.nativePassNALUData(nativeVideoPlayer,buffer,offset,length);
    }

    @Override
    public void onVideoRatioChanged(int videoW, int videoH) {
        if(videoParamsChanged!=null){
            videoParamsChanged.onVideoRatioChanged(videoW,videoH);
        }
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
            Video.finalize(nativeVideoPlayer);
        } finally {
            super.finalize();
        }
    }




}
