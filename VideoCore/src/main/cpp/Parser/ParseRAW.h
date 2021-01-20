//
// Created by Constantin on 2/6/2019.
//

#ifndef LIVE_VIDEO_10MS_ANDROID_PARSERAW_H
#define LIVE_VIDEO_10MS_ANDROID_PARSERAW_H

#include "../NALU/NALU.hpp"
#include <cstdio>
#include <memory>

/*********************************************
 ** Parses a stream of raw h264 NALUs
**********************************************/

class ParseRAW {
public:
    ParseRAW(NALU_DATA_CALLBACK cb);
    // normally H264, otherwise H265 - both protocols use the [0,0,0,1] pattern as prefix
    void parseData(const uint8_t* data,const size_t data_length,const bool isH265=false);
    // Special parsing method, where AUD determine the end of sliced data packets that cannot be decoded individually
    void parseDjiLiveVideoDataH264(const uint8_t* data,const size_t data_length);
    // similar to above but for jetson (testing)
    void parseJetsonRawSlicedH264(const uint8_t* data, const size_t data_length);
    void accumulateSlicedNALUsByAUD(const NALU& nalu);
    void accumulateSlicedNALUsByOther(const NALU& nalu);
    void reset();
private:
    const NALU_DATA_CALLBACK cb;
    NALU::NALU_BUFFER nalu_data;
    //std::vector<uint8_t> nalu_data;
    //std::shared_ptr<NALU::NALU_BUFFER> nalu_data;

    size_t nalu_data_position=4;
    int nalu_search_state=0;
    //
    std::array<uint8_t,NALU::NALU_MAXLEN> dji_data_buff;
    std::size_t dji_data_buff_size=0;
    int nMergedNALUs=0;
    std::chrono::steady_clock::time_point timePointFirstNALUToMerge;
    //
    // This time point is as 'early as possible' to debug the parsing time as accurately as possible.
    // E.g not the time when the 'ending' sequence was detected, but the first byte of this nalu was received / parsed
    std::chrono::steady_clock::time_point timePointStartOfReceivingNALU;
};

#endif //LIVE_VIDEO_10MS_ANDROID_PARSERAW_H
