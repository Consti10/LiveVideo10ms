//
// Created by Constantin on 1/9/2019.
//

#ifndef FPV_VR_VIDEOPLAYERN_H
#define FPV_VR_VIDEOPLAYERN_H

#include <jni.h>
#include <UDPReceiver.h>
#include <GroundRecorder.h>
#include "../Decoder/LowLagDecoder.h"
#include "../Parser/H264Parser.h"

class Video{
public:
    Video(JNIEnv * env,jobject videoParamsChangedI);
private:
    void onDecoderRatioChangedCallback(int videoW,int videoH);
    void onDecodingInfoChangedCallback(const LowLagDecoder::DecodingInfo & info);
    void onNewNALU(const NALU& nalu);
    struct{
        JavaVM* javaVirtualMachine;
        jobject globalJavaObj;
        jmethodID onDecoderFPSChangedJAVA= nullptr;
        jmethodID onVideoRatioChangedJAVA=nullptr;
    }callToJava;
    ANativeWindow* window=nullptr;
public:
    void onNewVideoData(const uint8_t* data,const int data_length,const bool isRTPData,const bool limitFPS);
    void addConsumers(JNIEnv* env,jobject surface,jstring groundRecordingFileName);
    void removeConsumers();
    void startNativeUDPReceiver(int port,bool useRTP);
    void stopNativeUDPReceiver();
    std::string getInfoString();
public:
    H264Parser mParser;
    LowLagDecoder* mLowLagDecoder= nullptr;
    UDPReceiver* mVideoReceiver= nullptr;
    GroundRecorder* mGroundRecorder= nullptr;
    long nNALUsAtLastCall=0;
};

#endif //FPV_VR_VIDEOPLAYERN_H
