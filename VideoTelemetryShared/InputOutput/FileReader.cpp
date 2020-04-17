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
#include <chrono>
#include "GroundRecorderRAW.hpp"
#include "GroundRecorderFPV.hpp"
#include "FileReaderMP4.hpp"
#include "FileReaderRAW.hpp"
#include "FileReaderFPV.h"

//return -1 if no valid telemetry filename, else the telemetry type
static int isTelemetryFilename(const std::string& path){
    int packetType=-1;
    if(FileHelper::endsWith(path,".ltm")){
        packetType=GroundRecorderFPV::PACKET_TYPE_TELEMETRY_LTM;
    }else if(FileHelper::endsWith(path,".mavlink")){
        packetType=GroundRecorderFPV::PACKET_TYPE_TELEMETRY_MAVLINK;
    }else if(FileHelper::endsWith(path,".frsky")){
        packetType=GroundRecorderFPV::PACKET_TYPE_TELEMETRY_FRSKY;
    }else if(FileHelper::endsWith(path,".smartport")){
        packetType=GroundRecorderFPV::PACKET_TYPE_TELEMETRY_SMARTPORT;
    }
    return packetType;
}

void FileReader::receiveLoop() {
    nReceivedB=0;
    if(FileHelper::endsWith(FILEPATH, ".mp4")) {
        FileReaderMP4::readMP4FileInChunks(assetManager,FILEPATH, [this](const uint8_t *data, size_t data_length) {
            passDataInChunks(data, data_length,GroundRecorderFPV::PACKET_TYPE_VIDEO_H264);
        }, receiving);
    }else if(FileHelper::endsWith(FILEPATH, ".fpv")){
        const auto start=std::chrono::steady_clock::now();
        FileReaderFPV::readFpvAssetOrFileInChunks(assetManager,FILEPATH, [this,start](const uint8_t *data,const GroundRecorderFPV::StreamPacket header) {
            //sync the timestamp when data was received with the timestamp from file
            auto elapsed=std::chrono::steady_clock::now()-start;
            //wait until we are at least at the time when data was received
            while(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()<header.timestamp){
                elapsed=std::chrono::steady_clock::now()-start;
            }
            passDataInChunks(data,header.packet_length,header.packet_type);
        }, receiving,false);
    }else if(FileHelper::endsWith(FILEPATH, ".h264")){
        //raw video ends with .h264
        FileReaderRAW::readRawInChunks(assetManager, FILEPATH, [this](const uint8_t *data, size_t data_length) {
            passDataInChunks(data, data_length,GroundRecorderFPV::PACKET_TYPE_VIDEO_H264);
        }, receiving);
    }else if(isTelemetryFilename(FILEPATH) != -1) {
        //Telemetry ends with .ltm, .mavlink usw
        const GroundRecorderFPV::PACKET_TYPE packetType=(GroundRecorderFPV::PACKET_TYPE)isTelemetryFilename(FILEPATH);
        FileReaderRAW::readRawInChunks(assetManager,FILEPATH, [this,packetType](const uint8_t *data, size_t data_length) {
            passDataInChunks(data, data_length,packetType);
        }, receiving);
    }else{
        LOGD("Error unknown filename %s", FILEPATH.c_str());
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
            for(const RAW_DATA_CALLBACK& cb: onDataReceivedCallbacks){
                if(cb!= nullptr){
                    cb(&data[offset],CHUNK_SIZE,packetType);
                }
            }
            offset+=CHUNK_SIZE;
        }else{
            nReceivedB+=len_left;
            for(const RAW_DATA_CALLBACK& cb: onDataReceivedCallbacks){
                if(cb!= nullptr){
                    cb(&data[offset],(size_t)len_left,packetType);
                }
            }
            return;
        }
    }
}



