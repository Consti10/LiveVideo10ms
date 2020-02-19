
#include "LowLagDecoder.h"
#include "../General/CPUPriority.hpp"
#include <unistd.h>
#include <sstream>


#define PRINT_DEBUG_INFO
constexpr const auto TAG="LowLagDecoder";

#include <h264_stream.h>
#include <vector>


#define MLOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
constexpr int BUFFER_TIMEOUT_US=20*1000; //20ms (a little bit more than 16.6ms)
constexpr int TIME_BETWEEN_LOGS_MS=5*1000; //5s


using namespace std::chrono;

LowLagDecoder::LowLagDecoder(ANativeWindow* window,const int checkOutputThreadCpuPrio,bool SW):
        mCheckOutputThreadCPUPriority(checkOutputThreadCpuPrio){
    decoder.SW=SW;
    decoder.window=window;
    decoder.configured=false;
}

void LowLagDecoder::registerOnDecoderRatioChangedCallback(DECODER_RATIO_CHANGED decoderRatioChangedC) {
    onDecoderRatioChangedCallback=decoderRatioChangedC;
}

void LowLagDecoder::registerOnDecodingInfoChangedCallback(DECODING_INFO_CHANGED_CALLBACK decodingInfoChangedCallback){
    onDecodingInfoChangedCallback=decodingInfoChangedCallback;
}

void LowLagDecoder::interpretNALU(const NALU& nalu){
    //we need this lock, since the receiving/parsing/feeding runs on its own thread, relative to the thread that creates / deletes the decoder
    std::lock_guard<std::mutex> lock(mMutexInputPipe);
    /*NALU* modNALU=nullptr;
    if(nalu.isSPS()){
        h264_stream_t* h=nalu.toH264Stream();
        //Do manipulations to h->sps...
        modNALU=NALU::fromH264StreamAndFree(h,&nalu);
    }*/
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
        feedDecoder(&nalu);
        decodingInfo.nNALUSFeeded++;
    }else{
        //store sps / pps data. As soon as enough data has been buffered to initialize the decoder,do so.
        mKeyFrameFinder.saveIfKeyFrame(nalu);
        if(mKeyFrameFinder.allKeyFramesAvailable()){
            configureStartDecoder(mKeyFrameFinder.getCSD0(),mKeyFrameFinder.getCSD1());
        }
    }
}

//Initialize decoder with provided SPS/PPS data.
//Set Decoder.configured to true on success
void LowLagDecoder::configureStartDecoder(const NALU& sps,const NALU& pps){
    if(decoder.SW){
        decoder.codec = AMediaCodec_createCodecByName("OMX.google.h264.decoder");
    }else {
        decoder.codec = AMediaCodec_createDecoderByType("video/avc");
        //char* name;
        //AMediaCodec_getName(decoder.codec,&name);
        //MLOGD("Created decoder %s",name);
        //AMediaCodec_releaseName(decoder.codec,name);
    }
    AMediaFormat* format=AMediaFormat_new();
    AMediaFormat_setString(format,AMEDIAFORMAT_KEY_MIME,"video/avc");
    const auto videoWH= sps.getVideoWidthHeightSPS();
    MLOGD("XYZ %d %d",videoWH[0],videoWH[1]);
    //AMediaFormat_setInt32(decoder.format,AMEDIAFORMAT_KEY_FRAME_RATE,60);
    //AVCProfileBaseline==1
    //AMediaFormat_setInt32(decoder.format,AMEDIAFORMAT_KEY_PROFILE,1);
    //AMediaFormat_setInt32(decoder.format,AMEDIAFORMAT_KEY_PRIORITY,0);
    AMediaFormat_setInt32(format,AMEDIAFORMAT_KEY_WIDTH,videoWH[0]);
    AMediaFormat_setInt32(format,AMEDIAFORMAT_KEY_HEIGHT,videoWH[1]);
    AMediaFormat_setBuffer(format,"csd-0",sps.data,(size_t)sps.data_length);
    AMediaFormat_setBuffer(format,"csd-1",pps.data,(size_t)pps.data_length);

    AMediaCodec_configure(decoder.codec,format, decoder.window, nullptr, 0);
    AMediaFormat_delete(format);
    if (decoder.codec== nullptr) {
        MLOGD("Cannot configure decoder");
        //set csd-0 and csd-1 back to 0, maybe they were just faulty but we have better luck with the next ones
        mKeyFrameFinder.reset();
        return;
    }
    AMediaCodec_start(decoder.codec);
    mCheckOutputThread=new std::thread([this] { this->checkOutputLoop(); });
    decoder.configured=true;
}

