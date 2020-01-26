
#include "LowLagDecoder.h"
#include "../General/CPUPriority.hpp"
#include "SPSHelper.h"
#include <unistd.h>
#include <sstream>


#define PRINT_DEBUG_INFO
#define TAG "LowLagDecoder"

#include <h264_stream.h>
#include <vector>


#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
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

    /*int s,t;
    int res=find_nal_unit(const_cast<uint8_t*>(nalu.data),nalu.data_length,&s,&t);
    LOGD("find nal %d %d %d",res,s,t);*/

    LOGD("::interpretNALU %d %s prefix %d",nalu.data_length,nalu.get_nal_name(nalu.get_nal_unit_type()).c_str(),nalu.hasValidPrefix());

    /*h264_stream_t* h = h264_new();
    read_debug_nal_unit(h,const_cast<uint8_t*>(nalu.getDataWithoutPrefix()),nalu.getDataSizeWithoutPrefix());
    h264_free(h);

    try{
        std::this_thread::sleep_for(milliseconds(1000));
    }catch (...){
    }*/

    NALU* naluX=nullptr;
    /*if(nalu.isSPS()){

        LOGD("NALU DATA %s",nalu.dataAsString().c_str());

        h264_stream_t* h = h264_new();
        //read_debug_nal_unit(h,const_cast<uint8_t*>(nalu.getDataWithoutPrefix()),nalu.getDataSizeWithoutPrefix());
        read_nal_unit(h,const_cast<uint8_t*>(nalu.getDataWithoutPrefix()),nalu.getDataSizeWithoutPrefix());

        //LOGD("Nalu unit type %d",h->nal->nal_unit_type);
        //h->sps->vui_parameters_present_flag=0;
        h->sps->vui_parameters_present_flag=0;
        h->sps->constraint_set0_flag=1;
        h->sps->constraint_set1_flag=1;
        h->sps->num_ref_frames=3;


        std::vector<uint8_t> data(1024);
        int writeRet=write_nal_unit(h,data.data(),1024);
        data.insert(data.begin(),0);
        data.insert(data.begin(),0);
        data.insert(data.begin(),0);
        data.at(3)=1;
        writeRet+=3;


        LOGD("write ret %d",writeRet);
        NALU newNALU(data.data(),writeRet);
        LOGD("::newNALU %d %s prefix %d",newNALU.data_length,newNALU.get_nal_name().c_str(),nalu.hasValidPrefix());
        LOGD("NALU DATA %s",newNALU.dataAsString().c_str());


        read_debug_nal_unit(h,const_cast<uint8_t*>(newNALU.getDataWithoutPrefix()),newNALU.getDataSizeWithoutPrefix());

        h264_free(h);
        naluX=&newNALU;
        //naluX=new NALU(SPS_X264_NO_VUI,sizeof(SPS_X264_NO_VUI));
    }*/

    /*if(nalu.isPPS()){
        LOGD("NALU DATA %s",nalu.dataAsString().c_str());

        h264_stream_t* h = h264_new();
        //read_debug_nal_unit(h,const_cast<uint8_t*>(nalu.getDataWithoutPrefix()),nalu.getDataSizeWithoutPrefix());
        read_nal_unit(h,const_cast<uint8_t*>(nalu.getDataWithoutPrefix()),nalu.getDataSizeWithoutPrefix());

        //LOGD("Nalu unit type %d",h->nal->nal_unit_type);
        //h->sps->vui_parameters_present_flag=0;

        std::vector<uint8_t> data(1024);
        int writeRet=write_nal_unit(h,data.data(),1024);
        data.insert(data.begin(),0);
        data.insert(data.begin(),0);
        data.insert(data.begin(),0);
        data.at(3)=1;
        writeRet+=3;


        LOGD("write ret %d",writeRet);
        NALU newNALU(data.data(),writeRet);
        LOGD("::newNALU %d %s prefix %d",newNALU.data_length,newNALU.get_nal_name().c_str(),nalu.hasValidPrefix());
        LOGD("NALU DATA %s",newNALU.dataAsString().c_str());


        read_debug_nal_unit(h,const_cast<uint8_t*>(newNALU.getDataWithoutPrefix()),newNALU.getDataSizeWithoutPrefix());

        h264_free(h);
        naluX=&newNALU;

    }*/


    //we need this lock, since the receiving/parsing/feeding runs on its own thread, relative to the thread that creates / deletes the decoder
    std::lock_guard<std::mutex> lock(mMutexInputPipe);
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
        if(naluX!=nullptr){
            feedDecoder(*naluX,false);
        }else{
            feedDecoder(nalu,false);
        }
        decodingInfo.nNALUSFeeded++;
    } else{
        if(naluX!=nullptr){
            configureStartDecoder(*naluX);
        }else{
            configureStartDecoder(nalu);
        }
    }
}

