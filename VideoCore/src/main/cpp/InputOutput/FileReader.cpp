//
// Created by geier on 07/02/2020.
//

#include "FileReader.h"
#include <iterator>
#include <array>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <memory>

static bool endsWith(const std::string& str, const std::string& suffix){
    return str.size() >= suffix.size() && 0 == str.compare(str.size()-suffix.size(), suffix.size(), suffix);
}
static const constexpr auto TAG="FileReader";
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)

//We cannot use recursion due to stack pointer size limitation. -> Use loop instead.
void FileReader::passDataInChunks(const uint8_t data[],const size_t size) {
    /*if(!receiving || size==0)return;
    if(size>CHUNK_SIZE){
        nReceivedB+=CHUNK_SIZE;
        onDataReceivedCallback(data,CHUNK_SIZE);
        passDataInChunks(&data[CHUNK_SIZE],size-CHUNK_SIZE);
    }else{
        LOGD("Size < %d",size);
        nReceivedB+=size;
        onDataReceivedCallback(data,size);
    }*/
    int offset=0;
    const ssize_t len=size;
    while(receiving){
        const ssize_t len_left=len-offset;
        if(len_left>=CHUNK_SIZE){
            nReceivedB+=CHUNK_SIZE;
            onDataReceivedCallback(&data[offset],CHUNK_SIZE);
            offset+=CHUNK_SIZE;
        }else{
            nReceivedB+=len_left;
            onDataReceivedCallback(&data[offset],(size_t)len_left);
            return;
        }
    }
}

void FileReader::passDataInChunks(const std::vector<uint8_t> &data) {
    LOGD("passDataInChunks %d",(int)data.size());
    passDataInChunks(data.data(),data.size());
}

std::vector<uint8_t> FileReader::getBufferFromMediaFormat(const char *name, AMediaFormat *format) {
    uint8_t* data;
    size_t data_size;
    AMediaFormat_getBuffer(format,name,(void**)&data,&data_size);
    std::vector<uint8_t> ret(data_size);
    memcpy(ret.data(),data,data_size);
    return ret;
}

//Append whole content of vector1 to vector0, why is there no cpp standart for that ?!
static inline void vector_append(std::vector<uint8_t>& destination,const std::vector<uint8_t>& source){
    //vector0.insert(vector0.end(),vector1.begin(),vector1.end());
    const auto vector0InitialSize=destination.size();
    destination.resize(vector0InitialSize+source.size());
    memcpy(&destination.data()[vector0InitialSize],source.data(),source.size());
}
//the ndk insert() gives warnings with IntelliJ and does not work with vector+array
template<size_t S>
static inline void vector_append2(std::vector<uint8_t>& destination,const std::array<uint8_t,S>& source,const size_t sourceSize){
    //vector0.insert(vector0.end(),vector1.begin(),vector1.begin()+vector1Size);
    const auto vector0InitialSize=destination.size();
    destination.resize(vector0InitialSize+sourceSize);
    memcpy(&destination.data()[vector0InitialSize],source.data(),sourceSize);
}

std::vector<uint8_t>
FileReader::loadRawAssetFileIntoMemory(AAssetManager *assetManager, const std::string &path) {
    //Read raw data from file (without MediaExtractor)
    AAsset* asset = AAssetManager_open(assetManager, path.c_str(), 0);
    if(!asset){
        LOGD("Error asset not found:%s",path.c_str());
        return std::vector<uint8_t>();
    }
    const size_t size=(size_t)AAsset_getLength(asset);
    AAsset_seek(asset,0,SEEK_SET);
    std::vector<uint8_t> rawData(size);
    const auto len=AAsset_read(asset,rawData.data(),size);
    AAsset_close(asset);
    LOGD("The entire file content (asset,raw) is in memory %d",(int)rawData.size());
    return rawData;
}

