
#include "LowLagDecoder.h"
#include "../General/CPUPriority.hpp"
#include "SPSHelper.h"
#include <unistd.h>
#include <sstream>


//#define PRINT_DEBUG_INFO
#define TAG "LowLagDecoder"

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
constexpr int BUFFER_TIMEOUT_US=20*1000; //20ms (a little bit more than 16.6ms)
constexpr int TIME_BETWEEN_LOGS_MS=5*1000; //5s

using namespace std::chrono;

LowLagDecoder::LowLagDecoder(ANativeWindow* window,const int checkOutputThreadCpuPrio):
        mCheckOutputThreadCPUPriority(checkOutputThreadCpuPrio){
    decoder.SW= false;
    decoder.window=window;
    decoder.configured=false;
}

void LowLagDecoder::registerOnDecoderRatioChangedCallback(std::function<void (int,int)> decoderRatioChangedC) {
    onDecoderRatioChangedCallback=decoderRatioChangedC;
}

void LowLagDecoder::registerOnDecodingInfoChangedCallback(std::function<void(LowLagDecoder::DecodingInfo&)> decodingInfoChangedCallback){
    onDecodingInfoChangedCallback=decodingInfoChangedCallback;
}

void LowLagDecoder::interpretNALU(const NALU& nalu){
    //LOGD("::interpretNALU %d",nalu.data_length);
    //we need this lock, since the receiving/parsing/feeding runs on its own thread, relative to the thread that creates / deletes the decoder
    std::lock_guard<std::mutex> lock(mMutex);
    decodingInfo.nNALU++;
    nNALUBytesFed.add(nalu.data_length);
    if(inputPipeClosed){
        //A feedD thread (e.g. file or udp) thread might still be running
        //But since we want to feed the EOS now we mustn't feed any more non-eos data
        return;
    }
    if(nalu.data_length<=4){
        //No data in NALU (e.g at the beginning of a stream)
        return;
    }
    if(decoder.configured){
        feedDecoder(nalu,false);
        decodingInfo.nNALUSFeeded++;
    } else{
        configureStartDecoder(nalu);
    }
}


void LowLagDecoder::configureStartDecoder(const NALU& nalu){
    if(nalu.isSPS()){
        memcpy(CSDO,nalu.data,(size_t )nalu.data_length);
        CSD0Length=nalu.data_length;
        LOGD("SPS found");
    }else if(nalu.isPPS()){
        memcpy(CSD1,nalu.data,(size_t )nalu.data_length);
        CSD1Length=nalu.data_length;
        LOGD("PPS found");
    }
    if(CSD0Length==0||CSD1Length==0){
        //There is no CSD0/CSD1 data yet. We don't have enough information to initialize the decoder.
        return;
    }
    //SW decoder seems to be broken in native
    if(decoder.SW){
        decoder.codec = AMediaCodec_createDecoderByType("OMX.google.h264.decoder");
    }else {
        decoder.codec = AMediaCodec_createDecoderByType("video/avc");
    }
    decoder.format=AMediaFormat_new();
    AMediaFormat_setString(decoder.format,AMEDIAFORMAT_KEY_MIME,"video/avc");
    int videoW,videoH;
    SPSHelper::ParseSPS(&CSDO[5],CSD0Length,&videoW,&videoH);
    LOGD("XYZ %d %d",videoW,videoH);

    AMediaFormat_setInt32(decoder.format,AMEDIAFORMAT_KEY_WIDTH,videoW);
    AMediaFormat_setInt32(decoder.format,AMEDIAFORMAT_KEY_HEIGHT,videoH);


    AMediaFormat_setBuffer(decoder.format,"csd-0",&CSDO,(size_t)CSD0Length);
    AMediaFormat_setBuffer(decoder.format,"csd-1",&CSD1,(size_t)CSD1Length);

    AMediaCodec_configure(decoder.codec, decoder.format, decoder.window, nullptr, 0);
    if (decoder.codec== nullptr) {
        LOGD("Cannot configure decoder");
        //set csdo and csd1 back to 0, maybe they were just faulty but we have better luck with the next ones
        CSD0Length=0;
        CSD1Length=0;
        return;
    }
    AMediaCodec_start(decoder.codec);
    decoder.configured=true;
    mCheckOutputThread=new std::thread([this] { this->checkOutputLoop(); });
}

