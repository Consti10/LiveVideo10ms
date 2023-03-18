//
// Created by Constantin on 1/9/2019.
//

#ifndef FPV_VR_VIDEOPLAYERN_H
#define FPV_VR_VIDEOPLAYERN_H

#include <UDPReceiver.h>
#include "GroundRecorderRAW.hpp"
#include <SharedPreferences.hpp>
#include <GroundRecorderFPV.hpp>
#include <FileReader.h>
#include <jni.h>
#include "../Experiment360/FFMpegVideoReceiver.h"
#include "../Experiment360/FFMPEGFileWriter.h"
#include "../Decoder/LowLagDecoder.h"
#include "../Parser/H26XParser.h"
#include "../Parser/EncodeRtpTest.h"

class VideoPlayer{
public:
    VideoPlayer(JNIEnv * env, jobject context, const char* DIR);
    enum VIDEO_DATA_TYPE{RTP_H264,RAW_h264,RTP_H265,RAW_H265,CUSTOM,CUSTOM2,DJI};
    void onNewVideoData(const uint8_t* data,const std::size_t data_length,const VIDEO_DATA_TYPE videoDataType);
    /*
     * Set the surface the decoder can be configured with. When @param surface==nullptr
     * It is guaranteed that the surface is not used by the decoder anymore when this call returns
     */
    void setVideoSurface(JNIEnv* env, jobject surface);
    /*
     * Start the receiver and ground recorder if enabled
     */
    void start(JNIEnv *env,jobject androidContext);
    /**
     * Stop the receiver and ground recorder if enabled
     */
    void stop(JNIEnv *env,jobject androidContext);
    /*
     * Returns a string with the current configuration for debugging
     */
    std::string getInfoString()const;
private:
    void onNewNALU(const NALU& nalu);
    //Assumptions: Max bitrate: 40 MBit/s, Max time to buffer: 100ms
    //5 MB should be plenty !
    static constexpr const size_t WANTED_UDP_RCVBUF_SIZE=1024*1024*5;
    //Retreive settings from shared preferences
    SharedPreferences mVideoSettings;
    enum SOURCE_TYPE_OPTIONS{UDP,FILE,ASSETS,VIA_FFMPEG_URL,EXTERNAL};
    const std::string GROUND_RECORDING_DIRECTORY;
    JavaVM* javaVm=nullptr;
public:
    H26XParser mParser;
    LowLagDecoder mLowLagDecoder;
    std::unique_ptr<FFMpegVideoReceiver> mFFMpegVideoReceiver;
    std::unique_ptr<UDPReceiver> mUDPReceiver;
    long nNALUsAtLastCall=0;
public:
    DecodingInfo latestDecodingInfo{};
    std::atomic<bool> latestDecodingInfoChanged=false;
    VideoRatio latestVideoRatio{};
    std::atomic<bool> latestVideoRatioChanged=false;
    // These are shared with telemetry receiver when recording / reading from .fpv files
    FileReader mFileReceiver;
    GroundRecorderFPV mGroundRecorderFPV;
    bool VS_ENABLE_H264_SPS_VUI_FIX=false;

    bool lastFrameWasAUD=false;
};

#endif //FPV_VR_VIDEOPLAYERN_H
