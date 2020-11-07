//
// Created by consti10 on 07.11.20.
//

#ifndef LIVEVIDEO10MS_RTP_HPP
#define LIVEVIDEO10MS_RTP_HPP

#include <arpa/inet.h>
#include <sstream>
// This code is written for little endian (aka ARM) byte order
static_assert(__BYTE_ORDER__==__LITTLE_ENDIAN);
// Same for both h264 and h265
// Defined in https://tools.ietf.org/html/rfc3550
//0                   1                   2                   3
//0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//|V=2|P|X|  CC   |M|     PT      |       sequence number         |
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//|                           timestamp                           |
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//|           synchronization source (SSRC) identifier            |
//+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//|            contributing source (CSRC) identifiers             |
//|                             ....                              |
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
typedef struct rtp_header {
    //For little endian
    uint8_t cc:4;            // CSRC count
    uint8_t extension:1;     // Extension bit
    uint8_t padding:1;       // Padding bit
    uint8_t version:2;       // Version, currently 2
    uint8_t payload:7;       // Payload type
    uint8_t marker:1;        // Marker bit
    //
    uint16_t sequence;        // sequence number
    uint32_t timestamp;       //  timestamp
    uint32_t sources;      // contributing sources
//NOTE: sequence,timestamp and sources has to be converted to the right endianness using htonl/htons
//For all the other members, I reversed the order such that it matches the byte order of the architecture
    uint16_t getSequence()const{
        return htons(sequence);
    }
    uint32_t getTimestamp()const{
        return htonl(timestamp);
    }
    uint32_t getSources()const{
        return htonl(sources);
    }
    std::string asString()const{
        std::stringstream ss;
        ss<<"cc"<<(int)cc<<"\n";
        ss<<"extension"<<(int)extension<<"\n";
        ss<<"padding"<<(int)padding<<"\n";
        ss<<"version"<<(int)version<<"\n";
        ss<<"payload"<<(int)payload<<"\n";
        ss<<"marker"<<(int)marker<<"\n";
        ss<<"sequence"<<(int)getSequence()<<"\n";
        ss<<"timestamp"<<(int)getTimestamp()<<"\n";
        ss<<"sources"<<(int)getSources()<<"\n";
        return ss.str();
    }
} __attribute__ ((packed)) rtp_header_t; /* 12 bytes */
static_assert(sizeof(rtp_header_t)==12);

//******************************************************** H264 ********************************************************
// https://tools.ietf.org/html/rfc6184
//+---------------+
//|0|1|2|3|4|5|6|7|
//+-+-+-+-+-+-+-+-+
//|F|NRI|  Type   |
//+---------------+
typedef struct nalu_header {
    uint8_t type:   5;
    uint8_t nri:    2;
    uint8_t f:      1;
} __attribute__ ((packed)) nalu_header_t;
static_assert(sizeof(nalu_header_t)==1);
//+---------------+
//|0|1|2|3|4|5|6|7|
//+-+-+-+-+-+-+-+-+
//|F|NRI|  Type   |
//+---------------+
typedef struct fu_indicator {
    uint8_t type:   5;
    uint8_t nri:    2;
    uint8_t f:      1;
} __attribute__ ((packed)) fu_indicator_t; /* 1 bytes */
static_assert(sizeof(fu_indicator_t)==1);
//+---------------+
//|0|1|2|3|4|5|6|7|
//+-+-+-+-+-+-+-+-+
//|S|E|R|  Type   |
//+---------------+
typedef struct fu_header {
    uint8_t type:   5;
    uint8_t r:      1;
    uint8_t e:      1;
    uint8_t s:      1;
} __attribute__ ((packed)) fu_header_t;
static_assert(sizeof(fu_header_t)==1);

//******************************************************** H265 ********************************************************
// defined in 1.1.4.  NAL Unit Header https://tools.ietf.org/html/rfc7798
//  +---------------+---------------+
//  |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |F|   Type    |  LayerId  | TID |
//  +-------------+-----------------+
typedef struct nal_unit_header_h265{
    uint8_t f:      1;
    uint8_t type:   6;
    uint8_t layerId:6;
    uint8_t tid:    3;
}__attribute__ ((packed)) nal_unit_header_h265_t;
static_assert(sizeof(nal_unit_header_h265_t)==2);
// defined in 4.4.3 FU Header
//+---------------+
//|0|1|2|3|4|5|6|7|
//+-+-+-+-+-+-+-+-+
//|S|E|  FuType   |
//+---------------+
typedef struct fu_header_h265{
    uint8_t fuType:6;
    uint8_t e:1;
    uint8_t s:1;
}__attribute__ ((packed)) fu_header_h265_t;
static_assert(sizeof(fu_header_h265_t)==1);

// Unfortunately the payload header is the same for h264 and h265
static constexpr auto RTP_PAYLOAD_TYPE_H264_H265=96;
static constexpr auto MY_SSRC_NUM=10;

#endif //LIVEVIDEO10MS_RTP_HPP
