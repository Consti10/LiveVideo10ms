//
// Created by Constantin on 2/6/2019.
//

#ifndef LIVE_VIDEO_10MS_ANDROID_PARSERTP_H
#define LIVE_VIDEO_10MS_ANDROID_PARSERTP_H

#include <cstdio>
#include "../NALU/NALU.hpp"
#include "IParser.h"

class ParseRTP : public IParser{
public:
    ParseRTP(NALU_DATA_CALLBACK cb);
public:
    void parseData(const uint8_t* data,const int data_length) override;
    void reset() override;
private:
    int nalu_data_length=0;
};
#endif //LIVE_VIDEO_10MS_ANDROID_PARSERTP_H
