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
    static constexpr const size_t MAX_NALU_BUFF_SIZE = 1024 * 1024;
    typedef std::function<void(const uint8_t[], unsigned int,GroundRecorderFPV::PACKET_TYPE)> MY_CALLBACK;

    static void readFpvFileInChunks(const std::string &FILENAME,MY_CALLBACK callback,std::atomic<bool>& receiving) {
        std::ifstream file (FILENAME.c_str(), std::ios::in|std::ios::binary|std::ios::ate);
        if (!file.is_open()) {
            LOGD("Cannot open file %s", FILENAME.c_str());
            return;
        }
        //LOGD("Opened File %s",FILENAME.c_str());
        file.seekg (0, std::ios::beg);
        auto start=std::chrono::steady_clock::now();
        const auto buffer=std::make_unique<std::array<uint8_t,MAX_NALU_BUFF_SIZE>>();
        while (receiving) {
            GroundRecorderFPV::StreamPacket header;
            file.read((char*)&header,sizeof(GroundRecorderFPV::StreamPacket));
            std::streamsize dataSize = file.gcount();
            if(dataSize!=sizeof(GroundRecorderFPV::StreamPacket)){
                file.clear();
                file.seekg (0, std::ios::beg);
                LOGD("RESET");
                continue;
            }
            file.read((char*)buffer->data(),header.packet_length);
            const int bytesRead=file.gcount();
            if(bytesRead!=header.packet_length){
                file.clear();
                file.seekg (0, std::ios::beg);
                LOGD("Error, file was written wrong");
                continue;
            }
            if(header.packet_type==0){
                auto elapsed=std::chrono::steady_clock::now()-start;
                //wait until we are at least at the time when data was received
                while(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()<header.timestamp){
                    elapsed=std::chrono::steady_clock::now()-start;
                }
            }
            callback(buffer->data(),header.packet_length,header.packet_type);
        }
        file.close();
    }


    static void readFpvAssetFileInChunks(AAssetManager* assetManager,const std::string &PATH,MY_CALLBACK callback,std::atomic<bool>& receiving) {
        AAsset *asset = AAssetManager_open(assetManager,PATH.c_str(),AASSET_MODE_BUFFER);
        if (!asset) {
            LOGD("Cannot open Asset:%s",PATH.c_str());
            return;
        }
        const auto buffer = std::make_unique<std::array<uint8_t, MAX_NALU_BUFF_SIZE>>();
        AAsset_seek(asset, 0, SEEK_SET);
        auto start=std::chrono::steady_clock::now();
        while(receiving) {
            GroundRecorderFPV::StreamPacket header;
            //If reading the header fails,we have reached the EOF
            const auto len = AAsset_read(asset,&header,sizeof(GroundRecorderFPV::StreamPacket));
            if(len!=sizeof(GroundRecorderFPV::StreamPacket)){
                AAsset_seek(asset, 0, SEEK_SET);
                LOGD("RESET");
                continue;
            }
            const auto len2 = AAsset_read(asset,buffer->data(),header.packet_length);
            if(len2!=header.packet_length){
                LOGD("Error, file was written wrong");
                AAsset_seek(asset, 0, SEEK_SET);
                continue;
            }
            if(header.packet_type==0){
                auto elapsed=std::chrono::steady_clock::now()-start;
                //wait until we are at least at the time when data was received
                while(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()<header.timestamp){
                    elapsed=std::chrono::steady_clock::now()-start;
                }
            }
            callback(buffer->data(),header.packet_length,header.packet_type);
        }
        AAsset_close(asset);
    }

    static void readFpvAssetOrFileInChunks(AAssetManager* assetManager,const std::string &PATH,MY_CALLBACK callback,std::atomic<bool>& receiving){
        if(assetManager==nullptr){
            readFpvFileInChunks(PATH,callback,receiving);
        }else{
            readFpvAssetFileInChunks(assetManager,PATH,callback,receiving);
        }
    }
}
#endif //LIVEVIDEO10MS_FILEREADERFPV_H
