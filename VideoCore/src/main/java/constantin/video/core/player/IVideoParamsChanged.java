package constantin.video.core.player;


import constantin.video.core.player.DecodingInfo;

//Similar to INativeVideoParamsChanged,but instead of passing all float's it passes
//a DecodingInfo instance. Not called by native code
public interface IVideoParamsChanged{
    void onVideoRatioChanged(int videoW, int videoH);
    void onDecodingInfoChanged(final DecodingInfo decodingInfo);
}
