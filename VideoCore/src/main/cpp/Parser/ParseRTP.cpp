//
// Created by Constantin on 2/6/2019.
//

#include "ParseRTP.h"
#include <android/log.h>

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
    uint32_t sources[1];      // contributing sources
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

constexpr auto TAG="ParseRTP";
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)



ParseRTP::ParseRTP(NALU_DATA_CALLBACK cb):cb(cb){
}

void ParseRTP::reset(){
    nalu_data_length=0;
}

void ParseRTP::parseData(const uint8_t* rtp_data,const size_t data_len){
    //12 rtp header bytes and 1 nalu_header_t type byte
        if(data_len<=13){
            LOGD("Not enough rtp data");
            return;
        }
        nalu_header_t *nalu_header;
        fu_header_t   *fu_header;
        uint8_t h264_nal_header;

        nalu_header = (nalu_header_t *)&rtp_data[12];
        if (nalu_header->type == 28) { /* FU-A */
            fu_header = (fu_header_t*)&rtp_data[13];
            if (fu_header->e == 1) {
                /* end of fu-a */
                memcpy(&nalu_data[nalu_data_length],&rtp_data[14],(size_t)data_len-14);
                nalu_data_length+=data_len-14;
                if(cb!= nullptr){
                    NALU nalu(nalu_data,nalu_data_length);
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
                h264_nal_header = (uint8_t)(fu_header->type & 0x1f)
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
            /* full nalu */
            nalu_data[0]=0;
            nalu_data[1]=0;
            nalu_data[2]=0;
            nalu_data[3]=1;
            nalu_data_length=4;
            h264_nal_header = (uint8_t )(nalu_header->type & 0x1f)
                              | (nalu_header->nri << 5)
                              | (nalu_header->f << 7);
            nalu_data[4]=h264_nal_header;
            nalu_data_length++;
            memcpy(&nalu_data[nalu_data_length],&rtp_data[13],(size_t)data_len-13);
            nalu_data_length+=data_len-13;

            if(cb!= nullptr){
                NALU nalu(nalu_data,nalu_data_length);
                cb(nalu);
            }

            nalu_data_length=0;
            //LOGV("full nalu");
        }else{
            LOGD("header:%d",(int)nalu_header->type);
        }
}

