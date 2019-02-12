//
// Created by Constantin on 12.01.2018.
//

#include "GroundRecorder.h"

GroundRecorder::GroundRecorder(std::string s) {
    ofstream.open (s.c_str());
}

void GroundRecorder::writeData(const uint8_t *data,const int data_length) {
    ofstream.write((char*)data,data_length);
}

void GroundRecorder::stop() {
    ofstream.flush();
    ofstream.close();
}
