//
// Created by Constantin on 1/9/2019.
//

#ifndef FPV_VR_VIDEOPLAYERN_H
#define FPV_VR_VIDEOPLAYERN_H

#include <jni.h>
#include <UDPReceiver.h>
#include "../Helper/GroundRecorder.hpp"
#include "../Decoder/LowLagDecoder.h"
#include "../Parser/H264Parser.h"
#include "../Helper/SettingsN.hpp"
#include "FileReader.h"
#include "../Experiment360/FFMpegVideoReceiver.h"

#include <media/NdkMediaMuxer.h>

class VideoNative{
public:
    VideoNative(JNIEnv * env,jobject videoParamsChangedI,jobject context,const char* DIR);
    void onNewVideoData(const uint8_t* data,const std::size_t data_length,const bool isRTPData,const int limitFPS);
    void addConsumers(JNIEnv* env,jobject surface);
    void removeConsumers();
    void startReceiver(JNIEnv *env, AAssetManager *assetManager);
    void stopReceiver();
    std::string getInfoString();
private:
    void onNewNALU(const NALU& nalu);
    ANativeWindow* window=nullptr;
    SettingsN mSettingsN;
    enum SOURCE_TYPE_OPTIONS{UDP,FILE,ASSETS,VIA_FFMPEG_URL,EXTERNAL};
    const std::string GROUND_RECORDING_DIRECTORY;
public:
    H264Parser mParser;
    LowLagDecoder* mLowLagDecoder= nullptr;
    UDPReceiver* mVideoReceiver= nullptr;
    FileReader* mFileReceiver=nullptr;
    GroundRecorder* mGroundRecorder= nullptr;
    long nNALUsAtLastCall=0;
    FFMpegVideoReceiver* mFFMpegVideoReceiver=nullptr;
public:
    LowLagDecoder::DecodingInfo latestDecodingInfo;
    LowLagDecoder::VideoRatio latestVideoRatio;
private:
    std::condition_variable conditionVariable;
    /*int mFD;
    AMediaMuxer* mMuxer=nullptr;
    ssize_t mTrackIndex;
    KeyFrameFinder mKeyFrameFInder;
    std::chrono::steady_clock::time_point lastFeed;*/
};

#endif //FPV_VR_VIDEOPLAYERN_H
