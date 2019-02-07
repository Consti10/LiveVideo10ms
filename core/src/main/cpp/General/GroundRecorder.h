//
// Created by Constantin on 12.01.2018.
//

#ifndef FPV_VR_GROUNDRECORDER_H
#define FPV_VR_GROUNDRECORDER_H

#include <cstdio>
#include <string>
#include <fstream>

class GroundRecorder {
public:
    explicit GroundRecorder(std::string s);
    void writeData(const uint8_t* data,const int data_length);
    void stop();
private:
    std::ofstream ofstream;
};


#endif //FPV_VR_GROUNDRECORDER_H
