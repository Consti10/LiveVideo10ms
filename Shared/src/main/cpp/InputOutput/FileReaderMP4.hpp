//
// Created by geier on 20/03/2020.
//

#ifndef TELEMETRY_FILEREADERMP4_HPP
#define TELEMETRY_FILEREADERMP4_HPP

#include <vector>
#include <chrono>
#include <string>
#include <array>
#include <sstream>
#include <fstream>
#include <iterator>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <memory>
#include <android/asset_manager.h>
#include <media/NdkMediaExtractor.h>
#include <AndroidLogger.hpp>

/**
 * Namespace that holds utility functions for reading MP4 files
 */
namespace FileReaderMP4{
    static constexpr const size_t MAX_NALU_BUFF_SIZE=1024*1024;
    typedef std::function<void(const uint8_t[],std::size_t)> RAW_DATA_CALLBACK;
    /**
    * Utility for MediaFormat Handling, return buffer as std::vector that owns memory
    */
    static std::vector<uint8_t>
    getBufferFromMediaFormat(const char *name, AMediaFormat *format) {
        uint8_t* data;
        size_t data_size;
        AMediaFormat_getBuffer(format,name,(void**)&data,&data_size);
        return std::vector<uint8_t>(data,data+data_size);
    }
    /**
     *  Helper, return size of file in bytes
     */
    ssize_t fsize(const char *filename) {
        struct stat st;
        if (stat(filename, &st) == 0)
            return (ssize_t)st.st_size;
        return -1;
    }
    /**
     * Open mp4 file, extract data as h264 bitstream
     * If @param assetManager != nullptr, @param FILENAME points to android asset file,else to a file on the phone file system
     * @param receiving termination condition (loops until receiving==false)
     */
    static void readMP4FileInChunks(AAssetManager *assetManager,const std::string &FILENAME,RAW_DATA_CALLBACK callback,const std::future<void>& shouldTerminate,const bool loopAtEndOfFile=true) {
        int fileOffset=0;
        int fileSize;
        int fd;
        AAsset* asset= nullptr;
        if(assetManager!= nullptr){
            asset = AAssetManager_open(assetManager,FILENAME.c_str(), 0);
            off_t start,size;
            fd=AAsset_openFileDescriptor(asset,&start,&size);
            fileOffset=start;
            fileSize=size;
            if(fd<0){
                MLOGE<<"AAsset_openFileDescriptor "<<fd;
                return;
            }
        }else{
            fileSize=fsize(FILENAME.c_str());
            fileOffset=0;
            if(fileSize<=0){
                MLOGE<<"file size "<<fileSize;
                return;
            }
            fd = open(FILENAME.c_str(), O_RDONLY, 0666);
            if(fd<0){
                MLOGE<<"open file: "<<FILENAME<<" "<<fd;
                return;
            }
        }
        AMediaExtractor* extractor=AMediaExtractor_new();
        auto mediaStatus=AMediaExtractor_setDataSourceFd(extractor,fd,fileOffset,fileSize);
        if(mediaStatus!=AMEDIA_OK){
            MLOGE<<"Error open File "<<FILENAME<<" mediaStatus: "<<mediaStatus;
            AMediaExtractor_delete(extractor);
            if(assetManager!=nullptr){
                AAsset_close(asset);
            }else{
                close(fd);
            }
            return;
        }
        const auto trackCount=AMediaExtractor_getTrackCount(extractor);
        bool videoTrackFound=false;
        for(size_t i=0;i<trackCount;i++){
            AMediaFormat* format= AMediaExtractor_getTrackFormat(extractor,i);
            const char* s;
            AMediaFormat_getString(format,AMEDIAFORMAT_KEY_MIME,&s);
            MLOGD<<"Track is"<<s;
            if(std::string(s).compare("video/avc")==0){
                mediaStatus=AMediaExtractor_selectTrack(extractor,i);
                const auto csd0=getBufferFromMediaFormat("csd-0",format);
                callback(csd0.data(),csd0.size());
                const auto csd1=getBufferFromMediaFormat("csd-1",format);
                callback(csd1.data(),csd1.size());
                MLOGD<<"Video track found "<<mediaStatus<<" "<<AMediaFormat_toString(format);
                AMediaFormat_delete(format);
                videoTrackFound=true;
                break;
            }
        }
        if(!videoTrackFound){
            MLOGE<<"Cannot find video track";
            AMediaExtractor_delete(extractor);
            if(assetManager!=nullptr){
                AAsset_close(asset);
            }else{
                close(fd);
            }
            return;
        }
        //All good, feed configuration, then load & feed data one by one
        //Loop until done
        //We cannot allocate such a big object on the stack, so we need to wrap into unique ptr
        const auto sampleBuffer=std::make_unique<std::array<uint8_t,MAX_NALU_BUFF_SIZE>>();
        //warning 'not updated' is no problem, since passed by reference
        while(shouldTerminate.wait_for(std::chrono::milliseconds(0)) == std::future_status::timeout){
            const auto sampleSize=AMediaExtractor_readSampleData(extractor,sampleBuffer->data(),sampleBuffer->size());
            if(sampleSize<=0){
                if(loopAtEndOfFile){
                    AMediaExtractor_seekTo(extractor,0, AMEDIAEXTRACTOR_SEEK_CLOSEST_SYNC);
                    continue;
                }else{
                    break;
                }
            }
            callback(sampleBuffer->data(),(size_t)sampleSize);
            AMediaExtractor_advance(extractor);
        }
        AMediaExtractor_delete(extractor);
        if(assetManager!=nullptr){
            AAsset_close(asset);
        }else{
            close(fd);
        }
    }
    /**
     * using the method above, loads bitstream into memory as one big buffer
     * @param assetManager
     * @param path
     * @return
     */
    static std::vector<uint8_t>
    loadConvertMP4AssetFileIntoMemory(AAssetManager *assetManager, const std::string &path){
        //This will save all data as RAW
        //SPS/PPS in the beginning, rest afterwards
        std::promise<void> exitSignal;
        std::future<void> futureObj = exitSignal.get_future();
        std::vector<uint8_t> rawData;
        rawData.reserve(1024*1024);
        readMP4FileInChunks(assetManager,path, [&rawData](const uint8_t* d,int len) {
            const auto offset=rawData.size();
            rawData.resize(rawData.size()+len);
            memcpy(&rawData.at(offset),d,(size_t)len);
        },futureObj,false);
        return rawData;
    }
}

#endif //TELEMETRY_FILEREADERMP4_HPP
