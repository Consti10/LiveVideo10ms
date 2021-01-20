//
// Created by Constantin on 24.01.2018.
//

#ifndef FPV_VR_PARSE2H264RAW_H
#define FPV_VR_PARSE2H264RAW_H

#include <functional>
#include <sstream>

/**
 * Input:
 * 1) rtp packets (h264/h265)
 * 2) raw packets (h264/h265)
 * Output:
 * NAL units in the onNewNalu callback, one after another
 */
//

#include "../NALU/NALU.hpp"

#include "ParseRAW.h"
#include "ParseRTP.h"

#include "FrameLimiter.hpp"
//
#include <map>
#include <list>
#include <TimeHelper.hpp>
#include <wifibroadcast/fec.hh>
//
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
//#include <rtpdec.h>
}

class H26XParser {
public:
    H26XParser(NALU_DATA_CALLBACK onNewNALU);
    void parse_raw_h264_stream(const uint8_t* data,const size_t data_length);
    void parse_raw_h265_stream(const uint8_t* data,const size_t data_length);
    void parse_rtp_h264_stream(const uint8_t* rtp_data,const size_t data_len);
    void parse_rtp_h265_stream(const uint8_t* rtp_data,const size_t data_len);
    //void parse_rtp_h264_stream_ffmpeg(const uint8_t* rtp_data,const size_t data_len);
    void parseDjiLiveVideoDataH264(const uint8_t* data,const size_t data_len);
    void parseJetsonRawSlicedH264(const uint8_t* data,const size_t data_len);
    //
    void parseCustom(const uint8_t* data,const size_t data_len);
    void parseCustomRTPinsideFEC(const uint8_t* data, const size_t data_len);
    void reset();
public:
    long nParsedNALUs=0;
    long nParsedKonfigurationFrames=0;
    //For live video set to -1 (no fps limitation), else additional latency will be generated
    void setLimitFPS(int maxFPS);
private:
    void newNaluExtracted(const NALU& nalu);
    const NALU_DATA_CALLBACK onNewNALU;
    std::chrono::steady_clock::time_point lastFrameLimitFPS=std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point lastTimeOnNewNALUCalled=std::chrono::steady_clock::now();
    ParseRAW mParseRAW;
    RTPDecoder mDecodeRTP;

    FrameLimiter mFrameLimiter;
    int maxFPS=0;
    //First time a NALU was succesfully decoded
    //std::chrono::steady_clock::time_point timeFirstNALUArrived=std::chrono::steady_clock::time_point(0);
    // Custom stuff
    std::vector<uint32_t> sequenceNumbers;
    struct CustomUdpPacket{
        uint32_t sequenceNumber;
        const uint8_t* data;
        size_t dataLength;
    };
    struct XPacket{
        uint32_t sequenceNumber;
        std::vector<uint8_t> data;
    };
    std::list<XPacket> bufferedPackets;
    int lastForwardedSequenceNr=-1;
    void debugSequenceNumbers(const uint32_t seqNr);
    uint32_t debugLastSequenceNumber;
    // To account for the rare case of restarting the tx I keep track of the n of dropped pack
    int droppedPacketsSinceLastForwardedPacket=0;
    //
    AvgCalculatorSize avgUDPPacketSize;
    //
    FECDecoder mFECDecoder;
    //
    std::vector<std::size_t> receivedDataPacketsSize;
};

#endif //FPV_VR_PARSE2H264RAW_H
