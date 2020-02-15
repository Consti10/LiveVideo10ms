//
// Created by Consti10 on 12/04/2019.
//

#ifndef TELEMETRY_GROUNDRECORDER_HPP
#define TELEMETRY_GROUNDRECORDER_HPP

#include "../../../../../../../../AppData/Local/Android/Sdk/ndk/21.0.6113669/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/include/c++/v1/cstdio"
#include "../../../../../../../../AppData/Local/Android/Sdk/ndk/21.0.6113669/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/include/c++/v1/string"
#include "../../../../../../../../AppData/Local/Android/Sdk/ndk/21.0.6113669/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/include/c++/v1/fstream"
#include "../../../../../../../../AppData/Local/Android/Sdk/ndk/21.0.6113669/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/include/c++/v1/sstream"
#include "../../../../../../../../AppData/Local/Android/Sdk/ndk/21.0.6113669/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/include/c++/v1/filesystem"


class GroundRecorder{
private:
    const std::string filename;
public:
    GroundRecorder(std::string s):filename(s) {
        //ofstream.open (filename.c_str());
    }

//only as soon as we actually write data the file is created
//to not pollute the file system with empty files
    void writeData(const uint8_t *data,const size_t data_length) {
        if(data_length==0){
            return;
        }
        if(!ofstream.is_open()){
            ofstream.open (filename.c_str());
        }
        ofstream.write((char*)data,data_length);
    }
    void stop() {
        ofstream.flush();
        ofstream.close();
    }
    static std::string findUnusedFilename(std::string directory,std::string filetype) {
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        std::stringstream filenameShort;
        //first,try with date,hours and minutes only
        filenameShort<<directory<<std::put_time(&tm, "%d-%m-%Y %H-%M")<<"."<<filetype;
        std::ifstream infile(filenameShort.str());
        if(!infile.good()){
            return filenameShort.str();
        }
        //else, also use seconds and assume this one is valid
        std::stringstream filenameLong;
        filenameLong<<directory<<std::put_time(&tm, "%d-%m-%Y %H-%M-%S")<<"."<<filetype;
        return filenameLong.str();
    }
private:
    std::ofstream ofstream;
};



#endif //TELEMETRY_GROUNDRECORDER_HPP
