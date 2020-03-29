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
#include "FileReaderFPV.h"

static bool isTelemetryFilename(const std::string& path){
    const std::array<std::string,4> x={
            ".ltm",".mavlink",".frsky",".smartport"
    };
    for(const auto s:x){
        if(FileHelper::endsWith(path,s)){
            return true;
        }
    }
    return false;
}

void FileReader::receiveLoop() {
    nReceivedB=0;
    if(FileHelper::endsWith(FILENAME,".mp4")) {
        FileReaderMP4::readMP4FileInChunks(assetManager,FILENAME,[this](const uint8_t *data, size_t data_length) {
            passDataInChunks(data, data_length,GroundRecorderFPV::PACKET_TYPE_VIDEO_H264);
        }, receiving);
    }else if(FileHelper::endsWith(FILENAME,".fpv")){
        FileReaderFPV::readFpvFileInChunks(FILENAME, [this](const uint8_t *data, size_t data_length,GroundRecorderFPV::PACKET_TYPE packetType) {
            passDataInChunks(data, data_length,packetType);
        },receiving);
    }else if(FileHelper::endsWith(FILENAME,".h264")){
        //raw video ends with .h264
        FileReaderRAW::readRawInChunks(assetManager,FILENAME,[this](const uint8_t *data, size_t data_length) {
            passDataInChunks(data, data_length,GroundRecorderFPV::PACKET_TYPE_VIDEO_H264);
        },receiving);
    }else if(isTelemetryFilename(FILENAME)) {
        //Telemetry ends with .ltm, .mavlink usw
        GroundRecorderFPV::PACKET_TYPE packetType;
        if(FileHelper::endsWith(FILENAME,".ltm")){
            packetType=GroundRecorderFPV::PACKET_TYPE_TELEMETRY_LTM;
        }else if(FileHelper::endsWith(FILENAME,".mavlink")){
            packetType=GroundRecorderFPV::PACKET_TYPE_TELEMETRY_MAVLINK;
        }else if(FileHelper::endsWith(FILENAME,".frsky")){
            packetType=GroundRecorderFPV::PACKET_TYPE_TELEMETRY_FRSKY;
        }else if(FileHelper::endsWith(FILENAME,".smartport")){
            packetType=GroundRecorderFPV::PACKET_TYPE_TELEMETRY_SMARTPORT;
        }
        FileReaderRAW::readRawFileInChunks(FILENAME,[this,packetType](const uint8_t *data, size_t data_length) {
            passDataInChunks(data, data_length,packetType);
        },receiving);
    }else{
        LOGD("Error unknown filename %s",FILENAME.c_str());
    }
}

void FileReader::passDataInChunks(const uint8_t *data, const size_t size,
                                  GroundRecorderFPV::PACKET_TYPE packetType) {
    //We cannot use recursion due to stack pointer size limitation. -> Use loop instead.
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
            onDataReceivedCallback(&data[offset],CHUNK_SIZE,GroundRecorderFPV::PACKET_TYPE_VIDEO_H264);
            offset+=CHUNK_SIZE;
        }else{
            nReceivedB+=len_left;
            onDataReceivedCallback(&data[offset],(size_t)len_left,GroundRecorderFPV::PACKET_TYPE_VIDEO_H264);
            return;
        }
    }
}



