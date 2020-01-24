//
// Created by Constantin on 24.01.2018.
//
#include "H264Parser.h"
#include <cstring>
#include <android/log.h>
#include <endian.h>
#include <chrono>
#include <thread>

constexpr auto TAG="H264Parser";
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)


H264Parser::H264Parser(NALU_DATA_CALLBACK onNewNALU):
    onNewNALU(onNewNALU),
    mParseRAW(std::bind(&H264Parser::newNaluExtracted, this, std::placeholders::_1)),
    mParseRTP(std::bind(&H264Parser::newNaluExtracted, this, std::placeholders::_1)){
}

void H264Parser::reset(){
    mParseRAW.reset();
    mParseRTP.reset();
    nParsedNALUs=0;
    nParsedKeyFrames=0;
    setLimitFPS(-1);
}

void H264Parser::parse_raw_h264_stream(const uint8_t *data,const  int data_length) {
    //LOGD("H264Parser::parse_raw_h264_stream %d",data_length);
    mParseRAW.parseData(data,data_length);
}

void H264Parser::parse_rtp_h264_stream(const uint8_t *rtp_data,const  int data_length) {
    mParseRTP.parseData(rtp_data,data_length);
}

void H264Parser::setLimitFPS(int maxFPS) {
    //Disable if maxFPS<=0 (e.g. live video) or maxFPS>240 (in this case, the bit rate becomes the limiting factor anyway)
    if(maxFPS<=0 || maxFPS>240){
       limitFPS=false;
    }else{
        limitFPS=true;
        minTimeBetweenFramesIfEnabled=1000.0f/((float)maxFPS);
    }
}

void H264Parser::newNaluExtracted(const NALU& nalu) {
    using namespace std::chrono;
    //LOGD("H264Parser::newNaluExtracted");
    if(onNewNALU!= nullptr){
        onNewNALU(nalu);
    }
    nParsedNALUs++;
    if(nalu.isSPS() || nalu.isPPS()){
        nParsedKeyFrames++;
    }
    //Wait until at least n ms are elapsed since the last call to onNewNALU
    if(limitFPS){
        //LOGD("Min frame time of %f",minTimeBetweenFramesIfEnabled);
        const auto minimumTimeBetweenMS=minTimeBetweenFramesIfEnabled;
        while(true){
            const auto now=steady_clock::now();
            const auto deltaSinceLastFrame=now-lastTimeOnNewNALUCalled;
            const int64_t deltaSinceLastFrameMS=duration_cast<milliseconds>(deltaSinceLastFrame).count();
            if(deltaSinceLastFrameMS>=minimumTimeBetweenMS){
                break;
            }
        }
        lastTimeOnNewNALUCalled=steady_clock::now();
        /*const auto now=steady_clock::now();
        const auto deltaSinceLastFrame=now-lastFrameLimitFPS;
        const int64_t waitTimeMS=32-duration_cast<milliseconds>(deltaSinceLastFrame).count();
        lastFrameLimitFPS=now;
        if(waitTimeMS>0){
            try{
                LOGD("Sleeping for %d",waitTimeMS);
                std::this_thread::sleep_for(milliseconds(waitTimeMS));
            }catch (...){
            }
        }*/
    }
    //LOGD("length: %d type: %s",data_length,get_nal_name(get_nal_unit_type(data,data_length)).c_str());
}



