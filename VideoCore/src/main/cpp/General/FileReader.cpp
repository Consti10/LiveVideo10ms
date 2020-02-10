//
// Created by geier on 07/02/2020.
//

#include "FileReader.h"
#include "../NALU/NALU.hpp"
#include <iterator>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <fstream>

static bool endsWith(const std::string& str, const std::string& suffix){
    return str.size() >= suffix.size() && 0 == str.compare(str.size()-suffix.size(), suffix.size(), suffix);
}
static const constexpr auto TAG="FileReader";
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)

void FileReader::passDataInChunks(const std::vector<uint8_t> &data) {
    if(data.size()==0){
        return;
    }
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

std::vector<uint8_t> FileReader::getBufferFromMediaFormat(const char *name, AMediaFormat *format) {
    uint8_t* data;
    size_t data_size;
    AMediaFormat_getBuffer(format,name,(void**)&data,&data_size);
    std::vector<uint8_t> ret(data_size);
    memcpy(ret.data(),data,data_size);
    return ret;
}

ssize_t FileReader::AMediaExtractor_readSampleDataCPP(AMediaExtractor *extractor,std::vector<uint8_t> &data) {
    data.resize(NALU::NALU_MAXLEN);
    const auto sampleSize=AMediaExtractor_readSampleData(extractor,data.data(),data.size());
    data.resize(sampleSize >= 0 ? (unsigned) sampleSize : 0);
    return sampleSize;
}

//Append whole content of vector1 to vector0, why is there no cpp standart for that ?!
template<typename T>
static void vector_append(std::vector<T>& vector0,const std::vector<T> vector1){
    vector0.insert(vector0.end(),vector1.begin(),vector1.end());
}

std::vector<uint8_t>
FileReader::loadAssetAsRawVideoStream(AAssetManager *assetManager, const std::string &path) {
    if(endsWith(path,".mp4")){
        //Use MediaExtractor to parse .mp4 file
        AAsset* asset = AAssetManager_open(assetManager,path.c_str(), 0);
        off_t start, length;
        auto fd=AAsset_openFileDescriptor(asset,&start,&length);
        if(fd<0){
            LOGD("ERROR AAsset_openFileDescriptor %d",fd);
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
                vector_append(rawData,tmp);
                tmp=getBufferFromMediaFormat("csd-1",format);
                vector_append(rawData,tmp);
                LOGD("Video track found %d %s",mediaStatus, AMediaFormat_toString(format));
                break;
            }
            AMediaFormat_delete(format);
        }
        std::vector<uint8_t> buffer;
        while(true){
            const auto sampleSize=FileReader::AMediaExtractor_readSampleDataCPP(extractor,buffer);
            if(sampleSize<0){
                break;
            }
            const auto flags=AMediaExtractor_getSampleFlags(extractor);
            LOGD("Read sample %d flags %d",sampleSize,flags);
            const NALU nalu(buffer.data(),sampleSize);
            LOGD("NALU %s",nalu.get_nal_name().c_str());
            vector_append(rawData,buffer);
            AMediaExtractor_advance(extractor);
        }
        AMediaExtractor_delete(extractor);
        return rawData;
    }else if(endsWith(path,".h264")){
        //Read raw data from file (without MediaExtractor)
        AAsset* asset = AAssetManager_open(assetManager, path.c_str(), 0);
        if(!asset){
            LOGD("Error asset not found:%s",path.c_str());
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
    LOGD("Error not supported file %s",path.c_str());
    return std::vector<uint8_t>();
}


void FileReader::receiveLoop() {
    nReceivedB=0;
    if(assetManager!= nullptr){
        //load once into memory, then loop (repeating) until done
        const auto data=loadAssetAsRawVideoStream(assetManager,filename);
        while(receiving){
            passDataInChunks(data);
        }
    }else{
        //load and pass small chunks one by one, reset to beginning of file when done
        parseFileAsRawVideoStream(filename);
    }
}

//Helper, return size of file in bytes
ssize_t fsize(const char *filename) {
    struct stat st;
    if (stat(filename, &st) == 0)
        return st.st_size;
    return -1;
}

void FileReader::parseFileAsRawVideoStream(const std::string &filename) {
    if(endsWith(filename,".mp4")){
        const auto fileSize=fsize(filename.c_str());
        if(fileSize<=0){
            LOGD("Error file size %d",(int)fileSize);
            return;
        }
        const int fd = open(filename.c_str(), O_RDONLY, 0666);
        if(fd<0){
            LOGD("Error open file: %s fp: %d",filename.c_str(),fd);
            return;
        }
        AMediaExtractor* extractor=AMediaExtractor_new();
        auto mediaStatus=AMediaExtractor_setDataSourceFd(extractor,fd,0,fileSize);
        if(mediaStatus!=AMEDIA_OK){
            LOGD("Error open File %s,mediaStatus: %d",filename.c_str(),mediaStatus);
            AMediaExtractor_delete(extractor);
            close(fd);
            return;
        }
        const auto trackCount=AMediaExtractor_getTrackCount(extractor);
        bool videoTrackFound=false;
        std::vector<uint8_t> csd0;
        std::vector<uint8_t> csd1;
        for(size_t i=0;i<trackCount;i++){
            AMediaFormat* format= AMediaExtractor_getTrackFormat(extractor,i);
            const char* s;
            AMediaFormat_getString(format,AMEDIAFORMAT_KEY_MIME,&s);
            LOGD("Track is %s",s);
            if(std::string(s).compare("video/avc")==0){
                mediaStatus=AMediaExtractor_selectTrack(extractor,i);
                csd0=getBufferFromMediaFormat("csd-0",format);
                csd1=getBufferFromMediaFormat("csd-1",format);
                LOGD("Video track found %d %s",mediaStatus, AMediaFormat_toString(format));
                AMediaFormat_delete(format);
                videoTrackFound=true;
                break;
            }
        }
        if(!videoTrackFound){
            LOGD("Cannot find video track");
            AMediaExtractor_delete(extractor);
            close(fd);
        }
        //All good, feed configuration, then load & feed data one by one
        passDataInChunks(csd0);
        passDataInChunks(csd1);
        //Loop until done
        std::vector<uint8_t> buffer;
        while(receiving){
            const auto sampleSize=FileReader::AMediaExtractor_readSampleDataCPP(extractor,buffer);
            if(sampleSize<0){
                //TODO seek back to 0
                break;
            }
            const auto flags=AMediaExtractor_getSampleFlags(extractor);
            passDataInChunks(buffer);
            AMediaExtractor_advance(extractor);
        }
        AMediaExtractor_delete(extractor);
        close(fd);
    }else{
        std::ifstream file (filename.c_str(), std::ios::in|std::ios::binary|std::ios::ate);
        if (!file.is_open()) {
            LOGD("Cannot open file %s",filename.c_str());
        } else {
            LOGD("Opened File %s",filename.c_str());
            file.seekg (0, std::ios::beg);
            std::vector<uint8_t> buffer(NALU::NALU_MAXLEN);
            while (receiving) {
                if(file.eof()){
                    file.clear();
                    file.seekg (0, std::ios::beg);
                }
                buffer.resize(NALU::NALU_MAXLEN);
                file.read((char*)buffer.data(), buffer.size());
                std::streamsize dataSize = file.gcount();
                buffer.resize(dataSize>=0 ?(unsigned) dataSize : 0);
                passDataInChunks(buffer);
            }
            file.close();
        }
    }
}

