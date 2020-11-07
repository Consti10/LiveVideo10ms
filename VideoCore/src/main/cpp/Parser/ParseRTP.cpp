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
        return false;
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

void RTPDecoder::parseRTPtoNALU(const uint8_t* rtp_data, const size_t data_length){
    //12 rtp header bytes and 1 nalu_header_t type byte
    if(data_length <= sizeof(rtp_header_t)+sizeof(nalu_header_t)){
        MLOGE<<"Not enough rtp data";
        return;
    }
    //MLOGD<<"Got rtp data";
    const auto* rtp_header=(rtp_header_t*)&rtp_data[0];
    //MLOGD<<"RTP Header: "<<rtp_header->asString();
    if(!validateRTPPacket(*rtp_header)){
        return;
    }
    const auto* nalu_header=(nalu_header_t *)&rtp_data[sizeof(rtp_header_t)];
    if (nalu_header->type == 28) { /* FU-A */
        //MLOGD<<"Got partial NALU";
        const fu_header_t* fu_header = (fu_header_t*)&rtp_data[13];
        if (fu_header->e == 1) {
            //MLOGD<<"End of fu-a";
            /* end of fu-a */
            appendNALUData(&rtp_data[14], (size_t) data_length - 14);
            if(!flagPacketHasGoneMissing){
                // To better measure latency we can actually use the timestamp from when the first bytes for this packet were received
                forwardNALU(timePointStartOfReceivingNALU);
            }
            mNALU_DATA_LENGTH=0;
        } else if (fu_header->s == 1) {
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
            const uint8_t h264_nal_header = (uint8_t)(fu_header->type & 0x1f)
                                            | (nalu_header->nri << 5)
                                            | (nalu_header->f << 7);
            mNALU_DATA[4]=h264_nal_header;
            mNALU_DATA_LENGTH++;
            appendNALUData(&rtp_data[14], (size_t) data_length - 14);
        } else {
            //MLOGD<<"Middle of fu-a";
            /* middle of fu-a */
            appendNALUData(&rtp_data[14], (size_t) data_length - 14);
        }
    } else if(nalu_header->type>0 && nalu_header->type<24){
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
        const uint8_t h264_nal_header = (uint8_t )(nalu_header->type & 0x1f)
                                        | (nalu_header->nri << 5)
                                        | (nalu_header->f << 7);
        mNALU_DATA[4]=h264_nal_header;
        mNALU_DATA_LENGTH++;
        memcpy(&mNALU_DATA[mNALU_DATA_LENGTH], &rtp_data[13], (size_t)data_length - 13);
        mNALU_DATA_LENGTH+= data_length - 13;
        forwardNALU(timePointStartOfReceivingNALU);
        mNALU_DATA_LENGTH=0;
    }else{
        //MLOGD<<"header:"<<nalu_header->type;
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
    const auto* rtp_header=(rtp_header_t*)&rtp_data[0];
    //MLOGD<<"RTP Header: "<<rtp_header->asString();
    if(!validateRTPPacket(*rtp_header)){
        return;
    }
    const auto* nal_unit_header_h265=(nal_unit_header_h265_t*)&rtp_data[sizeof(rtp_header_t)];
    if (nal_unit_header_h265->type > 50){
        MLOGE<<"Unsupported (HEVC) NAL type";
        return;
    }
    if(nal_unit_header_h265->type==48){
        // Aggregation packets are not supported yet (and never used in gstreamer / ffmpeg anyways)
        MLOGE<<"Unsupprted RTP H265 packet (Aggregation packet)";
        return;
    }else if(nal_unit_header_h265->type==49){
        // FU-X packet
        //MLOGD<<"Got partial nal";
        const auto fu_header=(fu_header_h265_t*)&rtp_data[sizeof(rtp_header_t) + sizeof(nal_unit_header_h265_t)];
        const auto fuPayloadOffset= sizeof(rtp_header_t) + sizeof(nal_unit_header_h265_t) + sizeof(fu_header_h265_t);
        const uint8_t* fu_payload=&rtp_data[fuPayloadOffset];
        const size_t fu_payload_size= data_length - fuPayloadOffset;
        if(fu_header->e){
            //MLOGD<<"end of fu packetization";
            appendNALUData(fu_payload, fu_payload_size);
            forwardNALU(timePointStartOfReceivingNALU,true);
            mNALU_DATA_LENGTH=0;
        }else if(fu_header->s){
            //MLOGD<<"start of fu packetization";
            //MLOGD<<"Bytes "<<StringHelper::vectorAsString(std::vector<uint8_t>(rtp_data,rtp_data+data_length));
            timePointStartOfReceivingNALU=std::chrono::steady_clock::now();
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
        mNALU_DATA[0]=0;
        mNALU_DATA[1]=0;
        mNALU_DATA[2]=0;
        mNALU_DATA[3]=1;
        mNALU_DATA_LENGTH=4;
        // I do not know what about the 'DONL' field but it seems to be never present
        // copy the NALU header and NALU data, other than h264 here I do not have to reconstruct anything ?!
        const auto* nalUnitPayloadData=&rtp_data[sizeof(rtp_header_t)];
        const size_t nalUnitPayloadDataSize= data_length-sizeof(rtp_header_t);
        appendNALUData(nalUnitPayloadData, nalUnitPayloadDataSize);
        forwardNALU(std::chrono::steady_clock::now(),true);
        mNALU_DATA_LENGTH=0;
    }
}

void RTPDecoder::forwardNALU(const std::chrono::steady_clock::time_point creationTime,const bool isH265) {
    if(cb!= nullptr){
        NALU nalu(mNALU_DATA, mNALU_DATA_LENGTH,isH265,creationTime);
        //MLOGD<<"NALU type "<<nalu.get_nal_name();
        //MLOGD<<"DATA:"<<nalu.dataAsString();
        //nalu_data.resize(nalu_data_length);
        //NALU nalu(nalu_data);
        cb(nalu);
    }
    mNALU_DATA_LENGTH=0;
}


void RTPDecoder::appendNALUData(const uint8_t *data, size_t data_len) {
    memcpy(&mNALU_DATA[mNALU_DATA_LENGTH],data,data_len);
    mNALU_DATA_LENGTH+=data_len;
}



// xxxxxxxxxxxxxxxxxxxxxxxxxxx RTPEncoder part xxxxxxxxxxxxxxxxxxxxxxxxxxx

int RTPEncoder::parseNALtoRTP(int framerate, const uint8_t *nalu_data, const size_t nalu_data_len) {
    // Watch out for not enough data (else algorithm might crash)
    if(nalu_data_len <= 5){
        return -1;
    }
    // Prefix is the 0,0,0,1. RTP does not use it
    const uint8_t *nalu_buf_without_prefix = &nalu_data[4];
    const size_t nalu_len_without_prefix= nalu_data_len - 4;

    ts_current += (90000 / framerate);  /* 90000 / 25 = 3600 */

    if (nalu_len_without_prefix <= RTP_PAYLOAD_MAX_SIZE) {
        /*
         * single nal unit
         */
        memset(mRTP_BUFF_SEND, 0, sizeof(rtp_header_t) + sizeof(nalu_header_t));
        /**
         * Set pointer for headers
         */
        auto* rtp_hdr= (rtp_header_t *)mRTP_BUFF_SEND;
        auto* nalu_hdr = (nalu_header_t *)&mRTP_BUFF_SEND[sizeof(rtp_header_t)];
        /**
         * Write rtp header
         */
        rtp_hdr->cc = 0;
        rtp_hdr->extension = 0;
        rtp_hdr->padding = 0;
        rtp_hdr->version = 2;
        rtp_hdr->payload = RTP_PAYLOAD_TYPE_H264_H265;
        // rtp_hdr->marker = (pstStream->u32PackCount - 1 == i) ? 1 : 0;   /* If the packet is the end of a frame, set it to 1, otherwise it is 0. rfc 1889 does not specify the purpose of this bit*/
        rtp_hdr->marker=0;
        rtp_hdr->sequence = htons(++seq_num % UINT16_MAX);
        rtp_hdr->timestamp = htonl(ts_current);
        //rtp_hdr->timestamp=0;
        rtp_hdr->sources = htonl(MY_SSRC_NUM);
        /*
         * Set rtp load single nal unit header
         */
        nalu_hdr->f = (nalu_buf_without_prefix[0] & 0x80) >> 7;        /* bit0 */
        nalu_hdr->nri = (nalu_buf_without_prefix[0] & 0x60) >> 5;      /* bit1~2 */
        nalu_hdr->type = (nalu_buf_without_prefix[0] & 0x1f);
        //MLOGD<<"ENC NALU hdr type"<<((int)nalu_hdr->type);
        /*
         * 3.Fill nal content
         */
        memcpy(mRTP_BUFF_SEND + 13, nalu_buf_without_prefix + 1, nalu_len_without_prefix - 1);    /* 不拷贝nalu头 */
        /*
         * 4. Forward the RTP packet
         */
        const size_t len_sendbuf = 12 + nalu_len_without_prefix;
        forwardRTPPacket(mRTP_BUFF_SEND, len_sendbuf);
        //
        //MLOGD<<"NALU <RTP_PAYLOAD_MAX_SIZE";
    } else {    /* nalu_len > RTP_PAYLOAD_MAX_SIZE */
        //MLOGD<<"NALU >RTP_PAYLOAD_MAX_SIZE";
        //assert(false);
        /*
         * FU-A segmentation
         */
        /*
         * 1. Count the number of divisions
         *
         * Except for the last shard，
         * Consumption per shard RTP_PAYLOAD_MAX_SIZE BYTE
         */
        /* The number of splits when nalu needs to be split to send */
        const int fu_pack_num = nalu_len_without_prefix % RTP_PAYLOAD_MAX_SIZE ? (nalu_len_without_prefix / RTP_PAYLOAD_MAX_SIZE + 1) : nalu_len_without_prefix / RTP_PAYLOAD_MAX_SIZE;
        /* The size of the last shard */
        const int last_fu_pack_size = nalu_len_without_prefix % RTP_PAYLOAD_MAX_SIZE ? nalu_len_without_prefix % RTP_PAYLOAD_MAX_SIZE : RTP_PAYLOAD_MAX_SIZE;
        /* fu-A Serial number */
        for (int fu_seq = 0; fu_seq < fu_pack_num; fu_seq++) {
            memset(mRTP_BUFF_SEND, 0, sizeof(rtp_header_t) + sizeof(fu_indicator_t) + sizeof(fu_header_t));
            //
            auto* rtp_hdr = (rtp_header_t *)mRTP_BUFF_SEND;
            auto* fu_ind = (fu_indicator_t *)&mRTP_BUFF_SEND[sizeof(rtp_header_t)];
            auto* fu_hdr = (fu_header_t *)&mRTP_BUFF_SEND[sizeof(rtp_header_t) + sizeof(fu_indicator_t)];
            /*
             * 根据FU-A的类型设置不同的rtp头和rtp荷载头
             */
            if (fu_seq == 0) {  /* 第一个FU-A */
                /*
                 * 1. 设置 rtp 头
                 */
                rtp_hdr->cc = 0;
                rtp_hdr->extension = 0;
                rtp_hdr->padding = 0;
                rtp_hdr->version = 2;
                rtp_hdr->payload = RTP_PAYLOAD_TYPE_H264_H265;
                rtp_hdr->marker = 0;    /* If the packet is the end of a frame, set it to 1, otherwise it is 0. rfc 1889 does not specify the purpose of this bit*/
                rtp_hdr->sequence = htons(++seq_num % UINT16_MAX);
                rtp_hdr->timestamp = htonl(ts_current);
                rtp_hdr->sources = htonl(MY_SSRC_NUM);
                /*
                 * 2. 设置 rtp 荷载头部
                 */
                fu_ind->f = (nalu_buf_without_prefix[0] & 0x80) >> 7;
                fu_ind->nri = (nalu_buf_without_prefix[0] & 0x60) >> 5;
                fu_ind->type = 28;
                //
                fu_hdr->s = 1;
                fu_hdr->e = 0;
                fu_hdr->r = 0;
                fu_hdr->type = nalu_buf_without_prefix[0] & 0x1f;
                /*
                 * 3. 填充nalu内容
                 */
                memcpy(mRTP_BUFF_SEND + 14, nalu_buf_without_prefix + 1, RTP_PAYLOAD_MAX_SIZE - 1);    /* 不拷贝nalu头 */
                /*
                 * 4. 发送打包好的rtp包到客户端
                 */
                const size_t len_sendbuf = 12 + 2 + (RTP_PAYLOAD_MAX_SIZE - 1);  /* rtp头 + nalu头 + nalu内容 */
                forwardRTPPacket(mRTP_BUFF_SEND, len_sendbuf);

            } else if (fu_seq < fu_pack_num - 1) { /* 中间的FU-A */
                /*
                 * 1. 设置 rtp 头
                 */
                rtp_hdr->cc = 0;
                rtp_hdr->extension = 0;
                rtp_hdr->padding = 0;
                rtp_hdr->version = 2;
                rtp_hdr->payload = RTP_PAYLOAD_TYPE_H264_H265;
                rtp_hdr->marker = 0;    /* 该包为一帧的结尾则置为1, 否则为0. rfc 1889 没有规定该位的用途 */
                rtp_hdr->sequence = htons(++seq_num % UINT16_MAX);
                rtp_hdr->timestamp = htonl(ts_current);
                rtp_hdr->sources = htonl(MY_SSRC_NUM);
                /*
                 * 2. 设置 rtp 荷载头部
                 */
                fu_ind->f = (nalu_buf_without_prefix[0] & 0x80) >> 7;
                fu_ind->nri = (nalu_buf_without_prefix[0] & 0x60) >> 5;
                fu_ind->type = 28;
                //
                fu_hdr->s = 0;
                fu_hdr->e = 0;
                fu_hdr->r = 0;
                fu_hdr->type = nalu_buf_without_prefix[0] & 0x1f;
                /*
                 * 3. 填充nalu内容
                 */
                memcpy(mRTP_BUFF_SEND + 14, nalu_buf_without_prefix + RTP_PAYLOAD_MAX_SIZE * fu_seq, RTP_PAYLOAD_MAX_SIZE);    /* 不拷贝nalu头 */
                /*
                 * 4. 发送打包好的rtp包到客户端
                 */
                const size_t len_sendbuf = 12 + 2 + RTP_PAYLOAD_MAX_SIZE;
                forwardRTPPacket(mRTP_BUFF_SEND, len_sendbuf);
            } else { /* 最后一个FU-A */
                /*
                 * 1. 设置 rtp 头
                 */
                rtp_hdr->cc = 0;
                rtp_hdr->extension = 0;
                rtp_hdr->padding = 0;
                rtp_hdr->version = 2;
                rtp_hdr->payload = RTP_PAYLOAD_TYPE_H264_H265;
                rtp_hdr->marker = 1;    /* 该包为一帧的结尾则置为1, 否则为0. rfc 1889 没有规定该位的用途 */
                rtp_hdr->sequence = htons(++seq_num % UINT16_MAX);
                rtp_hdr->timestamp = htonl(ts_current);
                rtp_hdr->sources = htonl(MY_SSRC_NUM);
                /*
                 * 2. 设置 rtp 荷载头部
                 */
                fu_ind->f = (nalu_buf_without_prefix[0] & 0x80) >> 7;
                fu_ind->nri = (nalu_buf_without_prefix[0] & 0x60) >> 5;
                fu_ind->type = 28;
                //
                fu_hdr->s = 0;
                fu_hdr->e = 1;
                fu_hdr->r = 0;
                fu_hdr->type = nalu_buf_without_prefix[0] & 0x1f;
                /*
                 * 3. 填充rtp荷载
                 */
                memcpy(mRTP_BUFF_SEND + 14, nalu_buf_without_prefix + RTP_PAYLOAD_MAX_SIZE * fu_seq, last_fu_pack_size);    /* 不拷贝nalu头 */
                /*
                 * 4. 发送打包好的rtp包到客户端
                 */
                const size_t len_sendbuf = 12 + 2 + last_fu_pack_size;
                forwardRTPPacket(mRTP_BUFF_SEND, len_sendbuf);

            } /* else-if (fu_seq == 0) */
        } /* end of for (fu_seq = 0; fu_seq < fu_pack_num; fu_seq++) */

    } /* end of else-if (nalu_len <= RTP_PAYLOAD_MAX_SIZE) */
    return 0;
}


void RTPEncoder::forwardRTPPacket(uint8_t *rtp_packet, size_t rtp_packet_len) {
    assert(rtp_packet_len!=0);
    assert(rtp_packet_len<=RTP_PACKET_MAX_SIZE);
    //MLOGD<<"forwardRTPPacket of size "<<rtp_packet_len;
    if(mCB!= nullptr){
        mCB({rtp_packet,rtp_packet_len});
    }else{
        MLOGE<<"No RTP Encoder callback set";
    }
}

TestEncodeDecodeRTP::TestEncodeDecodeRTP() {
    decoder=std::make_unique<RTPDecoder>(std::bind(&TestEncodeDecodeRTP::onNALU, this, std::placeholders::_1));
    encoder=std::make_unique<RTPEncoder>(std::bind(&TestEncodeDecodeRTP::onRTP, this, std::placeholders::_1));
}

void TestEncodeDecodeRTP::testEncodeDecodeRTP(const NALU& nalu) {
    if(nalu.getSize()<=6)return;
    encoder->parseNALtoRTP(30,nalu.getData(),nalu.getSize());
    //
    assert(lastNALU!=nullptr);
    assert(lastNALU->getSize()==nalu.getSize());
    const bool contentEquals=memcmp(nalu.getData(),lastNALU->getData(),nalu.getSize())==0;
    assert(contentEquals==true);
    lastNALU.reset();
}

void TestEncodeDecodeRTP::onRTP(const RTPEncoder::RTPPacket &packet) {
    decoder->parseRTPtoNALU(packet.data,packet.data_len);
}

void TestEncodeDecodeRTP::onNALU(const NALU &nalu) {
    // If we feed one NALU we should only get one NALU out
    assert(lastNALU==nullptr);
    lastNALU=std::make_unique<NALU>(nalu);
}
