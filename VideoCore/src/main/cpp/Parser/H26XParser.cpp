//
// Created by Constantin on 24.01.2018.
//
#include "H26XParser.h"
#include <cstring>
#include <android/log.h>
#include <endian.h>
#include <chrono>
#include <thread>
#include <StringHelper.hpp>


H26XParser::H26XParser(NALU_DATA_CALLBACK onNewNALU):
        onNewNALU(std::move(onNewNALU)),
        mParseRAW(std::bind(&H26XParser::newNaluExtracted, this, std::placeholders::_1)),
        mDecodeRTP(std::bind(&H26XParser::onNewNaluDataExtracted, this, std::placeholders::_1,std::placeholders::_2,std::placeholders::_3)){
}

void H26XParser::reset(){
    mParseRAW.reset();
    mDecodeRTP.reset();
    nParsedNALUs=0;
    nParsedKonfigurationFrames=0;
    setLimitFPS(-1);
}

void H26XParser::parse_raw_h264_stream(const uint8_t *data, const size_t data_length) {
    mParseRAW.parseData(data,data_length);
}

void H26XParser::parse_raw_h265_stream(const uint8_t *data, const size_t data_length) {
    mParseRAW.parseData(data,data_length,true);
}

void H26XParser::parse_rtp_h264_stream(const uint8_t *rtp_data, const size_t data_length) {
    mDecodeRTP.parseRTPH264toNALU(rtp_data, data_length);
}

void H26XParser::parse_rtp_h265_stream(const uint8_t *rtp_data, const size_t data_length) {
    mDecodeRTP.parseRTPH265toNALU(rtp_data, data_length);
}

void H26XParser::parseDjiLiveVideoDataH264(const uint8_t *data, const size_t data_length) {
    mParseRAW.parseDjiLiveVideoDataH264(data,data_length);
}

void H26XParser::parseJetsonRawSlicedH264(const uint8_t *data, const size_t data_length) {
    mParseRAW.parseJetsonRawSlicedH264(data,data_length);
}

void H26XParser::setLimitFPS(int maxFPS1) {
    this->maxFPS=maxFPS1;
}

void H26XParser::onNewNaluDataExtracted(const std::chrono::steady_clock::time_point creation_time,
                                        const uint8_t *nalu_data, const int nalu_data_size) {
    NALU nalu(nalu_data,nalu_data_size, false,creation_time);
    newNaluExtracted(nalu);
}

void H26XParser::newNaluExtracted(const NALU& nalu) {
    //LOGD("H264Parser::newNaluExtracted");
    if(onNewNALU!= nullptr){
        onNewNALU(nalu);
    }
    nParsedNALUs++;
    const bool sps_or_pps=nalu.isSPS() || nalu.isPPS();
    if(sps_or_pps){
        nParsedKonfigurationFrames++;
    }
    //sps or pps NALUs do not count as frames, as well as AUD
    //E.g. they won't create a frame on the output pipe)
    const bool isNotARealFrame=nalu.IS_H265_PACKET ? (nalu.isSPS() || nalu.isPPS() || nalu.isVPS() || nalu.is_aud()) : (nalu.isSPS() || nalu.isPPS() || nalu.is_aud());
    if(!(isNotARealFrame)){
        mFrameLimiter.limitFps(maxFPS);
    }
}




