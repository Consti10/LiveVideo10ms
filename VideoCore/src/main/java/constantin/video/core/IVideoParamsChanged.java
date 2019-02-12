package constantin.video.core;

public interface IVideoParamsChanged{
    void onVideoRatioChanged(int videoW, int videoH);
    void onDecodingInfoChanged(final DecodingInfo decodingInfo);
}
