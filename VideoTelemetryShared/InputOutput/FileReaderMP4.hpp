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
#include <MDebug.hpp>

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
        std::vector<uint8_t> ret(data_size);
        memcpy(ret.data(),data,data_size);
        return ret;
    }
    /**
     * Append whole content of vector1 to vector0, why is there no cpp standard for that ?!
     */
    static inline void vector_append(std::vector<uint8_t>& destination,const std::vector<uint8_t>& source){
        //vector0.insert(vector0.end(),vector1.begin(),vector1.end());
        const auto vector0InitialSize=destination.size();
        destination.resize(vector0InitialSize+source.size());
        memcpy(&destination.data()[vector0InitialSize],source.data(),source.size());
    }
    /**
     *  the ndk insert() gives warnings with IntelliJ and does not work with vector+array
     */
    template<size_t S>
    static inline void vector_append2(std::vector<uint8_t>& destination,const std::array<uint8_t,S>& source,const size_t sourceSize){
        //vector0.insert(vector0.end(),vector1.begin(),vector1.begin()+vector1Size);
        const auto vector0InitialSize=destination.size();
        destination.resize(vector0InitialSize+sourceSize);
        memcpy(&destination.data()[vector0InitialSize],source.data(),sourceSize);
    }
    /**
     *  return size of file in bytes
     */
    ssize_t fsize(const char *filename) {
        struct stat st;
        if (stat(filename, &st) == 0)
            return st.st_size;
        return -1;
    }
    /**
     * Instead of loading mp4 file into memory, keep data in file system and pass it one by one
     * @param assetManager if nullptr, @param FILENAME points to a file on the phone file system, else to android asset file
     * @param receiving termination condition (loops until receiving==false)
     */
    static void readMP4FileInChunks(AAssetManager *assetManager,const std::string &FILENAME,RAW_DATA_CALLBACK callback,std::atomic<bool>& receiving,const bool loopAtEndOfFile=true) {
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
                LOGD("ERROR AAsset_openFileDescriptor %d",fd);
                return;
            }
        }else{
            fileSize=fsize(FILENAME.c_str());
            fileOffset=0;
            if(fileSize<=0){
                LOGD("Error file size %d",fileSize);
                return;
            }
            fd = open(FILENAME.c_str(), O_RDONLY, 0666);
            if(fd<0){
                LOGD("Error open file: %s fp: %d",FILENAME.c_str(),fd);
                return;
            }
        }
        AMediaExtractor* extractor=AMediaExtractor_new();
        auto mediaStatus=AMediaExtractor_setDataSourceFd(extractor,fd,fileOffset,fileSize);
        if(mediaStatus!=AMEDIA_OK){
            LOGD("Error open File %s,mediaStatus: %d",FILENAME.c_str(),mediaStatus);
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
            LOGD("Track is %s",s);
            if(std::string(s).compare("video/avc")==0){
                mediaStatus=AMediaExtractor_selectTrack(extractor,i);
                const auto csd0=getBufferFromMediaFormat("csd-0",format);
                callback(csd0.data(),csd0.size());
                const auto csd1=getBufferFromMediaFormat("csd-1",format);
                callback(csd1.data(),csd1.size());
                LOGD("Video track found %d %s",mediaStatus, AMediaFormat_toString(format));
                AMediaFormat_delete(format);
                videoTrackFound=true;
                break;
            }
        }
        if(!videoTrackFound){
            LOGD("Cannot find video track");
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
        while(receiving){
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
   * convert the asset file specified at path from .mp4 into a raw .h264 bitstream
   * then return as one big data buffer
   */
    /*static std::vector<uint8_t>
    loadConvertMP4AssetFileIntoMemory(AAssetManager *assetManager, const std::string &path){
        //Use MediaExtractor to parse .mp4 file
        AAsset* asset = AAssetManager_open(assetManager,path.c_str(), 0);
        off_t start, length;
        auto fd=AAsset_openFileDescriptor(asset,&start,&length);
        if(fd<0){
            LOGD("ERROR AAsset_openFileDescriptor %d",fd);
            return std::vector<uint8_t>();
        }
        AMediaExtractor* extractor=AMediaExtractor_new();
        auto mediaStatus=AMediaExtractor_setDataSourceFd(extractor,fd,start,length);
        if(mediaStatus!=AMEDIA_OK){
            LOGD("Error open File %s,mediaStatus: %d",path.c_str(),mediaStatus);
            AMediaExtractor_delete(extractor);
            AAsset_close(asset);
        }
        const auto trackCount=AMediaExtractor_getTrackCount(extractor);
        //This will save all data as RAW
        //SPS/PPS in the beginning, rest afterwards
        std::vector<uint8_t> rawData;
        rawData.reserve((unsigned)length);
        for(size_t i=0;i<trackCount;i++){
            AMediaFormat* format= AMediaExtractor_getTrackFormat(extractor,i);
            const char* s;
            AMediaFormat_getString(format,AMEDIAFORMAT_KEY_MIME,&s);
            LOGD("Track is %s",s);
            if(std::string(s).compare("video/avc")==0){
                const auto mediaStatus=AMediaExtractor_selectTrack(extractor,i);
                auto tmp=getBufferFromMediaFormat("csd-0",format);
                vector_append(rawData,tmp);
                tmp=getBufferFromMediaFormat("csd-1",format);
                vector_append(rawData,tmp);
                LOGD("Video track found %d %s",mediaStatus, AMediaFormat_toString(format));
                break;
            }
            AMediaFormat_delete(format);
        }
        //We cannot allocate such a big object on the stack, so we need to wrap into unique ptr
        const auto sampleBuffer=std::make_unique<std::array<uint8_t,MAX_NALU_BUFF_SIZE>>();
        while(true){
            const auto sampleSize=AMediaExtractor_readSampleData(extractor,sampleBuffer->data(),sampleBuffer->size());
            if(sampleSize<=0){
                break;
            }
            const auto flags=AMediaExtractor_getSampleFlags(extractor);
            //LOGD("Read sample %d flags %d",(int)sampleSize,flags);
            vector_append2(rawData,(*sampleBuffer),(size_t)sampleSize);
            AMediaExtractor_advance(extractor);
        }
        AMediaExtractor_delete(extractor);
        AAsset_close(asset);
        LOGD("The entire file content (asset,mp4) is in memory %d",(int)rawData.size());
        return rawData;
    }*/
    static std::vector<uint8_t>
    loadConvertMP4AssetFileIntoMemory(AAssetManager *assetManager, const std::string &path){
        //This will save all data as RAW
        //SPS/PPS in the beginning, rest afterwards
        std::atomic<bool> fill=true;
        std::vector<uint8_t> rawData;
        rawData.reserve(1024*1024);
        readMP4FileInChunks(assetManager,path, [&rawData](const uint8_t* d,int len) {
            const auto offset=rawData.size();
            rawData.resize(rawData.size()+len);
            memcpy(&rawData.at(offset),d,(size_t)len);
        },fill,false);
        return rawData;
    }
}

#endif //TELEMETRY_FILEREADERMP4_HPP
