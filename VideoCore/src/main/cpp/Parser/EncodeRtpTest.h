//
// Created by consti10 on 18.03.23.
//

#ifndef LIVEVIDEO10MS_ENCODERTPTEST_H
#define LIVEVIDEO10MS_ENCODERTPTEST_H

#include <cstdio>
#include "../NALU/NALU.hpp"
#include "RTP.hpp"
#include "ParseRTP.h"

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

/*class TestEncodeDecodeRTP{
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
};*/


#endif //LIVEVIDEO10MS_ENCODERTPTEST_H
