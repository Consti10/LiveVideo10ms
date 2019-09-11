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
#include "../Helper/FileReader.hpp"
#include "../Experiment360/FFMpegVideoPlayer.h"

class VideoNative{
public:
    VideoNative(JNIEnv * env,jobject videoParamsChangedI,jobject context,const char* DIR);
private:
    void onDecoderRatioChangedCallback(int videoW,int videoH);
    void onDecodingInfoChangedCallback(const LowLagDecoder::DecodingInfo & info);
    void onNewNALU(const NALU& nalu);
    struct{
        JavaVM* javaVirtualMachine;
        jobject globalJavaObj;
        jmethodID onDecodingInfoChangedJAVA= nullptr;
        jmethodID onVideoRatioChangedJAVA=nullptr;
    }callToJava;
    ANativeWindow* window=nullptr;
    SettingsN mSettingsN;
    enum SOURCE_TYPE_OPTIONS{UDP,FILE,ASSETS,EXTERNAL};
    const std::string GROUND_RECORDING_DIRECTORY;
public:
    void onNewVideoData(const uint8_t* data,const int data_length,const bool isRTPData,const bool limitFPS);
    void addConsumers(JNIEnv* env,jobject surface);
    void removeConsumers();
    void startReceiver(JNIEnv *env, AAssetManager *assetManager);
    void stopReceiver();
    std::string getInfoString();
public:
    H264Parser mParser;
    LowLagDecoder* mLowLagDecoder= nullptr;
    UDPReceiver* mVideoReceiver= nullptr;
    FileReader* mFileReceiver=nullptr;
    GroundRecorder* mGroundRecorder= nullptr;
    long nNALUsAtLastCall=0;

    FFMpegVideoPlayer* test=nullptr;
};

#endif //FPV_VR_VIDEOPLAYERN_H
