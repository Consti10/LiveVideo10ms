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
#include <chrono>

/**
 * Thread-safe class for writing a .fpv ground recording file
 */
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
            fileCreationTime=std::chrono::steady_clock::now();
        }
    }
    void closeFileIfOpened(){
        if(ofstream.is_open()){
            ofstream.flush();
            ofstream.close();
        }
    }
    std::chrono::steady_clock::time_point fileCreationTime;
public:
    static constexpr uint8_t PACKET_TYPE_VIDEO_H264=0;
    static constexpr uint8_t PACKET_TYPE_TELEMETRY_LTM=1;
    static constexpr uint8_t PACKET_TYPE_TELEMETRY_MAVLINK=2;
    static constexpr uint8_t PACKET_TYPE_TELEMETRY_SMARTPORT=3;
    static constexpr uint8_t PACKET_TYPE_TELEMETRY_FRSKY=4;
    static constexpr uint8_t PACKET_TYPE_TELEMETRY_EZWB=5;
    static constexpr uint8_t PACKET_TYPE_TELEMETRY_ANDROD_GPS=6;
    static constexpr uint8_t PACKET_TYPE_MJPEG_ROTG02=7;
    using PACKET_TYPE=uint8_t;
    using TIMESTAMP_MS=unsigned int;
    // Each time I write raw data to the file it is prefixed by this header
    // giving info about how to interpret the raw data and its size
    typedef struct{
        unsigned int packet_length;
        PACKET_TYPE packet_type;
        // This value is in ms and always strictly increasing
        TIMESTAMP_MS timestamp;
        uint8_t placeholder[8];//8 bytes as placeholder for future use
    }__attribute__((packed)) StreamPacketHeader;
public:
    GroundRecorderFPV(std::string s):DIRECTORY(s) {}
    //It is okay to call start() multiple times
    //only in the 'started' state data is written to the ground recording file
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
    void writePacketIfStarted(const uint8_t *packet,const size_t packet_length,const PACKET_TYPE packet_type,int customTimeStamp=-1) {
        std::lock_guard<std::mutex> lock(mMutexFileAccess);
        if(!started)return;
        if(packet_length==0)return;
        createOpenFileIfNeeded();
        //calculate the timestamp
        auto now=std::chrono::steady_clock::now();
        const TIMESTAMP_MS timestamp=customTimeStamp!=-1 ? customTimeStamp :
                (TIMESTAMP_MS)std::chrono::duration_cast<std::chrono::milliseconds>(now - fileCreationTime).count();
        StreamPacketHeader streamPacket;
        streamPacket.packet_length=(unsigned int)packet_length;
        streamPacket.packet_type=packet_type;
        streamPacket.timestamp=timestamp;
        ofstream.write((char*)&streamPacket,sizeof(StreamPacketHeader));
        ofstream.write((char*)packet,packet_length);
    }
private:
    std::ofstream ofstream;
};

#endif //TELEMETRY_GROUNDRECORDERFPV_HPP
