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
#include <MJPEGDecodeAndroid.hpp>
#include <FileReaderFPV.h>

// Transcode from MJPEG stream (btw. .fpv file containing MJPEG stream )
// To .mp4 file containing .h264 encoded data
// On success, input file is deleted
// On error,if output file was writtend, output file is deleted

class SimpleTranscoder {
private:
    AMediaCodec* mediaCodec{};
    // in debug mode the transcoder will run until interrupted and use a test pattern as input
    // filename is determined by the TEST_FILE_DIRECTORY
    const bool DEBUG_USE_PATTERN_INSTEAD= true;
    const bool DELETE_FILES_WHEN_DONE= false;
    // only used to create new filename when in debug mode
    const std::string TEST_FILE_DIRECTORY;
    const std::string INPUT_FILE_PATH;
    const std::string OUTPUT_FILE_PATH;
    FileReaderMJPEG fileReaderMjpeg;
    size_t videoTrackIndex=0;
    AMediaMuxer* mediaMuxer=nullptr;
    int outputFileFD=0;
    MJPEGDecodeAndroid mjpegDecodeAndroid;
    // create and configure a new AMediaCodec
    // returns AMediaCodec on success, nullptr otherwise
    static AMediaCodec* openMediaCodecEncoder(const int wantedColorFormat);
    static constexpr int32_t WIDTH = 640;
    static constexpr int32_t HEIGHT = 480;
    static constexpr int32_t FRAME_RATE = 30;
    static constexpr int32_t BIT_RATE= 5*1024*1024;
    static constexpr int TIMEOUT_US=5*1000;
    bool successfullyTranscodedWholeFile=false;
public:
    SimpleTranscoder(std::string GROUND_RECORDING_DIRECTORY1,std::string INPUT_FILE_PATH1);
    ~SimpleTranscoder();
    void loopEncoder(JNIEnv* env);
    int frameTimeUs=0;
    int frameIndex=0;
};


#endif //LIVEVIDEO10MS_SIMPLEENCODER_H
