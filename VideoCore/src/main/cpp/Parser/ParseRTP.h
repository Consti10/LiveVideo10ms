//
// Created by Constantin on 2/6/2019.
//

#ifndef LIVE_VIDEO_10MS_ANDROID_PARSERTP_H
#define LIVE_VIDEO_10MS_ANDROID_PARSERTP_H

#include <cstdio>
#include "../NALU/NALU.hpp"

/*********************************************
 ** Parses a stream of rtp h264 data into NALUs
**********************************************/
class ParseRTP{
public:
    ParseRTP(NALU_DATA_CALLBACK cb);
public:
    //Decoding
    void parseRTPtoNALU(const uint8_t* rtp_data, const size_t data_length);
    void reset();
private:
    const NALU_DATA_CALLBACK cb;
    std::array<uint8_t,NALU::NALU_MAXLEN> BUFF_NALU_DATA;
    size_t BUFF_NALU_DATA_LENGTH=0;
};

/*********************************************
 ** Parses a stream of h264 NALUs into RTP packets
**********************************************/
class EncodeRTP{
public:
    struct RTPPacket{
        const uint8_t* data;
        const size_t data_len;
    };
    typedef std::function<void(const RTPPacket& rtpPacket)> RTP_DATA_CALLBACK;
public:
    EncodeRTP(RTP_DATA_CALLBACK cb):mCB(cb){};
    int parseNALtoRTP(int framerate, const uint8_t *nalu_data,const size_t nalu_data_len);
private:
    RTP_DATA_CALLBACK mCB;
    void forwardRTPPacket(uint8_t *rtp_packet, size_t rtp_packet_len);
    //
    static constexpr std::size_t RTP_PAYLOAD_MAX_SIZE=1024;
    static constexpr std::size_t RTP_PACKET_MAX_SIZE=RTP_PAYLOAD_MAX_SIZE+1024;
    static constexpr std::size_t SEND_BUF_SIZE=RTP_PAYLOAD_MAX_SIZE+1024;
    uint8_t RTP_BUFF_SEND[SEND_BUF_SIZE];
    uint16_t seq_num = 0;
    uint32_t ts_current = 0;
};

class TestEncodeDecodeRTP{
private:

};

#endif //LIVE_VIDEO_10MS_ANDROID_PARSERTP_H
