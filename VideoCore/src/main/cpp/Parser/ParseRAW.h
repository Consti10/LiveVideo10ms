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
    void parseData(const uint8_t* data,const size_t data_length);
    // Special parsing method, where AUD determine the end of sliced data packets that cannot decoded individually
    void parseDjiLiveVideoData(const uint8_t* data,const size_t data_length);
    void reset();
private:
    const NALU_DATA_CALLBACK cb;
    NALU::NALU_BUFFER nalu_data;
    //std::vector<uint8_t> nalu_data;
    //std::shared_ptr<NALU::NALU_BUFFER> nalu_data;
    std::mutex mBufferMutex;
    std::vector<NALU::NALU_BUFFER> allocatedBuffers{10};

    size_t nalu_data_position=4;
    int nalu_search_state=0;
    //
    std::array<uint8_t,NALU::NALU_MAXLEN> dji_data_buff;
    std::size_t dji_data_buff_size=0;
    //
    void getAvailableBuffer();
};

#endif //LIVE_VIDEO_10MS_ANDROID_PARSERAW_H
