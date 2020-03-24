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

class GroundRecorderFPV{
private:
    const std::string filename;
    std::mutex mMutexFileAccess;
    /**
    * only as soon as we actually write data the file is created
    * to not pollute the file system with empty files
    */
    void createOpenFileIfNeeded(){
        if(!ofstream.is_open()){

            ofstream.open (filename.c_str());
        }
    }
public:
    static constexpr uint8_t PACKET_TYPE_H264=0;
    static constexpr uint8_t PACKET_TYPE_TELEMETRY_LTM=1;
    static constexpr uint8_t PACKET_TYPE_TELEMETRY_MAVLINK=2;
    static constexpr uint8_t PACKET_TYPE_TELEMETRY_SMARTPORT=3;
    static constexpr uint8_t PACKET_TYPE_TELEMETRY_FRSKY=4;
    static constexpr uint8_t PACKET_TYPE_TELEMETRY_EZWB=5;
    typedef struct{
        unsigned int packet_length;
        uint8_t packet_type;
        unsigned int timestamp;
    }__attribute__((packed)) StreamPacket;
public:
    GroundRecorderFPV(std::string s):filename(s) {}
    ~GroundRecorderFPV(){
        if(ofstream.is_open()){
            ofstream.flush();
            ofstream.close();
        }
    }
    void writePacket(const uint8_t *packet,const size_t packet_length,const uint8_t packet_type) {
        //Make sure only one thread writes to file at the time
        std::lock_guard<std::mutex> lock(mMutexFileAccess);
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
