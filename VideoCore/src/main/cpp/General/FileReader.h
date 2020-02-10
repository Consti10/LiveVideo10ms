//
// Created by Constantin on 8/8/2018.
//

#ifndef FPV_VR_FILERECEIVER_H
#define FPV_VR_FILERECEIVER_H

#include <android/asset_manager.h>
#include <vector>
#include <chrono>
#include <string>

#include <media/NdkMediaExtractor.h>
#include <thread>
#include <android/log.h>
#include <sstream>
#include <fstream>

//Creates a new thread that 'receives' data from File and forwards video data
//Via the RAW_DATA_CALLBACK

class FileReader{
public:
    //RAW H264 NALUs, not specified how and when SPS/PPS come
    typedef std::function<void(const uint8_t[],int)> RAW_DATA_CALLBACK;
    //Read from file system
    FileReader(const std::string fn,RAW_DATA_CALLBACK onDataReceivedCallback,int chunkSize=1024):
            filename(fn),CHUNK_SIZE(chunkSize),
            onDataReceivedCallback(onDataReceivedCallback){
    }
    //Read from the android asset manager
    FileReader(AAssetManager* assetManager,const std::string filename,RAW_DATA_CALLBACK onDataReceivedCallback,int chunkSize=1024):
            filename(filename),CHUNK_SIZE(chunkSize),
            onDataReceivedCallback(onDataReceivedCallback){
        this->assetManager=assetManager;
    }
    void startReading(){
        receiving=true;
        mThread=new std::thread([this] { this->receiveLoop(); });
    }
    void stopReading(){
        receiving=false;
        if(mThread->joinable()){
            mThread->join();
        }
        delete(mThread);
    }
    int getNReceivedBytes(){
        return nReceivedB;
    }
private:
    //Pass all data divided in parts of data of size==CHUNK_SIZE
    //Returns when all data has been passed or stopReceiving is called
    void passDataInChunks(const std::vector<uint8_t>& data);

    //Parse the specified asset into raw h264 video data (if needed), then return it as one big std::vector
    //Make sure to only call this on small video files,else the application might run out of memory
    //Takes files of either type .h264 -> already in raw format or
    //of type .mp4 ->use MediaExtractor to convert into raw stream
    //@assetManager: valid ndk asset manager
    //@path: path to asset
    static std::vector<uint8_t> loadAssetAsRawVideoStream(AAssetManager* assetManager,const std::string& path);

    //instead of loading whole file into memory, pass data one by one
    void parseFileAsRawVideoStream(const std::string &filename);

    //Utility for MediaFormat Handling, return buffer as std::vector that owns memory
    static std::vector<uint8_t> getBufferFromMediaFormat(const char* name,AMediaFormat* format);
    //Utility, re-sizes vector to NALU_MAXLEN, then calls AMediaExtractor_readSampleData
    //Afterwards, re-sizes vector again to sample size if sample size>0 and returns sample size
    static ssize_t AMediaExtractor_readSampleDataCPP(AMediaExtractor* extractor,std::vector<uint8_t>& data);
private:
    const RAW_DATA_CALLBACK onDataReceivedCallback;
    std::thread* mThread= nullptr;
    std::atomic<bool> receiving;
    int nReceivedB=0;
    const std::string filename;
    const int CHUNK_SIZE;
    AAssetManager* assetManager= nullptr;
    void receiveLoop();
};

#endif //FPV_VR_FILERECEIVER_H
