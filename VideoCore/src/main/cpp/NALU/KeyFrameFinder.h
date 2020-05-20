//
// Created by geier on 07/02/2020.
//

#ifndef LIVEVIDEO10MS_KEYFRAMEFINDER_H
#define LIVEVIDEO10MS_KEYFRAMEFINDER_H

#include "NALU.hpp"
#include <vector>
#include <AndroidLogger.hpp>

//Takes a continuous stream of NALUs and save SPS / PPS data
//For later use
#include <media/NdkMediaFormat.h>

class KeyFrameFinder{
private:
    std::vector<uint8_t> CSD0;
    std::vector<uint8_t> CSD1;
public:
    void saveIfKeyFrame(const NALU &nalu){
        if(nalu.getSize()<=0)return;
        if(nalu.isSPS()){
            CSD0.resize(nalu.getSize());
            memcpy(CSD0.data(),nalu.getData(),(size_t )nalu.getSize());
            MLOGD<<"SPS found";
        }else if(nalu.isPPS()){
            CSD1.resize(nalu.getSize());
            memcpy(CSD1.data(),nalu.getData(),(size_t )nalu.getSize());
            MLOGD<<"PPS found";
        }
    }
    bool allKeyFramesAvailable(){
        return CSD0.size()>0 && CSD1.size()>0;
    }
    //SPS
    NALU getCSD0(){
        return NALU(CSD0);
    }
    //PPS
    NALU getCSD1(){
        return NALU(CSD1);
    }
    void setSPS_PPS_WIDTH_HEIGHT(AMediaFormat* format){
        const auto sps=getCSD0();
        const auto pps=getCSD1();
        const auto videoWH= sps.getVideoWidthHeightSPS();
        AMediaFormat_setInt32(format,AMEDIAFORMAT_KEY_WIDTH,videoWH[0]);
        AMediaFormat_setInt32(format,AMEDIAFORMAT_KEY_HEIGHT,videoWH[1]);
        AMediaFormat_setBuffer(format,"csd-0",sps.getData(),(size_t)sps.getSize());
        AMediaFormat_setBuffer(format,"csd-1",pps.getData(),(size_t)pps.getSize());
    }
    void reset(){
        CSD0.resize(0);
        CSD1.resize(0);
    }
};

#endif //LIVEVIDEO10MS_KEYFRAMEFINDER_H
