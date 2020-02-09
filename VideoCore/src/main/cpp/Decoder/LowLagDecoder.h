//
// Created by Constantin on 29.05.2017.
//

#ifndef LOW_LAG_DECODER
#define LOW_LAG_DECODER

#include <android/native_window.h>
#include <media/NdkMediaCodec.h>
#include <android/log.h>
#include <jni.h>
#include <iostream>
#include <thread>
#include <atomic>

#include "../NALU/NALU.hpp"
#include "../Helper/TimeHelper.hpp"
#include "../NALU/KeyFrameFinder.h"

class LowLagDecoder {
private:
    struct Decoder{
        bool configured= false;
        bool SW= false;
        AMediaCodec *codec= nullptr;
        ANativeWindow* window= nullptr;
        AMediaFormat *format=nullptr;
    };
public:
    struct DecodingInfo{
        std::chrono::steady_clock::time_point lastCalculation=std::chrono::steady_clock::now();
        long nNALU=0;
        long nNALUSFeeded=0;
        float currentFPS=0;
        float currentKiloBitsPerSecond=0;
        float avgParsingTime_ms=0;
        float avgWaitForInputBTime_ms=0;
        float avgDecodingTime_ms=0;
    };
    typedef std::function<void(DecodingInfo&)> DECODING_INFO_CHANGED_CALLBACK;
    typedef std::function<void(int,int)> DECODER_RATIO_CHANGED;
public:
    LowLagDecoder(ANativeWindow* window,int checkOutputThreadCpuPrio,bool SW=false);
    void registerOnDecoderRatioChangedCallback(DECODER_RATIO_CHANGED decoderRatioChangedC);
    void registerOnDecodingInfoChangedCallback(DECODING_INFO_CHANGED_CALLBACK decodingInfoChangedCallback);
    void interpretNALU(const NALU& nalu);
    void waitForShutdownAndDelete();
private:
    void configureStartDecoder(const NALU& sps,const NALU& pps);
    void feedDecoder(const NALU& nalu,bool justEOS);
    void checkOutputLoop();
    void printAvgLog();
    void closeInputPipe();
    int mWidth,mHeight;
    std::thread* mCheckOutputThread= nullptr;
    const int mCheckOutputThreadCPUPriority;
    Decoder decoder;
    DecodingInfo decodingInfo;
    bool inputPipeClosed=false;
    std::mutex mMutexInputPipe;
    DECODER_RATIO_CHANGED onDecoderRatioChangedCallback= nullptr;
    DECODING_INFO_CHANGED_CALLBACK onDecodingInfoChangedCallback= nullptr;
    std::chrono::steady_clock::time_point lastLog=std::chrono::steady_clock::now();
    RelativeCalculator nDecodedFrames;
    RelativeCalculator nNALUBytesFed;
    AvgCalculator parsingTime_us;
    AvgCalculator waitForInputB_us;
    AvgCalculator decodingTime_us;
private:
    static constexpr uint8_t SPS_X264[31]{
            0,0,0,1,103,66,192,40,217,0,120,2,39,229,192,90,128,128,128,160,0,0,125,32,0,29,76,17,227,6,73
    };
    static constexpr uint8_t SPS_X264_NO_VUI[15]{
            0,0,0,1,103,66,192,40,217,0,120,2,39,229,64
    };
    KeyFrameFinder mKeyFrameFinder;
};

#endif //LOW_LAG_DECODER
