//
// Created by Constantin on 8/8/2018.
//

#ifndef FPV_VR_FILERECEIVER_H
#define FPV_VR_FILERECEIVER_H

#include <android/asset_manager.h>
#include <vector>
#include <chrono>

class FileReader{
public:
    typedef std::function<void(const uint8_t[],int)> RAW_DATA_CALLBACK;
    //Read from file system
    FileReader(const std::string fn,const int waitTimeMS,RAW_DATA_CALLBACK onDataReceivedCallback,int chunkSize=1024):
            filename(fn),waitTimeMS(waitTimeMS),CHUNK_SIZE(chunkSize),
            onDataReceivedCallback(onDataReceivedCallback){
    }
    //Read from the android asset manager
    FileReader(AAssetManager* assetManager,const std::string filename,const int waitTimeMS,RAW_DATA_CALLBACK onDataReceivedCallback,int chunkSize=1024):
            filename(filename),waitTimeMS(waitTimeMS),CHUNK_SIZE(chunkSize),
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
    //Pass all data divided in parts of data of size==CHUNK_SIZE
    //Returns when all data has been passed or stopReceiving is called
    void passDataInChunks(const std::vector<uint8_t>& data){
        int offset=0;
        const int len=(int)data.size();
        while(receiving){
            const int len_left=len-offset;
            if(len_left>=CHUNK_SIZE){
                nReceivedB+=CHUNK_SIZE;
                onDataReceivedCallback(&data[offset],CHUNK_SIZE);
                //std::this_thread::sleep_for(std::chrono::nanoseconds(1000*1000*waitTimeMS));
                offset+=CHUNK_SIZE;
            }else{
                nReceivedB+=len_left;
                onDataReceivedCallback(&data[offset],len_left);
                return;
            }
        }
    }
private:
    const RAW_DATA_CALLBACK onDataReceivedCallback;
    std::thread* mThread= nullptr;
    std::atomic<bool> receiving;
    int nReceivedB=0;
    AAssetManager* assetManager= nullptr;
    const std::string filename;
    const int waitTimeMS;
    const int CHUNK_SIZE;
    static const constexpr auto TAG="FileReader";
    void receiveLoop(){
        nReceivedB=0;
        if(assetManager!= nullptr){
            AAsset* asset = AAssetManager_open(assetManager, filename.c_str(), 0);
            if(!asset){
                __android_log_print(ANDROID_LOG_VERBOSE, "FR","Error asset not found:%s",filename.c_str());
            }else{
                //__android_log_print(ANDROID_LOG_VERBOSE, "TR","Asset found");
                /*std::vector<uint8_t> buff(1024);
                while(receiving){
                    int len=AAsset_read(asset,buff.data(),1024);
                    passDataInChuncks(buff);
                    if(len==0){
                        AAsset_seek(asset,0,SEEK_SET);
                    }
                }*/
                const size_t size=(size_t)AAsset_getLength(asset);
                AAsset_seek(asset,0,SEEK_SET);
                std::vector<uint8_t> buff(size);
                int len=AAsset_read(asset,buff.data(),size);
                AAsset_close(asset);
                __android_log_print(ANDROID_LOG_VERBOSE,TAG,"The entire file content (asset) is in memory %d",(int)size);
                while(receiving){
                    passDataInChunks(buff);
                }
                __android_log_print(ANDROID_LOG_VERBOSE,TAG,"exit");
            }
        }else{
            std::ifstream file (filename.c_str(), std::ios::in|std::ios::binary|std::ios::ate);
            if (!file.is_open()) {
                __android_log_print(ANDROID_LOG_VERBOSE,TAG, "Cannot open file %s",filename.c_str());
            } else {
                const auto size = file.tellg();
                std::vector<uint8_t> buff(size);
                file.seekg (0, std::ios::beg);
                file.read ((char*)buff.data(), size);
                file.close();
                __android_log_print(ANDROID_LOG_VERBOSE,TAG,"the entire file content is in memory");
                while(receiving){
                    passDataInChunks(buff);
                }
            }
        }
    }
};

#endif //FPV_VR_FILERECEIVER_H
