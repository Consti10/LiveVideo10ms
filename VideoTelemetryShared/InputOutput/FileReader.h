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

//Creates a new thread that 'receives' data from File and forwards data
//Via the RAW_DATA_CALLBACK. It does not specify the type of the forwarded data -
//If it is video data, the following holds true:
//RAW H264 NALUs,not specified how and when SPS/PPS come

class FileReader{
public:
    typedef std::function<void(const uint8_t[],std::size_t)> RAW_DATA_CALLBACK;
    /**
     * Does nothing until startReading is called
     * @param filename Path to file,relative to internal storage
     * @param onDataReceivedCallback callback that is called with the loaded data from the receiving thread
     * @param chunkSize Determines how big the data chunks are that are fed to the parser. The smaller the chunk size
     * The faster does the onDataReceivedCallback() return
     * Therefore allowing the file receiver to stop and exit quicker
     */
    FileReader(const std::string filename,RAW_DATA_CALLBACK onDataReceivedCallback,std::size_t chunkSize=1024):
            FILENAME(filename), CHUNK_SIZE(chunkSize),assetManager(nullptr),onDataReceivedCallback(onDataReceivedCallback){
    }
    /**
     * Same as above, but
     * @param filename is relative to the assets folder
     */
    FileReader(AAssetManager* assetManager,const std::string filename,RAW_DATA_CALLBACK onDataReceivedCallback,std::size_t chunkSize=1024):
            FILENAME(filename), CHUNK_SIZE(chunkSize),assetManager(assetManager),onDataReceivedCallback(onDataReceivedCallback){
    }
    /**
     * Create and start the receiving thread, which will run until stopReading() is called.
     */
    void startReading(){
        receiving=true;
        mThread=std::make_unique<std::thread>([this]{this->receiveLoop();} );
    }
    /**
     * After stopReading() it is guaranteed that no more data will be fed trough the callback
     */
    void stopReading(){
        receiving=false;
        if(mThread->joinable()){
            mThread->join();
        }
        mThread.reset();
    }
    int getNReceivedBytes(){
        return nReceivedB;
    }
private:
    /**
     * Pass all data divided in parts of data of size==CHUNK_SIZE
     * Returns when all data has been passed or stopReceiving is called
     */
    void passDataInChunks(const uint8_t data[],const size_t size);
    void passDataInChunks(const std::vector<uint8_t>& data);

    /**
     * Depending on the file type, either parse data or leave it untouched
     * Make sure to only call this on small files,else the application might run out of memory
     * @assetManager: valid ndk asset manager
     * @path: path to asset.  If path ends with .mp4 -> convert, else leave untouched
     */
    static std::vector<uint8_t> loadAssetFileIntoMemory(AAssetManager *assetManager, const std::string &path);

    /**
     * instead of loading whole file into memory, pass data one by one.
     * MP4 is parsed, everything else is untouched
     */
    void readFileInChunks();

    void readFpvFileInChunks();

private:
    const RAW_DATA_CALLBACK onDataReceivedCallback;
    const std::size_t CHUNK_SIZE;
    //if assetManager!=nullptr the filename is relative to the assets directory,else normal filesystem
    const std::string FILENAME;
    //Cannot make const since the functions it is called with are not marked const
    AAssetManager* assetManager;
    std::unique_ptr<std::thread> mThread;
    std::atomic<bool> receiving;
    int nReceivedB=0;
    void receiveLoop();
};

#endif //FPV_VR_FILERECEIVER_H
