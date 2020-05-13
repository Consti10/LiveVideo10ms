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
#include <TimeHelper.hpp>
#include "../NALU/KeyFrameFinder.h"

struct DecodingInfo{
    std::chrono::steady_clock::time_point lastCalculation=std::chrono::steady_clock::now();
    long nNALU=0;
    long nNALUSFeeded=0;
    float currentFPS=0;
    float currentKiloBitsPerSecond=0;
    float avgParsingTime_ms=0;
    float avgWaitForInputBTime_ms=0;
    float avgDecodingTime_ms=0;
    bool operator==(const DecodingInfo& d2)const{
        return nNALU==d2.nNALU && nNALUSFeeded==d2.nNALUSFeeded && currentFPS==d2.currentFPS &&
               currentKiloBitsPerSecond==d2.currentKiloBitsPerSecond && avgParsingTime_ms==d2.avgParsingTime_ms &&
               avgWaitForInputBTime_ms==d2.avgWaitForInputBTime_ms && avgDecodingTime_ms==d2.avgDecodingTime_ms;
    }
};
struct VideoRatio{
    int width;
    int height;
    bool operator==(const VideoRatio& b)const{
        return width==b.width && height==b.height;
    }
};

//Handles decoding of .h264 video
class LowLagDecoder {
private:
    struct Decoder{
        bool configured= false;
        AMediaCodec *codec= nullptr;
        ANativeWindow* window= nullptr;
    };
public:
    //Make sure to do no heavy lifting on this callback, since it is called from the low-latency mCheckOutputThread thread (best to copy values and leave processing to another thread)
    //The decoding info callback is called every DECODING_INFO_RECALCULATION_INTERVAL_MS
    typedef std::function<void(const DecodingInfo)> DECODING_INFO_CHANGED_CALLBACK;
    //The decoder ratio callback is called every time the output format changes
    typedef std::function<void(const VideoRatio)> DECODER_RATIO_CHANGED;
public:
    //@param window: must be a valid ANativeWindow
    //@param checkOutputThreadCpuPrio the CPU priority the check output buffer thread will run on
    //@param SW use SW or HW decoder
    //We cannot initialize the Decoder until we have SPS and PPS data -
    //when streaming this data will be available at some point in future
    //Therefore we don't allocate the MediaCodec resources here
    LowLagDecoder(JavaVM* javaVm,ANativeWindow* window,bool SW=false);
    //register the specified callbacks. Only one can be registered at a time
    void registerOnDecoderRatioChangedCallback(DECODER_RATIO_CHANGED decoderRatioChangedC);
    void registerOnDecodingInfoChangedCallback(DECODING_INFO_CHANGED_CALLBACK decodingInfoChangedCallback);
    //If the decoder has been configured, feed NALU. Else search for configuration data and
    //configure as soon as possible
    void interpretNALU(const NALU& nalu);
    //close input pipe, then feed EOS input
    //wait for EOS to arrive at output, then free all resources.
    void waitForShutdownAndDelete();
private:
    //Initialize decoder with provided SPS/PPS data.
    //Set Decoder.configured to true on success
    void configureStartDecoder(const NALU& sps,const NALU& pps);
    //Feed nullptr to indicate EOS.
    //Else,wait for input buffer to become available before feeding NALU
    void feedDecoder(const NALU* nalu);
    //Runs until EOS arrives at output buffer
    void checkOutputLoop();
    //when this call returns, it is guaranteed that no more data will be fed to the decoder
    //but output buffer(s) are still polled from the queue. Use this before feeding EOS
    void closeInputPipe();
    //Debug log
    void printAvgLog();
    int mWidth,mHeight;
    std::thread* mCheckOutputThread= nullptr;
    const bool SW;
    //Holds the AMediaCodec instance, as well as the state (configured or not configured)
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
    //Every n ms re-calculate the Decoding info
    static const constexpr int DECODING_INFO_RECALCULATION_INTERVAL_MS=1000;
    JavaVM* javaVm;
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
