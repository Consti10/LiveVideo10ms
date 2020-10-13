//
// Created by Constantin on 2/6/2019.
//

#include "ParseRTP.h"
#include <AndroidLogger.hpp>

//changed "unsigned char" to uint8_t
typedef struct rtp_header {
#if __BYTE_ORDER == __BIG_ENDIAN
    //For big endian
    uint8_t version:2;       // Version, currently 2
    uint8_t padding:1;       // Padding bit
    uint8_t extension:1;     // Extension bit
    uint8_t cc:4;            // CSRC count
    uint8_t marker:1;        // Marker bit
    uint8_t payload:7;       // Payload type
#else
    //For little endian
    uint8_t cc:4;            // CSRC count
    uint8_t extension:1;     // Extension bit
    uint8_t padding:1;       // Padding bit
    uint8_t version:2;       // Version, currently 2
    uint8_t payload:7;       // Payload type
    uint8_t marker:1;        // Marker bit
#endif
    uint16_t sequence;        // sequence number
    uint32_t timestamp;       //  timestamp
    uint32_t sources;      // contributing sources
} __attribute__ ((packed)) rtp_header_t; /* 12 bytes */
//NOTE: sequence,timestamp and sources has to be converted to the right endian using htonl/htons

//Taken from https://github.com/hmgle/h264_to_rtp/blob/6cf8f07a8243413d7bbbdf5fa5db5fd1ca48beca/h264tortp.h
typedef struct nalu_header {
    uint8_t type:   5;  /* bit: 0~4 */
    uint8_t nri:    2;  /* bit: 5~6 */
    uint8_t f:      1;  /* bit: 7 */
} __attribute__ ((packed)) nalu_header_t; /* 1 bytes */
typedef struct fu_header {
    uint8_t type:   5;
    uint8_t r:      1;
    uint8_t e:      1;
    uint8_t s:      1;
} __attribute__ ((packed)) fu_header_t; /* 1 bytes */

//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
typedef struct fu_indicator {
    /* byte 0 */
    uint8_t type:   5;
    uint8_t nri:    2;
    uint8_t f:      1;
} __attribute__ ((packed)) fu_indicator_t; /* 1 bytes */
static constexpr auto H264=96;
static constexpr auto SSRC_NUM=10;
#include <arpa/inet.h>


ParseRTP::ParseRTP(NALU_DATA_CALLBACK cb):cb(std::move(cb)){
}

void ParseRTP::reset(){
    nalu_data_length=0;
    //nalu_data.reserve(NALU::NALU_MAXLEN);
}

void ParseRTP::parseData(const uint8_t* rtp_data,const size_t data_len){
    //12 rtp header bytes and 1 nalu_header_t type byte
        if(data_len<=13){
            MLOGD<<"Not enough rtp data";
            return;
        }
        //
        //const rtp_header_t* rtp_header=(rtp_header_t*)rtp_data;
        //LOG::D("Sequence number %d",(int)rtp_header->sequence);

        const nalu_header_t *nalu_header=(nalu_header_t *)&rtp_data[12];

        MLOGD<<"Parsing RTP data"<<nalu_header->type;

        if (nalu_header->type == 28) { /* FU-A */
            const fu_header_t* fu_header = (fu_header_t*)&rtp_data[13];
            if (fu_header->e == 1) {
                /* end of fu-a */
                memcpy(&nalu_data[nalu_data_length],&rtp_data[14],(size_t)data_len-14);
                nalu_data_length+=data_len-14;
                if(cb!= nullptr){
                    NALU nalu(nalu_data,nalu_data_length);
                    //nalu_data.resize(nalu_data_length);
                    //NALU nalu(nalu_data);
                    cb(nalu);
                }
                nalu_data_length=0;
            } else if (fu_header->s == 1) {
                /* start of fu-a */
                nalu_data[0]=0;
                nalu_data[1]=0;
                nalu_data[2]=0;
                nalu_data[3]=1;
                nalu_data_length=4;
                const uint8_t h264_nal_header = (uint8_t)(fu_header->type & 0x1f)
                                  | (nalu_header->nri << 5)
                                  | (nalu_header->f << 7);
                nalu_data[4]=h264_nal_header;
                nalu_data_length++;
                memcpy(&nalu_data[nalu_data_length],&rtp_data[14],(size_t)data_len-14);
                nalu_data_length+=data_len-14;
            } else {
                /* middle of fu-a */
                memcpy(&nalu_data[nalu_data_length],&rtp_data[14],(size_t)data_len-14);
                nalu_data_length+=data_len-14;
            }
            //LOGV("partially nalu");
        } else if(nalu_header->type>0 && nalu_header->type<24){
            MLOGD<<"Got full NALU";
            /* full nalu */
            nalu_data[0]=0;
            nalu_data[1]=0;
            nalu_data[2]=0;
            nalu_data[3]=1;
            nalu_data_length=4;
            const uint8_t h264_nal_header = (uint8_t )(nalu_header->type & 0x1f)
                              | (nalu_header->nri << 5)
                              | (nalu_header->f << 7);
            nalu_data[4]=h264_nal_header;
            nalu_data_length++;
            memcpy(&nalu_data[nalu_data_length],&rtp_data[13],(size_t)data_len-13);
            nalu_data_length+=data_len-13;

            if(cb!= nullptr){
                NALU nalu(nalu_data,nalu_data_length);
                //nalu_data.resize(nalu_data_length);
                //NALU nalu(nalu_data);
                cb(nalu);
            }
            nalu_data_length=0;
            //LOGV("full nalu");
        }else{
            //MLOGD<<"header:"<<nalu_header->type;
        }
}

