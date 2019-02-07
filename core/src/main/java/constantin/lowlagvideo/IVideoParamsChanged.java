package constantin.lowlagvideo;

public interface IVideoParamsChanged{
    void onVideoRatioChanged(int videoW, int videoH);
    void onDecodingInfoChanged(final DecodingInfo decodingInfo);
}
