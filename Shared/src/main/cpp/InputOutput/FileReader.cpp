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
#include <TimeHelper.hpp>
#include "GroundRecorderRAW.hpp"
#include "GroundRecorderFPV.hpp"
#include "FileReaderMP4.hpp"
#include "FileReaderRAW.hpp"
#include "FileReaderFPV.h"
#include <NDKThreadHelper.hpp>

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

void FileReader::startReading(AAssetManager *assetManager1,std::string FILEPATH1) {
    std::lock_guard<std::mutex> lock(mMutexStartStop);
    if(started)
        return;
    MLOGD<<"Receive from file "<<FILEPATH1;
    this->assetManager=assetManager1;
    this->FILEPATH=std::move(FILEPATH1);
    started=true;
    exitSignal=std::promise<void>();
    std::future<void> futureObj = exitSignal.get_future();
    mThread=std::make_unique<std::thread>(&FileReader::receiveLoop,this,std::move(futureObj));
    NDKThreadHelper::setName(mThread->native_handle(),"FileReader");
}

void FileReader::stopReadingIfStarted() {
    std::lock_guard<std::mutex> lock(mMutexStartStop);
    if(!started){
        return;
    }
    exitSignal.set_value();
    if(mThread->joinable()){
        mThread->join();
    }
    mThread.reset();
    started=false;
    nReceivedB=0;
}

void FileReader::receiveLoop(std::future<void> shouldTerminate) {
    nReceivedB=0;
    if(FileHelper::endsWith(FILEPATH, ".mp4")) {
        FileReaderMP4::readMP4FileInChunks(assetManager,FILEPATH, [this,&shouldTerminate](const uint8_t *data, size_t data_length) {
            splitDataInChunks(shouldTerminate,data, data_length,GroundRecorderFPV::PACKET_TYPE_VIDEO_H264);
        },shouldTerminate);
    }else if(FileHelper::endsWith(FILEPATH, ".fpv")){
        auto start=std::chrono::steady_clock::now();
        const bool loopAtEOF=assetManager!=nullptr;
        std::chrono::milliseconds lastPacketTimestamp(0);
        FileReaderFPV::readFpvAssetOrFileInChunks(assetManager,FILEPATH, [this,&start,&shouldTerminate,&lastPacketTimestamp](const FileReaderFPV::GroundRecordingPacket& packet) {
            // the timestamp is guaranteed to be strictly increasing. If it is not, this probably means we reached
            // EOF and are looping. Reset start time in this case
            if(packet.timestamp<lastPacketTimestamp){
                start=std::chrono::steady_clock::now();
            }
            //sync the timestamp when data was received with the timestamp from file
            auto elapsed=std::chrono::steady_clock::now()-start;
            lastPacketTimestamp=packet.timestamp;
            //wait until we are at least at the time when data was received
            while(elapsed<packet.timestamp*0.8){
                elapsed=std::chrono::steady_clock::now()-start;
                TestSleep::sleep(std::chrono::milliseconds(1));
            }
            //const auto timeToWaitUs=(long)header.timestamp-std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
            //if(timeToWaitNS)
            splitDataInChunks(shouldTerminate,packet.data,packet.data_length,packet.packet_type);
        },shouldTerminate,loopAtEOF);
    }else if(FileHelper::endsWith(FILEPATH, ".h264")){
        //raw video ends with .h264
        FileReaderRAW::readRawInChunks(assetManager, FILEPATH, [this,&shouldTerminate](const uint8_t *data, size_t data_length) {
            //MLOGD<<"Raw chunck h264";
            splitDataInChunks(shouldTerminate,data, data_length,GroundRecorderFPV::PACKET_TYPE_VIDEO_H264);
        },shouldTerminate);
    }else if(FileHelper::endsWith(FILEPATH, ".h265")){
        //raw video ends with .h264
        FileReaderRAW::readRawInChunks(assetManager, FILEPATH, [this,&shouldTerminate](const uint8_t *data, size_t data_length) {
           // MLOGD<<"Raw chunck h265";
            splitDataInChunks(shouldTerminate,data, data_length,GroundRecorderFPV::PACKET_TYPE_VIDEO_H265);
        },shouldTerminate);
    }else if(isTelemetryFilename(FILEPATH) != -1) {
        //Telemetry ends with .ltm, .mavlink usw
        const auto packetType=(GroundRecorderFPV::PACKET_TYPE)isTelemetryFilename(FILEPATH);
        FileReaderRAW::readRawInChunks(assetManager,FILEPATH, [this,packetType,&shouldTerminate](const uint8_t *data, size_t data_length) {
            splitDataInChunks(shouldTerminate,data, data_length,packetType);
        },shouldTerminate);
    }else{
        MLOGE<<"Unknown filename"<<FILEPATH;
    }
}

void FileReader::splitDataInChunks(const std::future<void>& shouldTerminate,const uint8_t *data, const size_t size,GroundRecorderFPV::PACKET_TYPE packetType) {
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
    while(shouldTerminate.wait_for(std::chrono::milliseconds(0)) == std::future_status::timeout){
        const ssize_t len_left=len-offset;
        if(len_left>=CHUNK_SIZE){
            passChunk(&data[offset],CHUNK_SIZE,packetType);
            offset+=CHUNK_SIZE;
        }else{
            passChunk(&data[offset],(size_t)len_left,packetType);
            return;
        }
    }
}

void FileReader::passChunk(const uint8_t data[],const size_t size,GroundRecorderFPV::PACKET_TYPE packetType) {
    nReceivedB+=size;
    for(const RAW_DATA_CALLBACK& cb: onDataReceivedCallbacks){
        if(cb!= nullptr){
            cb(data,size,packetType);
        }
    }
}


/*nPackets++;
LOGD("N packets %d",nPackets);
if(nPackets<5200 || nPackets>6000){
   //return;
}else{
   splitDataInChunks(shouldTerminate,data,header.packet_length,header.packet_type);
}*/

