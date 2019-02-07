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


class H264Parser {
public:
    H264Parser(NALU_DATA_CALLBACK onNewNALU);
    void parse_raw_h264_stream(const uint8_t* data,const int data_length);
    void parse_rtp_h264_stream(const uint8_t* rtp_data,const int data_len);
    void reset();
public:
    long nParsedNALUs=0;
    long nParsedKeyFrames=0;
    bool limitFPS=false;
private:
    void newNaluExtracted(const NALU& nalu);
    const NALU_DATA_CALLBACK onNewNALU;

    std::chrono::steady_clock::time_point lastFrameLimitFPS=std::chrono::steady_clock::now();

    ParseRAW mParseRAW;
    ParseRTP mParseRTP;
};

#endif //FPV_VR_PARSE2H264RAW_H
