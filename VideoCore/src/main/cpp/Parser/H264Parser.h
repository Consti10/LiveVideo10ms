//
// Created by Constantin on 24.01.2018.
//

#ifndef FPV_VR_PARSE2H264RAW_H
#define FPV_VR_PARSE2H264RAW_H

#include <functional>
#include <sstream>

/**
 * Input:
 * 1) rtp packets
 * 2) raw h.264 byte stream packets
 * Output:
 * h.264 NAL units in the onNewNalu callback, one after another
 */
//

#include "../NALU/NALU.hpp"

#include "ParseRAW.h"
#include "ParseRTP.h"

#include "FrameLimiter.h"

class H264Parser {
public:
    H264Parser(NALU_DATA_CALLBACK onNewNALU);
    void parse_raw_h264_stream(const uint8_t* data,const size_t data_length);
    void parse_rtp_h264_stream(const uint8_t* rtp_data,const size_t data_len);
    void reset();
public:
    long nParsedNALUs=0;
    long nParsedKeyFrames=0;
    //For live video set to -1 (no fps limitation), else additional latency will be generated
    void setLimitFPS(int maxFPS);
private:
    void newNaluExtracted(const NALU& nalu);
    const NALU_DATA_CALLBACK onNewNALU;
    std::chrono::steady_clock::time_point lastFrameLimitFPS=std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point lastTimeOnNewNALUCalled=std::chrono::steady_clock::now();
    ParseRAW mParseRAW;
    ParseRTP mParseRTP;
    FrameLimiter mFrameLimiter;
    int maxFPS;
    //First time a NALU was succesfully decoded
    //std::chrono::steady_clock::time_point timeFirstNALUArrived=std::chrono::steady_clock::time_point(0);
};

#endif //FPV_VR_PARSE2H264RAW_H
