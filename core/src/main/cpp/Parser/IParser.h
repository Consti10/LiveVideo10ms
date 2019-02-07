//
// Created by Constantin on 2/6/2019.
//

#ifndef LIVE_VIDEO_10MS_ANDROID_IPARSER_H
#define LIVE_VIDEO_10MS_ANDROID_IPARSER_H


class IParser{
public:
    IParser(NALU_DATA_CALLBACK cb):cb(cb){
    }
    virtual void parseData(const uint8_t* data,const int data_length)=0;
    virtual void reset()=0;
protected:
    uint8_t nalu_data[NALU_MAXLEN];
    const NALU_DATA_CALLBACK cb;
};


#endif //LIVE_VIDEO_10MS_ANDROID_IPARSER_H
