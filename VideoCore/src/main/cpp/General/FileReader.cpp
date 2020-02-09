//
// Created by geier on 07/02/2020.
//

#include "FileReader.h"
#include "../NALU/NALU.hpp"
#include <iterator>
#include <cstring>
#include <fcntl.h>

static bool endsWith(const std::string& str, const std::string& suffix){
    return str.size() >= suffix.size() && 0 == str.compare(str.size()-suffix.size(), suffix.size(), suffix);
}
static const constexpr auto TAG="FileReader";
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)

void FileReader::passDataInChunks(const std::vector<uint8_t> &data) {
    int offset=0;
    const int len=(int)data.size();
    while(receiving){
        const int len_left=len-offset;
        if(len_left>=CHUNK_SIZE){
            nReceivedB+=CHUNK_SIZE;
            onDataReceivedCallback(&data[offset],CHUNK_SIZE);
            offset+=CHUNK_SIZE;
        }else{
            nReceivedB+=len_left;
            onDataReceivedCallback(&data[offset],len_left);
            return;
        }
    }
}

std::vector<uint8_t>
FileReader::loadAssetAsRawVideoStream(AAssetManager *assetManager, const std::string &filename) {
    if(endsWith(filename,".mp4")){
        AAsset* asset = AAssetManager_open(assetManager,filename.c_str(), 0);
        off_t start, length;
        auto fd=AAsset_openFileDescriptor(asset,&start,&length);
        if(fd<0){
            LOGD("Error fd %d",fd);
            return std::vector<uint8_t>();
        }
        AMediaExtractor* extractor=AMediaExtractor_new();
        AMediaExtractor_setDataSourceFd(extractor,fd,start,length);
        const auto trackCount=AMediaExtractor_getTrackCount(extractor);
        //This will save all data as RAW
        //SPS/PPS in the beginning, rest afterwards
        std::vector<uint8_t> rawData;
        for(size_t i=0;i<trackCount;i++){
            AMediaFormat* format= AMediaExtractor_getTrackFormat(extractor,i);
            const char* s;
            AMediaFormat_getString(format,AMEDIAFORMAT_KEY_MIME,&s);
            LOGD("Track is %s",s);
            if(std::string(s).compare("video/avc")==0){
                const auto mediaStatus=AMediaExtractor_selectTrack(extractor,i);
                auto tmp=getBufferFromMediaFormat("csd-0",format);
                rawData.insert(rawData.end(),tmp.begin(),tmp.end());
                tmp=getBufferFromMediaFormat("csd-1",format);
                rawData.insert(rawData.end(),tmp.begin(),tmp.end());
                LOGD("Video track found %d %s",mediaStatus, AMediaFormat_toString(format));
                break;
            }
            AMediaFormat_delete(format);
        }
        std::vector<uint8_t> tmpBuffer(1024*1024);
        while(true){
            const auto sampleSize=AMediaExtractor_readSampleData(extractor,tmpBuffer.data(),tmpBuffer.size());
            const auto flags=AMediaExtractor_getSampleFlags(extractor);
            LOGD("Read sample %d flags %d",sampleSize,flags);
            const NALU nalu(tmpBuffer.data(),sampleSize);
            LOGD("NALU %s",nalu.get_nal_name().c_str());
            if(sampleSize<0){
                break;
            }
            rawData.insert(rawData.end(),tmpBuffer.begin(),tmpBuffer.begin()+sampleSize);
            AMediaExtractor_advance(extractor);
        }
        AMediaExtractor_delete(extractor);
        return rawData;
    }else if(endsWith(filename,".h264")){
        AAsset* asset = AAssetManager_open(assetManager, filename.c_str(), 0);
        if(!asset){
            LOGD("Error asset not found:%s",filename.c_str());
        }else{
            const size_t size=(size_t)AAsset_getLength(asset);
            AAsset_seek(asset,0,SEEK_SET);
            std::vector<uint8_t> rawData(size);
            int len=AAsset_read(asset,rawData.data(),size);
            AAsset_close(asset);
            LOGD("The entire file content (asset) is in memory %d",(int)size);
            return rawData;
        }
    }
    LOGD("Error not supported file %s",filename.c_str());
    return std::vector<uint8_t>();
}

std::vector<uint8_t> FileReader::getBufferFromMediaFormat(const char *name, AMediaFormat *format) {
    uint8_t* data;
    size_t data_size;
    AMediaFormat_getBuffer(format,name,(void**)&data,&data_size);
    std::vector<uint8_t> ret(data_size);
    memcpy(ret.data(),data,data_size);
    return ret;
}

void FileReader::receiveLoop() {
    nReceivedB=0;
    if(assetManager!= nullptr){
        const auto data=loadAssetAsRawVideoStream(assetManager,"testRoom1_1920Mono.mp4");
        passDataInChunks(data);
    }else{
        /*std::ifstream file (filename.c_str(), std::ios::in|std::ios::binary|std::ios::ate);
        if (!file.is_open()) {
            LOGD("Cannot open file %s",filename.c_str());
        } else {
            const auto size = file.tellg();
            std::vector<uint8_t> buff(size);
            file.seekg (0, std::ios::beg);
            file.read ((char*)buff.data(), size);
            file.close();
            LOGD("the entire file content is in memory");
            while(receiving){
                passDataInChunks(buff);
            }
        }*/
        parseMP4FileAsRawVideoStream(filename);
    }
}

void FileReader::parseMP4FileAsRawVideoStream(const std::string &filename) {
    AMediaExtractor* extractor=AMediaExtractor_new();
    auto fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);

    //const auto mediaStatus=AMediaExtractor_setDataSource(extractor,filename.c_str());
    off_t start, length;
    const auto mediaStatus=AMediaExtractor_setDataSourceFd(extractor,fd,start,length);
    if(mediaStatus!=AMEDIA_OK){
        LOGD("Error open File %s,mediaStatus: %d",filename.c_str(),mediaStatus);
    }
    const auto trackCount=AMediaExtractor_getTrackCount(extractor);
    for(size_t i=0;i<trackCount;i++){
        AMediaFormat* format= AMediaExtractor_getTrackFormat(extractor,i);
        const char* s;
        AMediaFormat_getString(format,AMEDIAFORMAT_KEY_MIME,&s);
        LOGD("Track is %s",s);
        if(std::string(s).compare("video/avc")==0){
            const auto mediaStatus=AMediaExtractor_selectTrack(extractor,i);
            auto tmp=getBufferFromMediaFormat("csd-0",format);
            passDataInChunks(tmp);
            tmp=getBufferFromMediaFormat("csd-1",format);
            passDataInChunks(tmp);
            LOGD("Video track found %d %s",mediaStatus, AMediaFormat_toString(format));
            break;
        }
        AMediaFormat_delete(format);
        //Loop until done
        std::vector<uint8_t> tmpBuffer(1024*1024);
        while(receiving){
            const auto sampleSize=AMediaExtractor_readSampleData(extractor,tmpBuffer.data(),tmpBuffer.size());
            const auto flags=AMediaExtractor_getSampleFlags(extractor);
            //LOGD("Read sample %d flags %d",sampleSize,flags);
            //const NALU nalu(tmpBuffer.data(),sampleSize);
            //LOGD("NALU %s",nalu.get_nal_name().c_str());
            if(sampleSize<0){
                break;
            }
            std::vector<uint8_t> tmp;
            tmp.insert(tmp.end(),tmpBuffer.begin(),tmpBuffer.begin()+sampleSize);
            passDataInChunks(tmp);
            AMediaExtractor_advance(extractor);
        }
        AMediaExtractor_delete(extractor);
    }
}
