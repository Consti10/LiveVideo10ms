//
// Created by geier on 24/05/2020.
//

#ifndef LIVEVIDEO10MS_SIMPLEENCODER_H
#define LIVEVIDEO10MS_SIMPLEENCODER_H

#include <jni.h>
#include <android/native_window.h>
#include <media/NdkMediaCodec.h>
#include <android/log.h>
#include <jni.h>
#include <iostream>
#include <thread>
#include <atomic>
#include <vector>
#include <media/NdkMediaMuxer.h>
//#include <MJPEGDecodeAndroid.hpp>
#include <FileReaderFPV.h>

class SimpleEncoder {
private:
    AMediaCodec* mediaCodec{};
    const std::string GROUND_RECORDING_DIRECTORY;
    const std::string INPUT_FILE=GROUND_RECORDING_DIRECTORY+"TestInput.fpv";
    FileReaderMJPEG fileReaderMjpeg;
    size_t videoTrackIndex=0;
    AMediaMuxer* mediaMuxer=nullptr;
    int outputFileFD=0;
    //MJPEGDecodeAndroid mjpegDecodeAndroid;
    // create and configure a new AMediaCodec
    // returns AMediaCodec on success, nullptr otherwise
    static AMediaCodec* openMediaCodecEncoder(const int wantedColorFormat);
    static constexpr int32_t WIDTH = 640;
    static constexpr int32_t HEIGHT = 480;
    static constexpr int32_t FRAME_RATE = 30;
    static constexpr int32_t BIT_RATE= 5*1024*1024;
    static constexpr int TIMEOUT_US=5*1000;
public:
    SimpleEncoder(std::string  GROUND_RECORDING_DIRECTORY1);
    ~SimpleEncoder();
    void loopEncoder(JNIEnv* env);
    int frameTimeUs=0;
    int frameIndex=0;
};


#endif //LIVEVIDEO10MS_SIMPLEENCODER_H