void LowLagDecoder::checkOutputLoop() {
    setCPUPriority(mCheckOutputThreadCPUPriority,"DecoderCheckOutput");
    AMediaCodecBufferInfo info;
    bool decoderSawEOS=false;
    while(!decoderSawEOS) {
        ssize_t index=AMediaCodec_dequeueOutputBuffer(decoder.codec,&info,BUFFER_TIMEOUT_US);
        if (index >= 0) {
            const int64_t nowNS=(int64_t)duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
            const int64_t nowUS=(int64_t)duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();
            //the timestamp for releasing the buffer is in NS, just release as fast as possible (e.g. now)
            AMediaCodec_releaseOutputBufferAtTime(decoder.codec,(size_t)index,nowNS);
            //but the presentationTime is in US
            int64_t deltaDecodingTime=nowUS-info.presentationTimeUs;
            decodingTime_us.add((long)deltaDecodingTime);
            nDecodedFrames.add(1);
            if (info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) {
                LOGD("Decoder saw EOS");
                decoderSawEOS=true;
                continue;
            }
        } else if (index == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED ) {
            auto format = AMediaCodec_getOutputFormat(decoder.codec);
            AMediaFormat_getInt32(format,AMEDIAFORMAT_KEY_WIDTH,&mWidth);
            AMediaFormat_getInt32(format,AMEDIAFORMAT_KEY_HEIGHT,&mHeight);
            if(onDecoderRatioChangedCallback!= nullptr && mWidth!=0 && mHeight!=0){
                onDecoderRatioChangedCallback(mWidth,mHeight);
            }
            //LOGD("AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED %d %d",mWidth,mHeight);
        } else if(index==AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED){
            //LOGD("AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED");
        } else if(index==AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
            //LOGD("AMEDIACODEC_INFO_TRY_AGAIN_LATER");
        } else {
            LOGD("dequeueOutputBuffer idx: %d .Exit.",(int)index);
            return;
        }
        //every 2 seconds recalculate the current fps and bitrate
        steady_clock::time_point now=steady_clock::now();
        const auto delta=now-decodingInfo.lastCalculation;
        if(duration_cast<seconds>(delta).count()>2){
            decodingInfo.lastCalculation=steady_clock::now();
            decodingInfo.currentFPS=nDecodedFrames.getDeltaSinceLastCall()/(float)duration_cast<seconds>(delta).count();
            decodingInfo.currentKiloBitsPerSecond=(nNALUBytesFed.getDeltaSinceLastCall()/duration_cast<seconds>(delta).count())/1024.0f*8.0f;
            //and recalculate the avg latencies. If needed,also print the log.
            decodingInfo.avgDecodingTime_ms=decodingTime_us.getAvg()/1000.0f;
            decodingInfo.avgParsingTime_ms=parsingTime_us.getAvg()/1000.0f;
            decodingInfo.avgWaitForInputBTime_ms=waitForInputB_us.getAvg()/1000.0f;
            printAvgLog();
            if(onDecodingInfoChangedCallback!= nullptr){
                onDecodingInfoChangedCallback(decodingInfo);
            }
        }
    }
}

