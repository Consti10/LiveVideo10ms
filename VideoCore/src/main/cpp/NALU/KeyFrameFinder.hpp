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
    std::unique_ptr<NALU> SPS;
    std::unique_ptr<NALU> PPS;
    // VPS are only used in H265
    std::unique_ptr<NALU> VPS;
public:
    void saveIfKeyFrame(const NALU &nalu){
        if(nalu.getSize()<=0)return;
        if(nalu.isSPS()){
            SPS=std::make_unique<NALU>(nalu);
            //MLOGD<<"SPS found";
        }else if(nalu.isPPS()){
            PPS=std::make_unique<NALU>(nalu);
            //MLOGD<<"PPS found";
        }else if(nalu.IS_H265_PACKET && nalu.isVPS()){
            VPS=std::make_unique<NALU>(nalu);
            //MLOGD<<"VPS found";
        }
    }
    // H264 needs sps and pps
    // H265 needs sps,pps and vps
    bool allKeyFramesAvailable(const bool IS_H265=false){
        if(IS_H265){
            return SPS != nullptr && PPS != nullptr && VPS!=nullptr;
        }
        return SPS != nullptr && PPS != nullptr;
    }
    //SPS
    const NALU& getCSD0()const{
        return *SPS;
    }
    //PPS
    const NALU& getCSD1()const{
        return *PPS;
    }
    static void appendNaluData(std::vector<uint8_t>& buff,const NALU& nalu){
        buff.insert(buff.begin(),nalu.getData(),nalu.getData()+nalu.getSize());
    }
    void reset(){
        SPS=nullptr;
        PPS=nullptr;
        VPS=nullptr;
    }
public:
    void writeAndroidPerformanceParams(AMediaFormat* format){
        static const auto PARAMETER_KEY_LOW_LATENCY="low-latency";
        AMediaFormat_setInt32(format,PARAMETER_KEY_LOW_LATENCY,1);
        // Lower values mean higher priority
        // Works on pixel 3 (look at output format description)
        static const auto AMEDIAFORMAT_KEY_PRIORITY="priority";
        AMediaFormat_setInt32(format,AMEDIAFORMAT_KEY_PRIORITY,0);
    }
    void h264_configureAMediaFormat(AMediaFormat* format){
        const auto sps=getCSD0();
        const auto pps=getCSD1();
        const auto videoWH= sps.getVideoWidthHeightSPS();
        AMediaFormat_setInt32(format,AMEDIAFORMAT_KEY_WIDTH,videoWH[0]);
        AMediaFormat_setInt32(format,AMEDIAFORMAT_KEY_HEIGHT,videoWH[1]);
        AMediaFormat_setBuffer(format,"csd-0",sps.getData(),(size_t)sps.getSize());
        AMediaFormat_setBuffer(format,"csd-1",pps.getData(),(size_t)pps.getSize());
        MLOGD<<"Video WH:"<<videoWH[0]<<" H:"<<videoWH[1];
        writeAndroidPerformanceParams(format);
        //AMediaFormat_setInt32(format,AMEDIAFORMAT_KEY_BIT_RATE,5*1024*1024);
        //AMediaFormat_setInt32(decoder.format,AMEDIAFORMAT_KEY_FRAME_RATE,60);
        //AVCProfileBaseline==1
        //AMediaFormat_setInt32(decoder.format,AMEDIAFORMAT_KEY_PROFILE,1);
        //AMediaFormat_setInt32(decoder.format,AMEDIAFORMAT_KEY_PRIORITY,0);
    }
    void h265_configureAMediaFormat(AMediaFormat* format){
        std::vector<uint8_t> buff={};
        buff.reserve(SPS->getSize()+PPS->getSize()+VPS->getSize());
        appendNaluData(buff,*VPS);
        appendNaluData(buff,*SPS);
        appendNaluData(buff,*PPS);
        const auto videoWH= getCSD0().getVideoWidthHeightSPS();
        AMediaFormat_setInt32(format,AMEDIAFORMAT_KEY_WIDTH,videoWH[0]);
        AMediaFormat_setInt32(format,AMEDIAFORMAT_KEY_HEIGHT,videoWH[1]);
        AMediaFormat_setBuffer(format,"csd-0",buff.data(),buff.size());
        MLOGD<<"Video WH:"<<videoWH[0]<<" H:"<<videoWH[1];
        writeAndroidPerformanceParams(format);
    }
};

#endif //LIVEVIDEO10MS_KEYFRAMEFINDER_HPP
