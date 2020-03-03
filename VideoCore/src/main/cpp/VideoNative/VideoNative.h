//
// Created by Constantin on 1/9/2019.
//

#ifndef FPV_VR_VIDEOPLAYERN_H
#define FPV_VR_VIDEOPLAYERN_H

#include <jni.h>
#include "UDPReceiver.h"
#include "GroundRecorderRAW.hpp"
#include "../Decoder/LowLagDecoder.h"
#include "../Parser/H264Parser.h"
#include "../Helper/SettingsN.hpp"
#include "FileReader.h"
#include "../Experiment360/FFMpegVideoReceiver.h"
#include "GroundRecorderMP4.hpp"

class VideoNative{
public:
    VideoNative(JNIEnv * env,jobject context,const char* DIR);
    void onNewVideoData(const uint8_t* data,const std::size_t data_length,const bool isRTPData,const int limitFPS);
    void addConsumers(JNIEnv* env,jobject surface);
    void removeConsumers();
    void startReceiver(JNIEnv *env, AAssetManager *assetManager);
    void stopReceiver();
    std::string getInfoString();
private:
    void onNewNALU(const NALU& nalu);
    //don't forget to call ANativeWindow_release() on this pointer
    ANativeWindow* window=nullptr;
    SettingsN mSettingsN;
    enum SOURCE_TYPE_OPTIONS{UDP,FILE,ASSETS,VIA_FFMPEG_URL,EXTERNAL};
    const std::string GROUND_RECORDING_DIRECTORY;
public:
    H264Parser mParser;
    std::unique_ptr<LowLagDecoder> mLowLagDecoder;
    std::unique_ptr<FFMpegVideoReceiver> mFFMpegVideoReceiver;
    std::unique_ptr<UDPReceiver> mVideoReceiver;
    std::unique_ptr<FileReader> mFileReceiver;
    long nNALUsAtLastCall=0;
public:
    DecodingInfo latestDecodingInfo;
    std::atomic<bool> latestDecodingInfoChanged;
    VideoRatio latestVideoRatio;
    std::atomic<bool> latestVideoRatioChanged;
private:
    std::unique_ptr<GroundRecorderRAW> mGroundRecorder;
    std::unique_ptr<GroundRecorderMP4> mMP4GroundRecorder;
    static constexpr const size_t UDP_RECEIVER_BUFFER_SIZE=1024*1024*5; //5 MB should be plenty
};

#endif //FPV_VR_VIDEOPLAYERN_H
