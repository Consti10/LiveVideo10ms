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
#include "GroundRecorderRAW.hpp"
#include "GroundRecorderFPV.hpp"
#include "FileReaderMP4.hpp"
#include "FileReaderRAW.hpp"

static bool endsWith(const std::string& str, const std::string& suffix){
    return str.size() >= suffix.size() && 0 == str.compare(str.size()-suffix.size(), suffix.size(), suffix);
}

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


std::vector<uint8_t>
FileReader::loadAssetFileIntoMemory(AAssetManager *assetManager, const std::string &path) {
    if(endsWith(path,".mp4")){
        return FileReaderMP4::loadConvertMP4AssetFileIntoMemory(assetManager,path);
    }else{
        return FileReaderRAW::loadRawAssetFileIntoMemory(assetManager,path);
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
    //std::bind(&FileReader::passDataInChunks, this, std::placeholders::_1,std::placeholders::_2)
    const auto f = [this](const uint8_t *data, size_t data_length) {
        passDataInChunks(data, data_length);
    };
    if(endsWith(FILENAME,".mp4")) {
        FileReaderMP4::readMP4FileInChunks(FILENAME, f, receiving);
    }else if(endsWith(FILENAME,".fpv")){
        readFpvFileInChunks();
    }else{
        FileReaderRAW::readRawFileInChunks(FILENAME,f,receiving);
    }
}


void FileReader::readFpvFileInChunks() {
    std::ifstream file (FILENAME.c_str(), std::ios::in|std::ios::binary|std::ios::ate);
    if (!file.is_open()) {
        LOGD("Cannot open file %s",FILENAME.c_str());
        return;
    } else {
        LOGD("Opened File %s",FILENAME.c_str());
        file.seekg (0, std::ios::beg);
        const auto buffer=std::make_unique<std::array<uint8_t,1024*1024>>();
        while (receiving) {
            if(file.eof()){
                file.clear();
                file.seekg (0, std::ios::beg);
            }
            GroundRecorderFPV::StreamPacket header;
            file.read((char*)&header,sizeof(GroundRecorderFPV::StreamPacket));
            file.read((char*)buffer->data(),header.packet_length);
            if(header.packet_type==0){
                onDataReceivedCallback(buffer->data(),(size_t)header.packet_length);
            }
        }
        file.close();
    }
}