void LowLagDecoder::checkOutputLoop() {
    setCPUPriority(mCheckOutputThreadCPUPriority,"DecoderCheckOutput");
    AMediaCodecBufferInfo info;
    bool decoderSawEOS=false;
    bool decoderProducedUnknown=false;
    while(!decoderSawEOS && !decoderProducedUnknown) {
        ssize_t index=AMediaCodec_dequeueOutputBuffer(decoder.codec,&info,BUFFER_TIMEOUT_US);
        if (index >= 0) {
            const auto now=steady_clock::now();
            const int64_t nowNS=(int64_t)duration_cast<nanoseconds>(now.time_since_epoch()).count();
            const int64_t nowUS=(int64_t)duration_cast<microseconds>(now.time_since_epoch()).count();
            //the timestamp for releasing the buffer is in NS, just release as fast as possible (e.g. now)
            AMediaCodec_releaseOutputBufferAtTime(decoder.codec,(size_t)index,nowNS);
            //but the presentationTime is in US
            int64_t deltaDecodingTime=nowUS-info.presentationTimeUs;
            decodingTime_us.add((long)deltaDecodingTime);
            nDecodedFrames.add(1);
            if (info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) {
                MLOGD("Decoder saw EOS");
                decoderSawEOS=true;
                continue;
            }
        } else if (index == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED ) {
            auto format = AMediaCodec_getOutputFormat(decoder.codec);
            AMediaFormat_getInt32(format,AMEDIAFORMAT_KEY_WIDTH,&mWidth);
            AMediaFormat_getInt32(format,AMEDIAFORMAT_KEY_HEIGHT,&mHeight);
            if(onDecoderRatioChangedCallback!= nullptr && mWidth!=0 && mHeight!=0){
                onDecoderRatioChangedCallback({mWidth,mHeight});
            }
            MLOGD("AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED %d %d %s",mWidth,mHeight,AMediaFormat_toString(format));
        } else if(index==AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED){
            MLOGD("AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED");
        } else if(index==AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
            //LOGD("AMEDIACODEC_INFO_TRY_AGAIN_LATER");
        } else {
            MLOGD("dequeueOutputBuffer idx: %d .Exit.",(int)index);
            decoderProducedUnknown=true;
            continue;
        }
        //Recalculate the
        //every 2 seconds recalculate the current fps and bitrate
        const auto now=steady_clock::now();
        const auto delta=now-decodingInfo.lastCalculation;
        if(duration_cast<milliseconds>(delta).count()>DECODING_INFO_RECALCULATION_INTERVAL_MS){
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
    MLOGD("Exit CheckOutputLoop");
}

//Feed nullptr to indicate EOS
void LowLagDecoder::feedDecoder(const NALU* naluP){
    const auto now=std::chrono::steady_clock::now();
    const auto deltaParsing=naluP!= nullptr ? now-naluP->creationTime : std::chrono::steady_clock::duration::zero();
    /*const bool isKeyFrame=nalu.data_length>0 &&( nalu.isSPS() || nalu.isSPS());
    if(isKeyFrame){
        return;
    }*/
    while(true){
        const auto index=AMediaCodec_dequeueInputBuffer(decoder.codec,BUFFER_TIMEOUT_US);
        if (index >=0) {
            size_t inputBufferSize;
            void* buf = AMediaCodec_getInputBuffer(decoder.codec,(size_t)index,&inputBufferSize);
            if(naluP== nullptr){
                AMediaCodec_queueInputBuffer(decoder.codec, (size_t)index, 0, (size_t)0,0, AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM);
            }else{
                const NALU& nalu=*naluP;
                if(nalu.data_length>inputBufferSize){
                    return;
                }
                std::memcpy(buf, nalu.data,(size_t)nalu.data_length);
                //this timestamp will be later used to calculate the decoding latency
                const uint64_t presentationTimeUS=(uint64_t)duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();
                const auto flag=nalu.isPPS() || nalu.isSPS() ? AMEDIACODEC_BUFFER_FLAG_CODEC_CONFIG : 0;
                AMediaCodec_queueInputBuffer(decoder.codec, (size_t)index, 0, (size_t)nalu.data_length,presentationTimeUS, flag);
                waitForInputB_us.add((long)duration_cast<microseconds>(steady_clock::now()-now).count());
                parsingTime_us.add((long)duration_cast<microseconds>(deltaParsing).count());
            }
            return;
        }else if(index==AMEDIACODEC_INFO_TRY_AGAIN_LATER){
            //just try again. But if we had no success in the last 1 second,log a warning and return
            const int deltaMS=(int)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-now).count();
            if(deltaMS>1000){
                MLOGD("AMEDIACODEC_INFO_TRY_AGAIN_LATER for more than 1 second %d. return.",deltaMS);
                return;
            }
        }else{
            //Something went wrong. But we will feed the next nalu soon anyways
            MLOGD("dequeueInputBuffer idx %d return.",(int)index);
            return;
        }
    }
}


//when this call returns, it is guaranteed that no more data will be fed to the decoder
//but output buffer(s) are still polled from the queue
void LowLagDecoder::closeInputPipe() {
    std::lock_guard<std::mutex> lock(mMutexInputPipe);
    inputPipeClosed=true;
}

void LowLagDecoder::waitForShutdownAndDelete() {
    closeInputPipe();
    if(decoder.configured){
        //feed the EOS
        feedDecoder(nullptr);
        //if not already happened, wait for EOS to arrive at the checkOutput thread
        if(mCheckOutputThread->joinable()){
            mCheckOutputThread->join();
        }
        delete(mCheckOutputThread);
        mCheckOutputThread= nullptr;
        //now clean up the hw decoder
        AMediaCodec_stop(decoder.codec);
        AMediaCodec_delete(decoder.codec);
        mKeyFrameFinder.reset();
        decoder.configured=false;
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
    MLOGD("%s",frameLog.str().c_str());
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





