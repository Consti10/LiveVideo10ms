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
    // Returns false if something is wrong (packet should be discarded)
    bool validateRTPPacket(const rtp_header_t& rtpHeader);
    // parse rtp h24 packet to NALU
    void parseRTPtoNALU(const uint8_t* rtp_data, const size_t data_length);
    // parse rtp h265 packet to NALU
    void parseRTPH265toNALU(const uint8_t* rtp_data, const size_t data_length);
    // copy data into the NALU buffer and increase mNALU_DATA_LENGTH
    void copyNaluData(const uint8_t* data,size_t data_len);
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

/*********************************************
 ** Parses a stream of h264 NALUs into RTP packets
**********************************************/
class RTPEncoder{
public:
    struct RTPPacket{
        const uint8_t* data;
        const size_t data_len;
    };
    typedef std::function<void(const RTPPacket& rtpPacket)> RTP_DATA_CALLBACK;
public:
    /**
     * @param cb The callback that receives the RTP packets
     * @param RTP_PACKET_MAX_SIZE Maximum size of an RTP packet. The Max payload size will be slightly smaller
     */
    RTPEncoder(RTP_DATA_CALLBACK cb,const std::size_t RTP_PACKET_MAX_SIZE=1024):
    mCB(cb),
    RTP_PACKET_MAX_SIZE(RTP_PACKET_MAX_SIZE){
        assert(RTP_PACKET_MAX_SIZE<=RTPEncoder::SEND_BUF_SIZE);
    };
    // Set / change the callback
    void setCallback(RTP_DATA_CALLBACK cb){mCB=cb;};
    // Parse one NALU into one or more RTP packets
    int parseNALtoRTP(int framerate, const uint8_t *nalu_data,const size_t nalu_data_len);
    // If the NAL unit fits into one rtp packet the overhead is 12 bytes
    // Else, the overhead can be up to 12+2 bytes
    static constexpr std::size_t RTP_PACKET_MAX_OVERHEAD=12+2;
    const std::size_t RTP_PACKET_MAX_SIZE;
    const std::size_t RTP_PAYLOAD_MAX_SIZE=RTP_PACKET_MAX_SIZE-RTP_PACKET_MAX_OVERHEAD;
private:
    RTP_DATA_CALLBACK mCB;
    void forwardRTPPacket(uint8_t *rtp_packet, size_t rtp_packet_len);
    // This buffer size does not affect the RTP packet size
    // I allocate a big buffer here to account for all RTP packet sizes of up to 1024*1024 bytes
    static constexpr const std::size_t SEND_BUF_SIZE=1024*1024;
    uint8_t mRTP_BUFF_SEND[SEND_BUF_SIZE];
    uint16_t seq_num = 0;
    uint32_t ts_current = 0;
};

class TestEncodeDecodeRTP{
private:
    void onNALU(const NALU& nalu);
    void onRTP(const RTPEncoder::RTPPacket& packet);
    std::unique_ptr<RTPEncoder> encoder;
    std::unique_ptr<RTPDecoder> decoder;
    std::unique_ptr<NALU> lastNALU;
public:
    TestEncodeDecodeRTP();
    // This encodes the nalu to RTP then decodes it again
    // After that, check that their contents match
    void testEncodeDecodeRTP(const NALU& nalu);
};

#endif //LIVE_VIDEO_10MS_ANDROID_PARSERTP_H