void LowLagDecoder::feedDecoder(const NALU& nalu,bool justEOS){
    const auto now=std::chrono::steady_clock::now();
    const auto deltaParsing=now-nalu.creationTime;
    while (true) {
        ssize_t index=AMediaCodec_dequeueInputBuffer(decoder.codec,BUFFER_TIMEOUT_US);
        size_t size;
        if (index >=0) {
            void* buf = AMediaCodec_getInputBuffer(decoder.codec,(size_t)index,&size);
            if(!justEOS){
                if(nalu.data_length>size){
                    return;
                }
                std::memcpy(buf, nalu.data,(size_t)nalu.data_length);
                //this timestamp will be later used to calculate the decoding latency
                const uint64_t presentationTimeUS=(uint64_t)duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();
                AMediaCodec_queueInputBuffer(decoder.codec, (size_t)index, 0, (size_t)nalu.data_length,presentationTimeUS, 0);
                waitForInputB_us.add((long)duration_cast<microseconds>(steady_clock::now()-now).count());
                parsingTime_us.add((long)duration_cast<microseconds>(deltaParsing).count());
            }else{
                 AMediaCodec_queueInputBuffer(decoder.codec, (size_t)index, 0, (size_t)0,0, AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM);
            }
            return;
        }else if(index==AMEDIACODEC_INFO_TRY_AGAIN_LATER){
            //just try again
        }else{
            //Something went wrong. But we will feed the next nalu soon anyways
            LOGD("dequeueInputBuffer idx %d Exit.",(int)index);
            return;
        }
    }
}


//when this call returns, it is guaranteed that no more data will be feed to the decoder
//but output buffer(s) are still polled from the queue
void LowLagDecoder::closeInputPipe() {
    std::lock_guard<std::mutex> lock(mMutex);
    inputPipeClosed=true;
}

void LowLagDecoder::waitForShutdownAndDelete() {
    closeInputPipe();
    if(decoder.configured){
        //feed the EOS
        feedDecoder(NALU(nullptr,0),true);
        //if not already happened, wait for EOS to arrive at the checkOutput thread
        if(mCheckOutputThread->joinable()){
            mCheckOutputThread->join();
        }
        delete(mCheckOutputThread);
        mCheckOutputThread= nullptr;
        //now clean up the hw decoder
        AMediaCodec_stop(decoder.codec);
        AMediaCodec_delete(decoder.codec);
    }
}


void LowLagDecoder::printAvgLog() {
#ifdef PRINT_DEBUG_INFO
    auto now=steady_clock::now();
    if(duration_cast<milliseconds>(now-lastLog).count()>TIME_BETWEEN_LOGS_MS){
        lastLog=now;
    }else{
        return;
    }
    std::ostringstream frameLog;
    float avgDecodingLatencySum=decodingInfo.avgParsingTime_ms+decodingInfo.avgWaitForInputBTime_ms+
            decodingInfo.avgDecodingTime_ms;
    frameLog<<"......................Decoding Latency Averages......................"<<
            "\nParsing:"<<decodingInfo.avgParsingTime_ms
            <<" | WaitInputBuffer:"<<decodingInfo.avgWaitForInputBTime_ms
            <<" | Decoding:"<<decodingInfo.avgDecodingTime_ms
            <<" | Decoding Latency Sum:"<<avgDecodingLatencySum<<
            "\nN NALUS:"<<decodingInfo.nNALU
            <<" | N NALUES feeded:" <<decodingInfo.nNALUSFeeded<<" | N Decoded Frames:"<<nDecodedFrames.getAbsolute()<<
            "\nFPS:"<<decodingInfo.currentFPS;
    LOGD("%s",frameLog.str().c_str());
#endif

    /*std::stringstream properties;
    properties << "system_clock" << std::endl;
    properties << std::chrono::system_clock::period::num << std::endl;
    properties << std::chrono::system_clock::period::den << std::endl;
    properties << "steady = " <<std:: boolalpha << std::chrono::system_clock::is_steady << std::endl << std::endl;

    properties << "high_resolution_clock" << std::endl;
    properties << std::chrono::high_resolution_clock::period::num << std::endl;
    properties << std::chrono::high_resolution_clock::period::den << std::endl;
    properties << "steady = " << std::boolalpha << std::chrono::high_resolution_clock::is_steady << std::endl << std::endl;

    properties << "steady_clock" << std::endl;
    properties << std::chrono::steady_clock::period::num << std::endl;
    properties << std::chrono::steady_clock::period::den << std::endl;
    properties << "steady = " << std::boolalpha << std::chrono::steady_clock::is_steady << std::endl << std::endl;

    LOGD("%s",properties.str().c_str());*/
}





