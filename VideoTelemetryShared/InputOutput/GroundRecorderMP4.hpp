//
// Created by geier on 14/02/2020.
//

#ifndef LIVEVIDEO10MS_GROUNDRECORDERMP4_HPP
#define LIVEVIDEO10MS_GROUNDRECORDERMP4_HPP

#include <string>
#include <media/NdkMediaMuxer.h>
#include <unistd.h>
#include <fcntl.h>
#include "../NALU/NALU.hpp"
#include "../NALU/KeyFrameFinder.h"

//TODO work in progress

class GroundRecorderMP4 {
public:
    GroundRecorderMP4(const std::string GROUND_RECORDING_DIRECTORY):GROUND_RECORDING_DIRECTORY(GROUND_RECORDING_DIRECTORY){}
    ~GroundRecorderMP4(){
        if(mMuxer!= nullptr){
            AMediaMuxer_stop(mMuxer);
            AMediaMuxer_delete(mMuxer);
            close(mFD);
        }
    }
    //
    void writeData(const NALU& nalu) {
        if(mMuxer== nullptr){
            mKeyFrameFInder.saveIfKeyFrame(nalu);
            if(mKeyFrameFInder.allKeyFramesAvailable()){
                const std::string fn=GroundRecorderRAW::findUnusedFilename(GROUND_RECORDING_DIRECTORY,"mp4");
                mFD = open(fn.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
                mMuxer=AMediaMuxer_new(mFD,AMEDIAMUXER_OUTPUT_FORMAT_MPEG_4);
                AMediaFormat*format=AMediaFormat_new();
                const auto SPS=mKeyFrameFInder.getCSD0();
                const auto PPS=mKeyFrameFInder.getCSD1();
                const auto videoWH= SPS.getVideoWidthHeightSPS();
                AMediaFormat_setString(format,AMEDIAFORMAT_KEY_MIME,"video/avc");
                AMediaFormat_setInt32(format,AMEDIAFORMAT_KEY_WIDTH,videoWH[0]);
                AMediaFormat_setInt32(format,AMEDIAFORMAT_KEY_HEIGHT,videoWH[1]);
                AMediaFormat_setBuffer(format,"csd-0",SPS.data,(size_t)SPS.data_length);
                AMediaFormat_setBuffer(format,"csd-1",PPS.data,(size_t)PPS.data_length);
                AMediaFormat_setInt32(format,AMEDIAFORMAT_KEY_MAX_INPUT_SIZE,NALU::NALU_MAXLEN);
                AMediaFormat_setInt32(format,AMEDIAFORMAT_KEY_FRAME_RATE,30);
                mTrackIndex=AMediaMuxer_addTrack(mMuxer,format);
                //LOGD("Media Muxer track index %d",(int)mTrackIndex);
                const auto status=AMediaMuxer_start(mMuxer);
                //LOGD("Media Muxer status %d",status);
                AMediaFormat_delete(format);
                //Write SEI ?!
                lastFeed=std::chrono::steady_clock::now();
            }
        }else{
            AMediaCodecBufferInfo info;
            info.offset=0;
            info.size=nalu.data_length;
            const auto now=std::chrono::steady_clock::now();
            const auto duration=now-lastFeed;
            lastFeed=now;
            info.presentationTimeUs=std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
            //info.flags=0;
            info.flags=AMEDIACODEC_CONFIGURE_FLAG_ENCODE; //1
            AMediaMuxer_writeSampleData(mMuxer,mTrackIndex,nalu.data,&info);
        }
    }
private:
    const std::string GROUND_RECORDING_DIRECTORY;
    int mFD;
    AMediaMuxer* mMuxer=nullptr;
    ssize_t mTrackIndex;
    KeyFrameFinder mKeyFrameFInder;
    std::chrono::steady_clock::time_point lastFeed;
};


#endif //LIVEVIDEO10MS_GROUNDRECORDERMP4_HPP