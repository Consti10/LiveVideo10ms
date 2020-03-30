package constantin.video.core.VideoPlayer;

//These methods are called by the native code.
//See also IVideoParamsChanged
public interface INativeVideoParamsChanged {
    @SuppressWarnings("unused")
    void onVideoRatioChanged(int videoW, int videoH);
    @SuppressWarnings("unused")
    void onDecodingInfoChanged(float currentFPS,float currentKiloBitsPerSecond,float avgParsingTime_ms,float avgWaitForInputBTime_ms,float avgDecodingTime_ms,
                               int nNALU, int nNALUSFeeded);
}
