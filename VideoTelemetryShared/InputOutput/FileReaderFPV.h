//
// Created by geier on 28/03/2020.
//

#ifndef LIVEVIDEO10MS_FILEREADERFPV_H
#define LIVEVIDEO10MS_FILEREADERFPV_H

#include <android/asset_manager.h>
#include <vector>
#include <chrono>
#include <string>
#include <thread>
#include <android/log.h>
#include <sstream>
#include <fstream>
#include <iterator>
#include <array>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <memory>
#include "GroundRecorderFPV.hpp"

namespace FileReaderFPV{
    typedef std::function<void(const uint8_t[], unsigned int,GroundRecorderFPV::PACKET_TYPE)> MY_CALLBACK;

    static void readFpvFileInChunks(const std::string &FILENAME,MY_CALLBACK callback,std::atomic<bool>& receiving) {
        std::ifstream file (FILENAME.c_str(), std::ios::in|std::ios::binary|std::ios::ate);
        if (!file.is_open()) {
            LOGD("Cannot open file %s",FILENAME.c_str());
            return;
        } else {
            LOGD("Opened File %s",FILENAME.c_str());
            file.seekg (0, std::ios::beg);
            auto start=std::chrono::steady_clock::now();
            const auto buffer=std::make_unique<std::array<uint8_t,1024*1024>>();
            while (receiving) {
                if(file.eof()){
                    file.clear();
                    file.seekg (0, std::ios::beg);
                    LOGD("RESET");
                }
                GroundRecorderFPV::StreamPacket header;
                file.read((char*)&header,sizeof(GroundRecorderFPV::StreamPacket));
                file.read((char*)buffer->data(),header.packet_length);
                if(header.packet_type==0){
                    auto elapsed=std::chrono::steady_clock::now()-start;
                    //wait until we are at least at the time when data was received
                    while(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()<header.timestamp){
                        elapsed=std::chrono::steady_clock::now()-start;
                    }
                    callback(buffer->data(),(size_t)header.packet_length,header.packet_type);
                }
            }
            file.close();
        }
    }
}
#endif //LIVEVIDEO10MS_FILEREADERFPV_H
