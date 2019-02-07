//
// Created by Constantin on 2/6/2019.
//

#ifndef LIVE_VIDEO_10MS_ANDROID_PARSERAW_H
#define LIVE_VIDEO_10MS_ANDROID_PARSERAW_H

#include <cstdio>
#include "../NALU/NALU.hpp"
#include "IParser.h"

class ParseRAW : public IParser{
public:
    ParseRAW(NALU_DATA_CALLBACK cb);
    void parseData(const uint8_t* data,const int data_length)override;
    void reset()override;
private:
//needed for raw
    int nalu_data_position=4;
    int nalu_search_state=0;
};

#endif //LIVE_VIDEO_10MS_ANDROID_PARSERAW_H
