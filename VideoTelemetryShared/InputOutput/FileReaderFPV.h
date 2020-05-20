//
// Created by geier on 28/03/2020.
//

#ifndef LIVEVIDEO10MS_FILEREADERFPV_H
#define LIVEVIDEO10MS_FILEREADERFPV_H

#include <android/asset_manager.h>
#include <vector>
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
    // This one is passed around when reading the data.
    typedef struct{
        GroundRecorderFPV::PACKET_TYPE packet_type;
        std::chrono::milliseconds timestamp;
        uint8_t* data;
        size_t data_length;
    }GroundRecordingPacket;
    typedef std::function<void(const GroundRecordingPacket&)> MY_CALLBACK;

    static void readFpvFileInChunks(const std::string &FILENAME,MY_CALLBACK callback, const std::future<void>& shouldTerminate,const bool loopAtEOF) {
        std::ifstream file (FILENAME.c_str(), std::ios::in|std::ios::binary|std::ios::ate);
        if (!file.is_open()) {
            MLOGE<<"Cannot open file "<<FILENAME;
            return;
        }
        //LOG::D("Opened File %s",FILENAME.c_str());
        file.seekg (0, std::ios::beg);
        const auto buffer=std::make_unique<std::array<uint8_t,MAX_NALU_BUFF_SIZE>>();
        while(shouldTerminate.wait_for(std::chrono::milliseconds(0)) == std::future_status::timeout){
            GroundRecorderFPV::StreamPacketHeader header;
            file.read((char*)&header,sizeof(GroundRecorderFPV::StreamPacketHeader));
            std::streamsize dataSize = file.gcount();
            if(dataSize!=sizeof(GroundRecorderFPV::StreamPacketHeader)){
                if(loopAtEOF){
                    file.clear();
                    file.seekg (0, std::ios::beg);
                    MLOGD<<"RESET";
                    continue;
                }else{
                    break;
                }
            }
            file.read((char*)buffer->data(),header.packet_length);
            const int bytesRead=file.gcount();
            if(bytesRead!=header.packet_length){
                file.clear();
                file.seekg (0, std::ios::beg);
                MLOGE<<"File was written wrong";
                continue;
            }
            GroundRecordingPacket groundRecordingPacket{header.packet_type,std::chrono::milliseconds(header.timestamp),buffer->data(),header.packet_length};
            callback(groundRecordingPacket);
        }
        file.close();
    }


    static void readFpvAssetFileInChunks(AAssetManager* assetManager,const std::string &PATH,MY_CALLBACK callback,const std::future<void>& shouldTerminate,const bool loopAtEOF) {
        AAsset *asset = AAssetManager_open(assetManager,PATH.c_str(),AASSET_MODE_BUFFER);
        if (!asset) {
            MLOGE<<"Cannot open Asset: "<<PATH;
            return;
        }
        const auto buffer = std::make_unique<std::array<uint8_t, MAX_NALU_BUFF_SIZE>>();
        AAsset_seek(asset, 0, SEEK_SET);
        while(shouldTerminate.wait_for(std::chrono::milliseconds(0)) == std::future_status::timeout){
            GroundRecorderFPV::StreamPacketHeader header;
            //If reading the header fails,we have reached the EOF
            const auto len = AAsset_read(asset,&header,sizeof(GroundRecorderFPV::StreamPacketHeader));
            if(len!=sizeof(GroundRecorderFPV::StreamPacketHeader)){
                if(loopAtEOF){
                    AAsset_seek(asset, 0, SEEK_SET);
                    MLOGD<<"RESET";
                    continue;
                }else{
                    break;
                }
            }
            const auto len2 = AAsset_read(asset,buffer->data(),header.packet_length);
            if(len2!=header.packet_length){
                MLOGE<<"file was written wrong";
                AAsset_seek(asset, 0, SEEK_SET);
                continue;
            }
            GroundRecordingPacket groundRecordingPacket{header.packet_type,std::chrono::milliseconds(header.timestamp),buffer->data(),header.packet_length};
            callback(groundRecordingPacket);
        }
        AAsset_close(asset);
    }

    static void readFpvAssetOrFileInChunks(AAssetManager* assetManager,const std::string &PATH,MY_CALLBACK callback,const std::future<void>& shouldTerminate,const bool loopAtEOF){
        if(assetManager==nullptr){
            readFpvFileInChunks(PATH,callback,shouldTerminate,loopAtEOF);
        }else{
            readFpvAssetFileInChunks(assetManager,PATH,callback,shouldTerminate,loopAtEOF);
        }
    }
}
#endif //LIVEVIDEO10MS_FILEREADERFPV_H
