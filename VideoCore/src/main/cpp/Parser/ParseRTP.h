//
// Created by Constantin on 2/6/2019.
//

#ifndef LIVE_VIDEO_10MS_ANDROID_PARSERTP_H
#define LIVE_VIDEO_10MS_ANDROID_PARSERTP_H

#include <cstdio>
#include "../NALU/NALU.hpp"
#include "RTP.hpp"

/*********************************************
 ** Parses a stream of rtp h26X data into NALUs
**********************************************/
class RTPDecoder{
public:
    RTPDecoder(NALU_DATA_CALLBACK cb);
public:
    // check if a packet is missing by using the rtp sequence number and
    // if the payload is dynamic (h264 or h265)
    // Returns false if payload is wrong
    // sets the 'missing packet' flag to true if packet got lost
    bool validateRTPPacket(const rtp_header_t& rtpHeader);
    // parse rtp h264 packet to NALU
    void parseRTPH264toNALU(const uint8_t* rtp_data, const size_t data_length);
    // parse rtp h265 packet to NALU
    void parseRTPH265toNALU(const uint8_t* rtp_data, const size_t data_length);
    // copy data_len bytes into the mNALU_DATA buffer at the current position
    // and increase mNALU_DATA_LENGTH by data_len
    void appendNALUData(const uint8_t* data, size_t data_len);
    // reset mNALU_DATA_LENGTH to 0
    void reset();
private:
    // Properly calls the cb function
    // Resets the mNALU_DATA_LENGTH to 0
    void forwardNALU(const std::chrono::steady_clock::time_point creationTime,const bool isH265=false);
    const NALU_DATA_CALLBACK cb;
    std::array<uint8_t,NALU::NALU_MAXLEN> mNALU_DATA;
    size_t mNALU_DATA_LENGTH=0;
private:
    //TDOD: What shall we do if a start, middle or end of fu-a is missing ?
    int lastSequenceNumber=-1;
    bool flagPacketHasGoneMissing=false;
    // This time point is as 'early as possible' to debug the parsing time as accurately as possible.
    // E.g for a fu-a NALU the time point when the start fu-a was received, not when its end is received
    std::chrono::steady_clock::time_point timePointStartOfReceivingNALU;
};

#endif //LIVE_VIDEO_10MS_ANDROID_PARSERTP_H
