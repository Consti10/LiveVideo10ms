//
// Created by geier on 07/02/2020.
//

#ifndef LIVEVIDEO10MS_KEYFRAMEFINDER_HPP
#define LIVEVIDEO10MS_KEYFRAMEFINDER_HPP

#include "NALU.hpp"
#include <vector>
#include <AndroidLogger.hpp>
#include <media/NdkMediaFormat.h>
#include <memory>

// Takes a continuous stream of NALUs and save SPS / PPS data
// For later use
class KeyFrameFinder{
private:
    //NALU* CSD0=nullptr;
    //NALU* CSD1=nullptr;
    std::unique_ptr<NALU> CSD0;
    std::unique_ptr<NALU> CSD1;
public:
    void saveIfKeyFrame(const NALU &nalu){
        if(nalu.getSize()<=0)return;
        if(nalu.isSPS()){
            CSD0=std::make_unique<NALU>(nalu);
            //MLOGD<<"SPS found";
        }else if(nalu.isPPS()){
            CSD1=std::make_unique<NALU>(nalu);
            //MLOGD<<"PPS found";
        }
    }
    bool allKeyFramesAvailable(){
        return CSD0!=nullptr && CSD1!=nullptr;
    }
    //SPS
    const NALU& getCSD0()const{
        return *CSD0;
    }
    //PPS
    const NALU& getCSD1()const{
        return *CSD1;
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
        CSD0=nullptr;
        CSD1=nullptr;
    }
};

#endif //LIVEVIDEO10MS_KEYFRAMEFINDER_HPP
