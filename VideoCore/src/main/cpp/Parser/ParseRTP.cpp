//
// Created by Constantin on 2/6/2019.
//

#include "ParseRTP.h"
#include <AndroidLogger.hpp>

RTPDecoder::RTPDecoder(NALU_DATA_CALLBACK cb): cb(std::move(cb)){
}

void RTPDecoder::reset(){
    mNALU_DATA_LENGTH=0;
    //nalu_data.reserve(NALU::NALU_MAXLEN);
}

bool RTPDecoder::validateRTPPacket(const rtp_header_t& rtp_header) {
    if(rtp_header.payload!=RTP_PAYLOAD_TYPE_H264_H265){
        MLOGE<<"Unsupported payload type "<<(int)rtp_header.payload;
        //return false;
    }
    // Testing regarding sequence numbers.This stuff can be removed without issues
    const int seqNr=rtp_header.getSequence();
    if(seqNr==lastSequenceNumber){
        // duplicate. This should never happen for 'normal' rtp streams, but can be usefully when testing bitrates
        // (Since you can send the same packet multiple times to emulate a higher bitrate)
        MLOGD<<"Same seqNr";
        return false;
    }
    if(lastSequenceNumber==-1){
        // first packet in stream
        flagPacketHasGoneMissing=false;
    }else{
        // Don't forget that the sequence number loops every UINT16_MAX packets
        if(seqNr != ((lastSequenceNumber+1) % UINT16_MAX)){
            // We are missing a Packet !
            MLOGD<<"missing a packet. Last:"<<lastSequenceNumber<<" Curr:"<<seqNr<<" Diff:"<<(seqNr-(int)lastSequenceNumber);
            //flagPacketHasGoneMissing=true;
        }
    }
    lastSequenceNumber=seqNr;
    return true;
}

void RTPDecoder::parseRTPH264toNALU(const uint8_t* rtp_data, const size_t data_length){
    //12 rtp header bytes and 1 nalu_header_t type byte
    if(data_length <= sizeof(rtp_header_t)+sizeof(nalu_header_t)){
        MLOGE<<"Not enough rtp data";
        return;
    }
    //MLOGD<<"Got rtp data";
    const RTP::RTPPacketH264 rtpPacket(rtp_data,data_length);
    //MLOGD<<"RTP Header: "<<rtp_header->asString();
    if(!validateRTPPacket(rtpPacket.header)){
        return;
    }
    const auto& nalu_header=rtpPacket.getNALUHeaderH264();
    if (nalu_header.type == 28) { /* FU-A */
        //MLOGD<<"Got partial NALU";
        const auto& fu_header=rtpPacket.getFuHeader();
        const auto fu_payload=rtpPacket.getFuPayload();
        const auto fu_payload_size=rtpPacket.getFuPayloadSize();
        if (fu_header.e == 1) {
            //MLOGD<<"End of fu-a";
            /* end of fu-a */
            appendNALUData(fu_payload, fu_payload_size);
            if(!flagPacketHasGoneMissing){
                // To better measure latency we can actually use the timestamp from when the first bytes for this packet were received
                forwardNALU(timePointStartOfReceivingNALU);
            }
            mNALU_DATA_LENGTH=0;
        } else if (fu_header.s == 1) {
            //MLOGD<<"Start of fu-a";
            timePointStartOfReceivingNALU=std::chrono::steady_clock::now();
            // Beginning of new fu sequence - we can remove the 'drop packet' flag
            if(flagPacketHasGoneMissing){
                MLOGD<<"Got fu-a start - clearing missing packet flag";
                flagPacketHasGoneMissing=false;
            }
            /* start of fu-a */
            mNALU_DATA[0]=0;
            mNALU_DATA[1]=0;
            mNALU_DATA[2]=0;
            mNALU_DATA[3]=1;
            mNALU_DATA_LENGTH=4;
            const uint8_t h264_nal_header = (uint8_t)(fu_header.type & 0x1f)
                                            | (nalu_header.nri << 5)
                                            | (nalu_header.f << 7);
            mNALU_DATA[4]=h264_nal_header;
            mNALU_DATA_LENGTH++;
            appendNALUData(fu_payload, fu_payload_size);
        } else {
            //MLOGD<<"Middle of fu-a";
            /* middle of fu-a */
            appendNALUData(fu_payload, fu_payload_size);
        }
    } else if(nalu_header.type>0 && nalu_header.type<24){
        //MLOGD<<"Got full nalu";
        timePointStartOfReceivingNALU=std::chrono::steady_clock::now();
        // Full NALU - we can remove the 'drop packet' flag
        if(flagPacketHasGoneMissing){
            MLOGD<<"Got full NALU - clearing missing packet flag";
            flagPacketHasGoneMissing= false;
        }
        /* full nalu */
        mNALU_DATA[0]=0;
        mNALU_DATA[1]=0;
        mNALU_DATA[2]=0;
        mNALU_DATA[3]=1;
        mNALU_DATA_LENGTH=4;
        const uint8_t h264_nal_header = (uint8_t )(nalu_header.type & 0x1f)
                                        | (nalu_header.nri << 5)
                                        | (nalu_header.f << 7);
        mNALU_DATA[4]=h264_nal_header;
        mNALU_DATA_LENGTH++;
        memcpy(&mNALU_DATA[mNALU_DATA_LENGTH], &rtp_data[13], (size_t)data_length - 13);
        mNALU_DATA_LENGTH+= data_length - 13;
        forwardNALU(timePointStartOfReceivingNALU);
        mNALU_DATA_LENGTH=0;
    }else{
        MLOGE<<"Got unsupported H264 RTP packet. NALU type:"<<nalu_header.type;
    }
}

