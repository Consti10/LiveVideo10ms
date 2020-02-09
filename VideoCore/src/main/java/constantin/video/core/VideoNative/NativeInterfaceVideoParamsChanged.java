package constantin.video.core.VideoNative;


public interface NativeInterfaceVideoParamsChanged {
    @SuppressWarnings("unused")
    void onVideoRatioChanged(int videoW, int videoH);
    @SuppressWarnings("unused")
    void onDecodingInfoChanged(float currentFPS,float currentKiloBitsPerSecond,float avgParsingTime_ms,float avgWaitForInputBTime_ms,float avgDecodingTime_ms,
                               int nNALU, int nNALUSFeeded);
}