//store sps / pps data. As soon as enough data has been buffered to initialize the decoder,
//Do so.
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
        decoder.codec = AMediaCodec_createCodecByName("OMX.google.h264.decoder");
    }else {
        decoder.codec = AMediaCodec_createDecoderByType("video/avc");
        //char* name;
        //AMediaCodec_getName(decoder.codec,&name);
        //LOGD("Created decoder %s",name);
        //AMediaCodec_releaseName(decoder.codec,name);
    }
    decoder.format=AMediaFormat_new();
    AMediaFormat_setString(decoder.format,AMEDIAFORMAT_KEY_MIME,"video/avc");
    int videoW,videoH;
    SPSHelper::ParseSPS(&CSDO[5],CSD0Length,&videoW,&videoH);
    LOGD("XYZ %d %d",videoW,videoH);

    //AMediaFormat_setInt32(decoder.format,AMEDIAFORMAT_KEY_FRAME_RATE,60);
    //AVCProfileBaseline==1
    //AMediaFormat_setInt32(decoder.format,AMEDIAFORMAT_KEY_PROFILE,1);
    //AMediaFormat_setInt32(decoder.format,AMEDIAFORMAT_KEY_PRIORITY,0);

    AMediaFormat_setInt32(decoder.format,AMEDIAFORMAT_KEY_WIDTH,videoW);
    AMediaFormat_setInt32(decoder.format,AMEDIAFORMAT_KEY_HEIGHT,videoH);

    AMediaFormat_setBuffer(decoder.format,"csd-0",&CSDO,(size_t)CSD0Length);
    AMediaFormat_setBuffer(decoder.format,"csd-1",&CSD1,(size_t)CSD1Length);

    AMediaCodec_configure(decoder.codec, decoder.format, decoder.window, nullptr, 0);
    if (decoder.codec== nullptr) {
        LOGD("Cannot configure decoder");
        //set csd-0 and csd-1 back to 0, maybe they were just faulty but we have better luck with the next ones
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
            LOGD("AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED %d %d %s",mWidth,mHeight,AMediaFormat_toString(format));
        } else if(index==AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED){
            LOGD("AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED");
        } else if(index==AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
            //LOGD("AMEDIACODEC_INFO_TRY_AGAIN_LATER");
        } else {
            LOGD("dequeueOutputBuffer idx: %d .Exit.",(int)index);
            return;
        }
        //every 2 seconds recalculate the current fps and bitrate
        const auto now=steady_clock::now();
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
    /*const bool isKeyFrame=nalu.data_length>0 &&( nalu.isSPS() || nalu.isSPS());
    if(isKeyFrame){
        return;
    }*/
    while(true){
        const auto index=AMediaCodec_dequeueInputBuffer(decoder.codec,BUFFER_TIMEOUT_US);
        if (index >=0) {
            size_t inputBufferSize;
            void* buf = AMediaCodec_getInputBuffer(decoder.codec,(size_t)index,&inputBufferSize);
            if(justEOS){
                AMediaCodec_queueInputBuffer(decoder.codec, (size_t)index, 0, (size_t)0,0, AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM);
            }else{
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
                LOGD("AMEDIACODEC_INFO_TRY_AGAIN_LATER for more than 1 second %d. return.",deltaMS);
                return;
            }
        }else{
            //Something went wrong. But we will feed the next nalu soon anyways
            LOGD("dequeueInputBuffer idx %d return.",(int)index);
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
        CSD0Length=0;
        CSD1Length=0;
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





