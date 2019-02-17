package constantin.video.core.VideoNative;

import android.view.Surface;

public class VideoNative {
    static {
        System.loadLibrary("VideoNative");
    }
    public static native  <T extends NativeInterfaceVideoParamsChanged> long initialize(T t);
    public static native void finalize(long nativeVideoPlayer);
    //
    public static native void nativeAddConsumers(long nativeInstance, Surface surface, String filePathGroundRec);
    public static native void nativeRemoveConsumers(long videoPlayerN);

    public static native void nativePassNALUData(long nativeInstance,byte[] b,int offset,int size);
    public static native void nativeStartUDPReceiver(long nativeInstance, int port,boolean useRTP);
    public static native void nativeStopUDPReceiver(long nativeInstance);

    /**
     * Debugging/ Testing only
     */
    public static native String getVideoInfoString(long nativeInstance);
    public static native boolean anyVideoDataReceived(long nativeInstance);
    public static native boolean anyVideoBytesParsedSinceLastCall(long nativeInstance);
    public static native boolean receivingVideoButCannotParse(long nativeInstance);

    public interface NativeInterfaceVideoParamsChanged {
        @SuppressWarnings("unused")
        void onVideoRatioChanged(int videoW, int videoH);
        @SuppressWarnings("unused")
        void onDecodingInfoChanged(float currentFPS, float currentKiloBitsPerSecond, float avgParsingTime_ms, float avgWaitForInputBTime_ms, float avgDecodingTime_ms,
                                   int nNALU, int nNALUSFeeded);
    }
}
