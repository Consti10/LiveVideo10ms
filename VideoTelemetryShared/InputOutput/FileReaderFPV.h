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
    typedef std::function<void(const uint8_t[],const GroundRecorderFPV::StreamPacket)> MY_CALLBACK;

    static void readFpvFileInChunks(const std::string &FILENAME,MY_CALLBACK callback, const std::future<void>& shouldTerminate,const bool loopAtEOF) {
        std::ifstream file (FILENAME.c_str(), std::ios::in|std::ios::binary|std::ios::ate);
        if (!file.is_open()) {
            LOGD("Cannot open file %s", FILENAME.c_str());
            return;
        }
        //LOGD("Opened File %s",FILENAME.c_str());
        file.seekg (0, std::ios::beg);
        const auto buffer=std::make_unique<std::array<uint8_t,MAX_NALU_BUFF_SIZE>>();
        while(shouldTerminate.wait_for(std::chrono::milliseconds(0)) == std::future_status::timeout){
            GroundRecorderFPV::StreamPacket header;
            file.read((char*)&header,sizeof(GroundRecorderFPV::StreamPacket));
            std::streamsize dataSize = file.gcount();
            if(dataSize!=sizeof(GroundRecorderFPV::StreamPacket)){
                if(loopAtEOF){
                    file.clear();
                    file.seekg (0, std::ios::beg);
                    LOGD("RESET");
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
                LOGD("Error, file was written wrong");
                continue;
            }
            callback(buffer->data(),header);
        }
        file.close();
    }


    static void readFpvAssetFileInChunks(AAssetManager* assetManager,const std::string &PATH,MY_CALLBACK callback,const std::future<void>& shouldTerminate,const bool loopAtEOF) {
        AAsset *asset = AAssetManager_open(assetManager,PATH.c_str(),AASSET_MODE_BUFFER);
        if (!asset) {
            LOGD("Cannot open Asset:%s",PATH.c_str());
            return;
        }
        const auto buffer = std::make_unique<std::array<uint8_t, MAX_NALU_BUFF_SIZE>>();
        AAsset_seek(asset, 0, SEEK_SET);
        while(shouldTerminate.wait_for(std::chrono::milliseconds(0)) == std::future_status::timeout){
            GroundRecorderFPV::StreamPacket header;
            //If reading the header fails,we have reached the EOF
            const auto len = AAsset_read(asset,&header,sizeof(GroundRecorderFPV::StreamPacket));
            if(len!=sizeof(GroundRecorderFPV::StreamPacket)){
                if(loopAtEOF){
                    AAsset_seek(asset, 0, SEEK_SET);
                    LOGD("RESET");
                    continue;
                }else{
                    break;
                }
            }
            const auto len2 = AAsset_read(asset,buffer->data(),header.packet_length);
            if(len2!=header.packet_length){
                LOGD("Error, file was written wrong");
                AAsset_seek(asset, 0, SEEK_SET);
                continue;
            }
            callback(buffer->data(),header);
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