// https://github.com/ireader/media-server/blob/master/librtp/payload/rtp-h265-unpack.c

#define FU_START(v) (v & 0x80)
#define FU_END(v)	(v & 0x40)
#define FU_NAL(v)	(v & 0x3F)

#include <StringHelper.hpp>
void RTPDecoder::parseRTPH265toNALU(const uint8_t* rtp_data, const size_t data_length){
    // 12 rtp header bytes and 1 nalu_header_t type byte
    if(data_length <= sizeof(rtp_header_t)+sizeof(nal_unit_header_h265_t)){
        MLOGE<<"Not enough rtp data";
        return;
    }
    //MLOGD<<"Got rtp data";
    const RTP::RTPPacketH265 rtpPacket(rtp_data,data_length);
    //MLOGD<<"RTP Header: "<<rtp_header->asString();
    if(!validateRTPPacket(rtpPacket.header)){
        return;
    }
    const auto& nal_unit_header_h265=rtpPacket.getNALUHeaderH265();
    if (nal_unit_header_h265.type > 50){
        MLOGE<<"Unsupported (HEVC) NAL type";
        return;
    }
    if(nal_unit_header_h265.type==48){
        // Aggregation packets are not supported yet (and never used in gstreamer / ffmpeg anyways)
        MLOGE<<"Unsupported RTP H265 packet (Aggregation packet)";
        return;
    }else if(nal_unit_header_h265.type==49){
        // FU-X packet
        //MLOGD<<"Got partial nal";
        const auto& fu_header=rtpPacket.getFuHeader();
        const auto fu_payload=rtpPacket.getFuPayload();
        const auto fu_payload_size=rtpPacket.getFuPayloadSize();
        if(fu_header.e){
            //MLOGD<<"end of fu packetization";
            appendNALUData(fu_payload, fu_payload_size);
            forwardNALU(timePointStartOfReceivingNALU,true);
            mNALU_DATA_LENGTH=0;
        }else if(fu_header.s){
            //MLOGD<<"start of fu packetization";
            //MLOGD<<"Bytes "<<StringHelper::vectorAsString(std::vector<uint8_t>(rtp_data,rtp_data+data_length));
            timePointStartOfReceivingNALU=std::chrono::steady_clock::now();
            if(flagPacketHasGoneMissing){
                MLOGD<<"Got fu-a start - clearing missing packet flag";
                flagPacketHasGoneMissing=false;
            }
            mNALU_DATA[0]=0;
            mNALU_DATA[1]=0;
            mNALU_DATA[2]=0;
            mNALU_DATA[3]=1;
            mNALU_DATA_LENGTH=4;
            // copy header and reconstruct ?!!!
            const uint8_t* ptr=&rtp_data[sizeof(rtp_header_t)];
            uint8_t variableNoIdea=rtp_data[sizeof(rtp_header_t) + sizeof(nal_unit_header_h265_t)];
            // replace NAL Unit Type Bits - I have no idea how that works, but this manipulation works :)
            mNALU_DATA[mNALU_DATA_LENGTH] = (FU_NAL(variableNoIdea) << 1) | (ptr[0] & 0x81);
            mNALU_DATA_LENGTH++;
            mNALU_DATA[mNALU_DATA_LENGTH] = ptr[1];
            mNALU_DATA_LENGTH++;
            // copy the rest of the data
            appendNALUData(fu_payload, fu_payload_size);
        }else{
            //MLOGD<<"middle of fu packetization";
            appendNALUData(fu_payload, fu_payload_size);
        }
    }else{
        // single NAL unit
        //MLOGD<<"Got single nal";
        if(flagPacketHasGoneMissing){
            MLOGD<<"Got full NALU - clearing missing packet flag";
            flagPacketHasGoneMissing= false;
        }
        mNALU_DATA[0]=0;
        mNALU_DATA[1]=0;
        mNALU_DATA[2]=0;
        mNALU_DATA[3]=1;
        mNALU_DATA_LENGTH=4;
        // I do not know what about the 'DONL' field but it seems to be never present
        // copy the NALU header and NALU data, other than h264 here nothing has to be 'reconstructed'
        appendNALUData(rtpPacket.rtpPayload, rtpPacket.rtpPayloadSize);
        forwardNALU(std::chrono::steady_clock::now(),true);
        mNALU_DATA_LENGTH=0;
    }
}

void RTPDecoder::forwardNALU(const std::chrono::steady_clock::time_point creationTime,const bool isH265) {
    if(cb!= nullptr){
        const size_t minNaluSize=NALU::getMinimumNaluSize(isH265);
        if(mNALU_DATA_LENGTH>=minNaluSize){
            NALU nalu(mNALU_DATA, mNALU_DATA_LENGTH,isH265,creationTime);
            //MLOGD<<"NALU type "<<nalu.get_nal_name();
            //MLOGD<<"DATA:"<<nalu.dataAsString();
            //nalu_data.resize(nalu_data_length);
            //NALU nalu(nalu_data);
            cb(nalu);
        }else{
            MLOGE<<"RTP parser produced NALU with insufficient data size";
        }
    }
    mNALU_DATA_LENGTH=0;
}


void RTPDecoder::appendNALUData(const uint8_t *data, size_t data_len) {
    memcpy(&mNALU_DATA[mNALU_DATA_LENGTH],data,data_len);
    mNALU_DATA_LENGTH+=data_len;
}