int ParseRTP::h264nal2rtp_send(int framerate, uint8_t *pstStream, int nalu_len) {
    uint8_t *nalu_buf;
    nalu_buf = &pstStream[4];
    nalu_len-=4;
    // int nalu_len;   /* Does not include the 0x00000001 start code, but includes the length of the nalu header */
    static uint32_t ts_current = 0;
    static uint16_t seq_num = 0;
    rtp_header_t *rtp_hdr;
    nalu_header_t *nalu_hdr;
    fu_indicator_t *fu_ind;
    fu_header_t *fu_hdr;
    size_t len_sendbuf;

    int fu_pack_num;        /* The number of splits when nalu needs to be split to send */
    int last_fu_pack_size;  /* The size of the last shard */
    int fu_seq;             /* fu-A Serial number */

    //debug_print();
    ts_current += (90000 / framerate);  /* 90000 / 25 = 3600 */

    /*
     * Add length judgment
     * 当 nalu_len == 0 时， Must skip to the next cycle
     * nalu_len == 0 时，If you don't jump out, a segfault will occur!
     * fix by hmg
     */
    if (nalu_len < 1) {
        return -1;
    }

    if (nalu_len <= RTP_PAYLOAD_MAX_SIZE) {
        /*
         * single nal unit
         */

        memset(SENDBUFFER, 0, SEND_BUF_SIZE);

        /*
         * 1. 设置 rtp 头
         */
        rtp_hdr = (rtp_header_t *)SENDBUFFER;
        rtp_hdr->cc = 0;
        rtp_hdr->extension = 0;
        rtp_hdr->padding = 0;
        rtp_hdr->version = 2;
        rtp_hdr->payload = H264;
        // rtp_hdr->marker = (pstStream->u32PackCount - 1 == i) ? 1 : 0;   /* 该包为一帧的结尾则置为1, 否则为0. rfc 1889 没有规定该位的用途 */
        rtp_hdr->sequence = htons(++seq_num % UINT16_MAX);
        rtp_hdr->timestamp = htonl(ts_current);
        rtp_hdr->sources = htonl(SSRC_NUM);

        //debug_print();
        /*
         * 2. Set rtp load single nal unit header
         */
        nalu_hdr = (nalu_header_t *)&SENDBUFFER[12];
        nalu_hdr->f = (nalu_buf[0] & 0x80) >> 7;        /* bit0 */
        nalu_hdr->nri = (nalu_buf[0] & 0x60) >> 5;      /* bit1~2 */
        nalu_hdr->type = (nalu_buf[0] & 0x1f);
        //debug_print();

        /*
         * 3.Fill nal content
         */
        //debug_print();
        memcpy(SENDBUFFER + 13, nalu_buf + 1, nalu_len - 1);    /* 不拷贝nalu头 */

        /*
         * 4. Send the packaged rtp to the client
         */
        len_sendbuf = 12 + nalu_len;
        send_data_to_client_list(SENDBUFFER->data(), len_sendbuf);
        //debug_print();

        MLOGD<<"NALU <RTP_PAYLOAD_MAX_SIZE";

    } else {    /* nalu_len > RTP_PAYLOAD_MAX_SIZE */
        MLOGD<<"NALU >RTP_PAYLOAD_MAX_SIZE";
        /*
         * FU-A segmentation
         */
        //debug_print();

        /*
         * 1. Count the number of divisions
         *
         * Except for the last shard，
         * Consumption per shard RTP_PAYLOAD_MAX_SIZE BYLE
         */
        fu_pack_num = nalu_len % RTP_PAYLOAD_MAX_SIZE ? (nalu_len / RTP_PAYLOAD_MAX_SIZE + 1) : nalu_len / RTP_PAYLOAD_MAX_SIZE;
        last_fu_pack_size = nalu_len % RTP_PAYLOAD_MAX_SIZE ? nalu_len % RTP_PAYLOAD_MAX_SIZE : RTP_PAYLOAD_MAX_SIZE;
        fu_seq = 0;

        for (fu_seq = 0; fu_seq < fu_pack_num; fu_seq++) {
            memset(SENDBUFFER, 0, SEND_BUF_SIZE);

            /*
             * 根据FU-A的类型设置不同的rtp头和rtp荷载头
             */
            if (fu_seq == 0) {  /* 第一个FU-A */
                /*
                 * 1. 设置 rtp 头
                 */
                rtp_hdr = (rtp_header_t *)SENDBUFFER;
                rtp_hdr->cc = 0;
                rtp_hdr->extension = 0;
                rtp_hdr->padding = 0;
                rtp_hdr->version = 2;
                rtp_hdr->payload = H264;
                rtp_hdr->marker = 0;    /* 该包为一帧的结尾则置为1, 否则为0. rfc 1889 没有规定该位的用途 */
                rtp_hdr->sequence = htons(++seq_num % UINT16_MAX);
                rtp_hdr->timestamp = htonl(ts_current);
                rtp_hdr->sources = htonl(SSRC_NUM);

                /*
                 * 2. 设置 rtp 荷载头部
                 */
#if 1
                fu_ind = (fu_indicator_t *)&SENDBUFFER[12];
                fu_ind->f = (nalu_buf[0] & 0x80) >> 7;
                fu_ind->nri = (nalu_buf[0] & 0x60) >> 5;
                fu_ind->type = 28;
#else   /* 下面的错误以后再找 */
                SENDBUFFER[12] = (nalu_buf[0] & 0x80) >> 7  /* bit0: f */
                        | (nalu_buf[0] & 0x60) >> 4             /* bit1~2: nri */
                        | 28 << 3;                              /* bit3~7: type */
#endif

#if 1
                fu_hdr = (fu_header_t *)&SENDBUFFER[13];
                fu_hdr->s = 1;
                fu_hdr->e = 0;
                fu_hdr->r = 0;
                fu_hdr->type = nalu_buf[0] & 0x1f;
#else
                SENDBUFFER[13] = 1 | (nalu_buf[0] & 0x1f) << 3;
#endif

                /*
                 * 3. 填充nalu内容
                 */
                memcpy(SENDBUFFER + 14, nalu_buf + 1, RTP_PAYLOAD_MAX_SIZE - 1);    /* 不拷贝nalu头 */

                /*
                 * 4. 发送打包好的rtp包到客户端
                 */
                len_sendbuf = 12 + 2 + (RTP_PAYLOAD_MAX_SIZE - 1);  /* rtp头 + nalu头 + nalu内容 */
                send_data_to_client_list(SENDBUFFER->data(), len_sendbuf);

            } else if (fu_seq < fu_pack_num - 1) { /* 中间的FU-A */
                /*
                 * 1. 设置 rtp 头
                 */
                rtp_hdr = (rtp_header_t *)SENDBUFFER;
                rtp_hdr->cc = 0;
                rtp_hdr->extension = 0;
                rtp_hdr->padding = 0;
                rtp_hdr->version = 2;
                rtp_hdr->payload = H264;
                rtp_hdr->marker = 0;    /* 该包为一帧的结尾则置为1, 否则为0. rfc 1889 没有规定该位的用途 */
                rtp_hdr->sequence = htons(++seq_num % UINT16_MAX);
                rtp_hdr->timestamp = htonl(ts_current);
                rtp_hdr->sources = htonl(SSRC_NUM);

                /*
                 * 2. 设置 rtp 荷载头部
                 */
#if 1
                fu_ind = (fu_indicator_t *)&SENDBUFFER[12];
                fu_ind->f = (nalu_buf[0] & 0x80) >> 7;
                fu_ind->nri = (nalu_buf[0] & 0x60) >> 5;
                fu_ind->type = 28;

                fu_hdr = (fu_header_t *)&SENDBUFFER[13];
                fu_hdr->s = 0;
                fu_hdr->e = 0;
                fu_hdr->r = 0;
                fu_hdr->type = nalu_buf[0] & 0x1f;
#else   /* 下面的错误以后要找 */
                SENDBUFFER[12] = (nalu_buf[0] & 0x80) >> 7  /* bit0: f */
                        | (nalu_buf[0] & 0x60) >> 4             /* bit1~2: nri */
                        | 28 << 3;                              /* bit3~7: type */
                    SENDBUFFER[13] = 0 | (nalu_buf[0] & 0x1f) << 3;
#endif

                /*
                 * 3. 填充nalu内容
                 */
                memcpy(SENDBUFFER + 14, nalu_buf + RTP_PAYLOAD_MAX_SIZE * fu_seq, RTP_PAYLOAD_MAX_SIZE);    /* 不拷贝nalu头 */

                /*
                 * 4. 发送打包好的rtp包到客户端
                 */
                len_sendbuf = 12 + 2 + RTP_PAYLOAD_MAX_SIZE;
                send_data_to_client_list(SENDBUFFER->data(), len_sendbuf);

            } else { /* 最后一个FU-A */
                /*
                 * 1. 设置 rtp 头
                 */
                rtp_hdr = (rtp_header_t *)SENDBUFFER;
                rtp_hdr->cc = 0;
                rtp_hdr->extension = 0;
                rtp_hdr->padding = 0;
                rtp_hdr->version = 2;
                rtp_hdr->payload = H264;
                rtp_hdr->marker = 1;    /* 该包为一帧的结尾则置为1, 否则为0. rfc 1889 没有规定该位的用途 */
                rtp_hdr->sequence = htons(++seq_num % UINT16_MAX);
                rtp_hdr->timestamp = htonl(ts_current);
                rtp_hdr->sources = htonl(SSRC_NUM);

                /*
                 * 2. 设置 rtp 荷载头部
                 */
#if 1
                fu_ind = (fu_indicator_t *)&SENDBUFFER[12];
                fu_ind->f = (nalu_buf[0] & 0x80) >> 7;
                fu_ind->nri = (nalu_buf[0] & 0x60) >> 5;
                fu_ind->type = 28;

                fu_hdr = (fu_header_t *)&SENDBUFFER[13];
                fu_hdr->s = 0;
                fu_hdr->e = 1;
                fu_hdr->r = 0;
                fu_hdr->type = nalu_buf[0] & 0x1f;
#else   /* 下面的错误以后找 */
                SENDBUFFER[12] = (nalu_buf[0] & 0x80) >> 7  /* bit0: f */
                        | (nalu_buf[0] & 0x60) >> 4             /* bit1~2: nri */
                        | 28 << 3;                              /* bit3~7: type */
                    SENDBUFFER[13] = 1 << 1 | (nalu_buf[0] & 0x1f) << 3;
#endif

                /*
                 * 3. 填充rtp荷载
                 */
                memcpy(SENDBUFFER + 14, nalu_buf + RTP_PAYLOAD_MAX_SIZE * fu_seq, last_fu_pack_size);    /* 不拷贝nalu头 */

                /*
                 * 4. 发送打包好的rtp包到客户端
                 */
                len_sendbuf = 12 + 2 + last_fu_pack_size;
                send_data_to_client_list(SENDBUFFER->data(), len_sendbuf);

            } /* else-if (fu_seq == 0) */
        } /* end of for (fu_seq = 0; fu_seq < fu_pack_num; fu_seq++) */

    } /* end of else-if (nalu_len <= RTP_PAYLOAD_MAX_SIZE) */

    //debug_print();
    return 0;
}/* void *h264tortp_send(VENC_STREAM_S *pstream, char *rec_ipaddr) */

void ParseRTP::send_data_to_client_list(uint8_t *send_buf, size_t len_sendbuf) {
    MLOGD<<"Got RTP packet "<<len_sendbuf;
    parseData(send_buf,len_sendbuf);
}

