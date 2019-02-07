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

class AvgCalculator{
private:
    long sum=0;
    long sumCount=0;
public:
    AvgCalculator() = default;

    void add(long x){
        sum+=x;
        sumCount++;
    }
    long getAvg(){
        if(sumCount==0)return 0;
        return sum/sumCount;
    }
    void reset(){
        sum=0;
        sumCount=0;
    }
};

class RelativeCalculator{
private:
    long sum=0;
    long sumAtLastCall=0;
public:
    RelativeCalculator() = default;
    void add(long x){
        sum+=x;
    }
    long getDeltaSinceLastCall() {
        long ret = sum - sumAtLastCall;
        sumAtLastCall = sum;
        return ret;
    }
    long getAbsolute(){
        return sum;
    }
};


class LowLagDecoder {
private:
    struct Decoder{
        bool configured= false;
        bool SW= false;
        AMediaCodec *codec= nullptr;
        ANativeWindow* window= nullptr;
        AMediaFormat *format= nullptr;
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
public:
    LowLagDecoder(ANativeWindow* window,int checkOutputThreadCpuPrio);
    void registerOnDecoderRatioChangedCallback(std::function<void(int,int)> decoderRatioChangedC);
    void registerOnDecodingInfoChangedCallback(std::function<void(DecodingInfo&)> decodingInfoChangedCallback);
    void interpretNALU(const NALU& nalu);
    void waitForShutdownAndDelete();
private:
    void configureStartDecoder(const NALU& nalu);
    void feedDecoder(const NALU& nalu,bool justEOS);
    void checkOutputLoop();
    void printAvgLog();
    void closeInputPipe();
    int mWidth,mHeight;
    std::thread* mCheckOutputThread= nullptr;
    const int mCheckOutputThreadCPUPriority;
    Decoder decoder;
    DecodingInfo decodingInfo;
    uint8_t CSDO[NALU_MAXLEN],CSD1[NALU_MAXLEN];
    int CSD0Length=0,CSD1Length=0;
    bool inputPipeClosed=false;
    std::mutex mMutex;
    std::function<void(int,int)> onDecoderRatioChangedCallback= nullptr;
    std::function<void(DecodingInfo&)> onDecodingInfoChangedCallback= nullptr;
    std::chrono::steady_clock::time_point lastLog=std::chrono::steady_clock::now();
    RelativeCalculator nDecodedFrames;
    RelativeCalculator nNALUBytesFed;
    AvgCalculator parsingTime_us;
    AvgCalculator waitForInputB_us;
    AvgCalculator decodingTime_us;
};

#endif //LOW_LAG_DECODER
