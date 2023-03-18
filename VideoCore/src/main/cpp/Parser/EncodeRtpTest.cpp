//
// Created by consti10 on 18.03.23.
//

#include "EncodeRtpTest.h"

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

/*TestEncodeDecodeRTP::TestEncodeDecodeRTP() {
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
    decoder->parseRTPH264toNALU(packet.data, packet.data_len);
}

void TestEncodeDecodeRTP::onNALU(const NALU &nalu) {
    // If we feed one NALU we should only get one NALU out
    assert(lastNALU==nullptr);
    lastNALU=std::make_unique<NALU>(nalu);
}*/
