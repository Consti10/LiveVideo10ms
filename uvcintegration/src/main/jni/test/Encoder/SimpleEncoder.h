//
// Created by geier on 24/05/2020.
//

#ifndef LIVEVIDEO10MS_SIMPLEENCODER_H
#define LIVEVIDEO10MS_SIMPLEENCODER_H

#include <android/native_window.h>
#include <media/NdkMediaCodec.h>
#include <android/log.h>
#include <jni.h>
#include <iostream>
#include <thread>
#include <atomic>
#include <vector>
#include <media/NdkMediaMuxer.h>
#include <MJPEGDecodeAndroid.hpp>
#include <FileReaderFPV.h>

class SimpleEncoder {
private:
    std::thread* mEncoderThread= nullptr;
    std::atomic<bool> running;
    void loopEncoder();
    AMediaCodec* mediaCodec;
    const std::string GROUND_RECORDING_DIRECTORY;
    const std::string INPUT_FILE=GROUND_RECORDING_DIRECTORY+"TestInput.fpv";
    FileReaderMJPEG fileReaderMjpeg;
    size_t videoTrackIndex;
    AMediaMuxer* mediaMuxer=nullptr;
    int outputFileFD;
    MJPEGDecodeAndroid mjpegDecodeAndroid;
    bool openMediaCodecEncoder(const int wantedColorFormat);
    static constexpr int32_t WIDTH = 640;
    static constexpr int32_t HEIGHT = 480;
    static constexpr int32_t FRAME_RATE = 30;
    static constexpr int32_t BIT_RATE= 5*1024*1024;
public:
    SimpleEncoder(const std::string GROUND_RECORDING_DIRECTORY1):GROUND_RECORDING_DIRECTORY(GROUND_RECORDING_DIRECTORY1){}
    void start();
    void addBufferData(const uint8_t* data,const size_t data_len);
    void stop();
    std::mutex inputBufferDataMutex;
    std::vector<uint8_t> inputBufferData;
};


#endif //LIVEVIDEO10MS_SIMPLEENCODER_H
