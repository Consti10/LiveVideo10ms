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
    this->maxFPS=maxFPS;
}

void H264Parser::newNaluExtracted(const NALU& nalu) {
    using namespace std::chrono;
    //LOGD("H264Parser::newNaluExtracted");
    if(onNewNALU!= nullptr){
        onNewNALU(nalu);
    }
    nParsedNALUs++;
    const bool sps_or_pps=nalu.isSPS() || nalu.isPPS();
    if(sps_or_pps){
        nParsedKeyFrames++;
    }
    //sps or pps NALUs do not count as frames, as well as AUD
    //E.g. they won't create a frame on the output pipe)
    if(!(sps_or_pps || nalu.get_nal_unit_type()==NALU::NAL_UNIT_TYPE_AUD)){
        mFrameLimiter.limitFps(maxFPS);
    }
}



