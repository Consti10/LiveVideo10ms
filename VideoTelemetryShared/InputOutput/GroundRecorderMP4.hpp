//
// Created by geier on 14/02/2020.
//

#ifndef LIVEVIDEO10MS_GROUNDRECORDERMP4_HPP
#define LIVEVIDEO10MS_GROUNDRECORDERMP4_HPP

#include <string>
#include <media/NdkMediaMuxer.h>
#include <unistd.h>
#include <fcntl.h>
#include <FileHelper.hpp>

//TODO work in progress
//DEPRECATED

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
    void writeData(const uint8_t* data, size_t data_length) {
        if(mMuxer== nullptr){
            const std::string fn=FileHelper::findUnusedFilename(GROUND_RECORDING_DIRECTORY,"mp4");
            mFD = open(fn.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
            mMuxer=AMediaMuxer_new(mFD,AMEDIAMUXER_OUTPUT_FORMAT_MPEG_4);
            AMediaFormat*format=AMediaFormat_new();

            AMediaFormat_setString(format,AMEDIAFORMAT_KEY_MIME,"video/raw");
            AMediaFormat_setInt32(format,AMEDIAFORMAT_KEY_WIDTH,640);
            AMediaFormat_setInt32(format,AMEDIAFORMAT_KEY_HEIGHT,480);
            AMediaFormat_setInt32(format,AMEDIAFORMAT_KEY_FRAME_RATE,30);
            mTrackIndex=AMediaMuxer_addTrack(mMuxer,format);
            //LOGD("Media Muxer track index %d",(int)mTrackIndex);
            const auto status=AMediaMuxer_start(mMuxer);
            MLOGD<<"Media Muxer status "<<status;
            AMediaFormat_delete(format);
            //Write SEI ?!
            lastFeed=std::chrono::steady_clock::now();
        }else{
            AMediaCodecBufferInfo info;
            info.offset=0;
            info.size=data_length;
            const auto now=std::chrono::steady_clock::now();
            const auto duration=now-lastFeed;
            lastFeed=now;
            info.presentationTimeUs=std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
            //info.flags=0;
            info.flags=AMEDIACODEC_CONFIGURE_FLAG_ENCODE; //1
            AMediaMuxer_writeSampleData(mMuxer,mTrackIndex,data,&info);
        }
    }
private:
    const std::string GROUND_RECORDING_DIRECTORY;
    int mFD;
    AMediaMuxer* mMuxer=nullptr;
    ssize_t mTrackIndex;
    std::chrono::steady_clock::time_point lastFeed;
};


#endif //LIVEVIDEO10MS_GROUNDRECORDERMP4_HPP