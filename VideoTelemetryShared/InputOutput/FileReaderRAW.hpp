//
// Created by geier on 20/03/2020.
//

#ifndef TELEMETRY_FILEREADERRAW_HPP
#define TELEMETRY_FILEREADERRAW_HPP

#include <android/asset_manager.h>
#include <vector>
#include <chrono>
#include <string>

#include <media/NdkMediaExtractor.h>
#include <thread>
#include <android/log.h>
#include <sstream>
#include <fstream>
#include <MDebug.hpp>

/**
 * Namespace with utility functions for reading RAW files
 */
namespace FileReaderRAW {
    static constexpr const size_t MAX_NALU_BUFF_SIZE = 1024 * 1024;
    typedef std::function<void(const uint8_t[], std::size_t)> RAW_DATA_CALLBACK;

    /**
     * Open raw file and pass its data one by one in Chunks until @param receiving==false
     */
    static void readRawFileInChunks(const std::string &FILENAME,const RAW_DATA_CALLBACK callback,
                                    std::atomic<bool> &receiving) {
        std::ifstream file(FILENAME.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            LOGD("Cannot open file %s", FILENAME.c_str());
            return;
        }
        //LOGD("Opened File %s", FILENAME.c_str());
        file.seekg(0, std::ios::beg);
        const auto buffer = std::make_unique<std::array<uint8_t, MAX_NALU_BUFF_SIZE>>();
        while (receiving) {
            if (file.eof()) {
                file.clear();
                file.seekg(0, std::ios::beg);
            }
            file.read((char *) buffer->data(), buffer->size());
            std::streamsize dataSize = file.gcount();
            if (dataSize > 0) {
                callback(buffer->data(), (size_t) dataSize);
            }
        }
        file.close();
    }
    /**
     * Same as above, but for asset file
     */
    static void readRawAssetInChunks(AAssetManager *assetManager, const std::string &PATH,const RAW_DATA_CALLBACK callback,
                                     std::atomic<bool> &receiving,const bool loopAtEndOfFile){
        AAsset *asset = AAssetManager_open(assetManager,PATH.c_str(),AASSET_MODE_BUFFER);
        if (!asset) {
            LOGD("Cannot open Asset:%s",PATH.c_str());
            return;
        }
        const auto buffer = std::make_unique<std::array<uint8_t, MAX_NALU_BUFF_SIZE>>();
        AAsset_seek(asset, 0, SEEK_SET);
        while(receiving){
            const auto len = AAsset_read(asset,buffer->data(),MAX_NALU_BUFF_SIZE);
            if(len>0){
                callback(buffer.get()->data(),(size_t)len);
            }else{
                if(loopAtEndOfFile){
                    AAsset_seek(asset, 0, SEEK_SET);
                }else{
                    break;
                }
            }
        }
        AAsset_close(asset);
    }

    static std::vector<uint8_t>
    loadRawAssetFileIntoMemory(AAssetManager *assetManager, const std::string &path) {
        AAsset *asset = AAssetManager_open(assetManager, path.c_str(), 0);
        if (!asset) {
            LOGD("Error asset not found:%s", path.c_str());
            return std::vector<uint8_t>();
        }
        const size_t size = (size_t) AAsset_getLength(asset);
        AAsset_seek(asset, 0, SEEK_SET);
        std::vector<uint8_t> rawData(size);
        const auto len = AAsset_read(asset, rawData.data(), size);
        AAsset_close(asset);
        LOGD("The entire file content (asset,raw) is in memory %d", (int) rawData.size());
        return rawData;
    }

    /**
     * Load the Asset file specified by path into memory and return as one big data buffer
     * The data is neither parsed nor modified any other way
    */
    static std::vector<uint8_t>
    loadRawAssetFileIntoMemory2(AAssetManager *assetManager, const std::string &path) {
        std::atomic<bool> fill=true;
        std::vector<uint8_t> rawData;
        rawData.reserve(1024*1024);
        readRawAssetInChunks(assetManager,path, [&rawData](const uint8_t* d,int len) {
            const auto offset=rawData.size();
            rawData.resize(rawData.size()+len);
            memcpy(&rawData.at(offset),d,(size_t)len);
        },fill,false);
        LOGD("The entire file content (asset,raw) is in memory %d", (int) rawData.size());
        return rawData;
    }
}

#endif //TELEMETRY_FILEREADERRAW_HPP
