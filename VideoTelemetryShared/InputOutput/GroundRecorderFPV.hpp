//
// Created by geier on 20/03/2020.
//

#ifndef TELEMETRY_GROUNDRECORDERFPV_HPP
#define TELEMETRY_GROUNDRECORDERFPV_HPP

#include <cstdio>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include "FileHelper.hpp"

class GroundRecorderFPV{
private:
    const std::string DIRECTORY;
    //Make sure only one thread writes to file at the time
    std::mutex mMutexFileAccess;
    bool started=false;
    /**
    * only as soon as we actually write data the file is created
    * to not pollute the file system with empty files
    */
    void createOpenFileIfNeeded(){
        if(!ofstream.is_open()){
            const std::string groundRecordingFlename=FileHelper::findUnusedFilename(DIRECTORY,"fpv");
            ofstream.open (groundRecordingFlename.c_str());
        }
    }
    void closeFileIfOpened(){
        if(ofstream.is_open()){
            ofstream.flush();
            ofstream.close();
        }
    }
public:
    static constexpr uint8_t PACKET_TYPE_H264=0;
    static constexpr uint8_t PACKET_TYPE_TELEMETRY_LTM=1;
    static constexpr uint8_t PACKET_TYPE_TELEMETRY_MAVLINK=2;
    static constexpr uint8_t PACKET_TYPE_TELEMETRY_SMARTPORT=3;
    static constexpr uint8_t PACKET_TYPE_TELEMETRY_FRSKY=4;
    static constexpr uint8_t PACKET_TYPE_TELEMETRY_EZWB=5;
    using PACKET_TYPE=uint8_t;
    using TIMESTAMP=unsigned int;
    typedef struct{
        unsigned int packet_length;
        PACKET_TYPE packet_type;
        TIMESTAMP timestamp;
    }__attribute__((packed)) StreamPacket;
public:
    GroundRecorderFPV(std::string s):DIRECTORY(s) {}
    void start(){
        std::lock_guard<std::mutex> lock(mMutexFileAccess);
        started=true;
    }
    void stop(){
        std::lock_guard<std::mutex> lock(mMutexFileAccess);
        started=false;
        closeFileIfOpened();
    }
    //Only write data if started and data_length>0
    void writePacketIfStarted(const uint8_t *packet,const size_t packet_length,const PACKET_TYPE packet_type) {
        std::lock_guard<std::mutex> lock(mMutexFileAccess);
        if(!started)return;
        if(packet_length==0)return;
        createOpenFileIfNeeded();
        StreamPacket streamPacket{(unsigned int)packet_length,0};
        streamPacket.packet_type=packet_type;
        ofstream.write((char*)&streamPacket,sizeof(StreamPacket));
        ofstream.write((char*)packet,packet_length);
    }
private:
    std::ofstream ofstream;
};

#endif //TELEMETRY_GROUNDRECORDERFPV_HPP
