package constantin.video.core;

import android.content.Context;
import android.view.Surface;

import constantin.video.core.VideoNative.VideoNative;

public class VideoPlayer implements FileVideoReceiver.IVideoDataRaw, VideoNative.NativeInterfaceVideoParamsChanged {
    private final long nativeVideoPlayer;
    private final IVideoParamsChanged videoParamsChanged;
    private FileVideoReceiver mFileVideoReceiver;
    private enum MODE{UDP,FILE}

    private MODE mCurrentMode;

    public VideoPlayer(final IVideoParamsChanged iVideoParamsChanged){
        videoParamsChanged=iVideoParamsChanged;
        nativeVideoPlayer= VideoNative.initialize(this);
    }

    public void prepare(Surface surface,String groundRecordingFile){
        VideoNative.nativeAddConsumers(nativeVideoPlayer,surface,groundRecordingFile);
    }

    private void release(){
        VideoNative.nativeRemoveConsumers(nativeVideoPlayer);
    }

    public void addAndStartReceiver(int port,boolean useRTP){
        mCurrentMode =MODE.UDP;
        VideoNative.nativeStartUDPReceiver(nativeVideoPlayer,port,useRTP);
    }

    public void addAndStartReceiver(final Context context,final FileVideoReceiver.REC_MODE mode,final String filename){
        mCurrentMode =MODE.FILE;
        mFileVideoReceiver=new FileVideoReceiver(context,this,mode,filename);
        mFileVideoReceiver.startReceiving();
    }

    public void stopAndRemovePlayerReceiver(){
        if(mCurrentMode ==MODE.UDP){
            VideoNative.nativeStopUDPReceiver(nativeVideoPlayer);
        }else{
            mFileVideoReceiver.stopReceivingAndWait();
            mFileVideoReceiver=null;
        }
        release();
    }

    public long getNativeInstance(){
        return nativeVideoPlayer;
    }


    @Override
    public void onNewVideoData(byte[] buffer, int offset, int length) {
        VideoNative.nativePassNALUData(nativeVideoPlayer,buffer,offset,length);
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
            VideoNative.finalize(nativeVideoPlayer);
        } finally {
            super.finalize();
        }
    }




}