std::vector<uint8_t>
FileReader::loadConvertMP4AssetFileIntoMemory(AAssetManager *assetManager, const std::string &path) {
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
    const auto sampleBuffer=std::make_unique<std::array<uint8_t,NALU_BUFF_SIZE>>();
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
}

std::vector<uint8_t>
FileReader::loadAssetFileIntoMemory(AAssetManager *assetManager, const std::string &path) {
    if(endsWith(path,".mp4")){
        return loadConvertMP4AssetFileIntoMemory(assetManager,path);
    }else{
        return loadRawAssetFileIntoMemory(assetManager,path);
    }
}

void FileReader::receiveLoop() {
    nReceivedB=0;
    if(assetManager!= nullptr){
        //load once into memory, then loop (repeating) until done
        const auto data= loadAssetFileIntoMemory(assetManager, FILENAME);
        while(receiving){
            passDataInChunks(data);
        }
    }else{
        //load and pass small chunks one by one, reset to beginning of file when done
        readFileInChunks();
    }
}

void FileReader::readFileInChunks() {
    if(endsWith(FILENAME,".mp4")){
        readMP4FileInChunks();
    }else{
        readRawFileInChunks();
    }
}

//Helper, return size of file in bytes
ssize_t fsize(const char *filename) {
    struct stat st;
    if (stat(filename, &st) == 0)
        return st.st_size;
    return -1;
}

void FileReader::readMP4FileInChunks() {
    const auto fileSize=fsize(FILENAME.c_str());
    if(fileSize<=0){
        LOGD("Error file size %d",(int)fileSize);
        return;
    }
    const int fd = open(FILENAME.c_str(), O_RDONLY, 0666);
    if(fd<0){
        LOGD("Error open file: %s fp: %d",FILENAME.c_str(),fd);
        return;
    }
    AMediaExtractor* extractor=AMediaExtractor_new();
    auto mediaStatus=AMediaExtractor_setDataSourceFd(extractor,fd,0,fileSize);
    if(mediaStatus!=AMEDIA_OK){
        LOGD("Error open File %s,mediaStatus: %d",FILENAME.c_str(),mediaStatus);
        AMediaExtractor_delete(extractor);
        close(fd);
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
            passDataInChunks(csd0);
            const auto csd1=getBufferFromMediaFormat("csd-1",format);
            passDataInChunks(csd1);
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
        return;
    }
    //All good, feed configuration, then load & feed data one by one
    //Loop until done
    //We cannot allocate such a big object on the stack, so we need to wrap into unique ptr
    const auto sampleBuffer=std::make_unique<std::array<uint8_t,NALU_BUFF_SIZE>>();
    while(receiving){
        const auto sampleSize=AMediaExtractor_readSampleData(extractor,sampleBuffer->data(),sampleBuffer->size());
        if(sampleSize<=0){
            AMediaExtractor_seekTo(extractor,0, AMEDIAEXTRACTOR_SEEK_CLOSEST_SYNC);
            continue;
        }
        passDataInChunks(sampleBuffer->data(),(size_t)sampleSize);
        AMediaExtractor_advance(extractor);
    }
    AMediaExtractor_delete(extractor);
    close(fd);
}

void FileReader::readRawFileInChunks() {
    std::ifstream file (FILENAME.c_str(), std::ios::in|std::ios::binary|std::ios::ate);
    if (!file.is_open()) {
        LOGD("Cannot open file %s",FILENAME.c_str());
        return;
    } else {
        LOGD("Opened File %s",FILENAME.c_str());
        file.seekg (0, std::ios::beg);
        const auto buffer=std::make_unique<std::array<uint8_t,NALU_BUFF_SIZE>>();
        while (receiving) {
            if(file.eof()){
                file.clear();
                file.seekg (0, std::ios::beg);
            }
            file.read((char*)buffer->data(), buffer->size());
            std::streamsize dataSize = file.gcount();
            if(dataSize>0){
                passDataInChunks(buffer->data(),(size_t)dataSize);
            }
        }
        file.close();
    }
}
