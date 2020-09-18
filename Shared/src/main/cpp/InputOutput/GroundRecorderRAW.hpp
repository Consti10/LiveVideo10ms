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

private:
    std::ofstream ofstream;
};



#endif //TELEMETRY_GROUNDRECORDER_HPP
