//
// Created by Consti10 on 12/04/2019.
//

#ifndef TELEMETRY_GROUNDRECORDER_HPP
#define TELEMETRY_GROUNDRECORDER_HPP

#include <cstdio>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>

//Write raw data into the file specified by filename.
//The file is only created as soon as any data is written, to not pollute
//the file system with empty files

class GroundRecorderRAW{
private:
    const std::string filename;
public:
    GroundRecorderRAW(std::string s):filename(s) {}
    ~GroundRecorderRAW(){
        if(ofstream.is_open()){
            ofstream.flush();
            ofstream.close();
        }
    }
     /**
      * only as soon as we actually write data the file is created
      * to not pollute the file system with empty files
      */
    void writeData(const uint8_t *data,const size_t data_length) {
        if(data_length==0){
            return;
        }
        if(!ofstream.is_open()){
            ofstream.open (filename.c_str());
        }
        ofstream.write((char*)data,data_length);
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
