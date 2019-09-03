//
// Created by Constantin on 8/8/2018.
//

#ifndef FPV_VR_FILERECEIVER_H
#define FPV_VR_FILERECEIVER_H

#include <android/asset_manager.h>

class FileReader{
public:
    //Read from file system
    FileReader(const std::string fn,const int waitTimeMS,std::function<void(uint8_t[],int)> onDataReceivedCallback,int chunkSize=8):
            filename(fn),waitTimeMS(waitTimeMS),CHUNCK_SIZE(chunkSize),
            onDataReceivedCallback(onDataReceivedCallback){
    }
    //Read from the android asset manager
    FileReader(AAssetManager* assetManager,const std::string filename,const int waitTimeMS,std::function<void(uint8_t[],int)> onDataReceivedCallback,int chunkSize=8):
            filename(filename),waitTimeMS(waitTimeMS),CHUNCK_SIZE(chunkSize),
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
    int getNReceivedbytes(){
        return nReceivedB;
    }
    void passDataInChuncks(uint8_t* data,const int len){
        int offset=0;
        while(receiving){
            int len_left=len-offset;
            if(len_left>=CHUNCK_SIZE){
                nReceivedB+=CHUNCK_SIZE;
                onDataReceivedCallback(&data[offset],CHUNCK_SIZE);
                std::this_thread::sleep_for(std::chrono::nanoseconds(1000*1000*waitTimeMS));
                offset+=CHUNCK_SIZE;
            }else{
                nReceivedB+=len_left;
                onDataReceivedCallback(&data[offset],len_left);
                return;
            }
        }
    }
private:
    const std::function<void(uint8_t[],int)> onDataReceivedCallback;
    std::thread* mThread= nullptr;
    std::atomic<bool> receiving;
    int nReceivedB=0;
    AAssetManager* assetManager= nullptr;
    const std::string filename;
    const int waitTimeMS;
    const int CHUNCK_SIZE;
    void receiveLoop(){
        if(assetManager!= nullptr){
            AAsset* asset = AAssetManager_open(assetManager, filename.c_str(), 0);
            //const auto* buff=AAsset_getBuffer(asset);
            //const auto len=AAsset_getLength(asset);
            if(!asset){
                __android_log_print(ANDROID_LOG_VERBOSE, "FR","Error asset not found:%s",filename.c_str());
            }else{
                //__android_log_print(ANDROID_LOG_VERBOSE, "TR","Asset found");
                auto* buff=new uint8_t[1024];
                while(receiving){
                    int len=AAsset_read(asset,buff,1024);
                    passDataInChuncks(buff,len);
                    if(len==0){
                        AAsset_seek(asset,0,SEEK_SET);
                    }
                    //__android_log_print(ANDROID_LOG_VERBOSE, "TR","%d",len);
                }
            }
        }else{
            std::ifstream file (filename.c_str(), std::ios::in|std::ios::binary|std::ios::ate);
            nReceivedB=0;
            if (file.is_open()) {
                auto size = file.tellg();
                auto* memblock = new uint8_t [size];
                file.seekg (0, std::ios::beg);
                file.read ((char*)memblock, size);
                file.close();
                __android_log_print(ANDROID_LOG_VERBOSE, "TR","the entire file content is in memory");
                int offset=0;
                while(receiving){
                    passDataInChuncks(memblock,(int)size);
                }
                delete[] memblock;
            } else {
                __android_log_print(ANDROID_LOG_VERBOSE, "TR", "Cannot open file %s",filename.c_str());
            }
        }
    }
};

#endif //FPV_VR_FILERECEIVER_H
