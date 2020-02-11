//
// Created by Constantin on 2/6/2019.
//


#include "ParseRAW.h"
#include <android/log.h>

ParseRAW::ParseRAW(NALU_DATA_CALLBACK cb):cb(cb){
}

void ParseRAW::reset(){
    nalu_data_position=4;
    nalu_search_state=0;
}

void ParseRAW::parseData(const uint8_t* data,const size_t data_length){
    std::stringstream ss;
    for (int i = 0; i < data_length; ++i) {
       /*ss<<nalu_data[nalu_data_position]<<data[i];
       if(ss.str().length()>1000){
           __android_log_print(ANDROID_LOG_DEBUG,"TAG","%s",ss.str().c_str());
           ss=std::stringstream();
       }*/
        nalu_data[nalu_data_position++] = data[i];
        if (nalu_data_position >= NALU::NALU_MAXLEN - 1) {
            nalu_data_position = 0;
        }
        switch (nalu_search_state) {
            case 0:
            case 1:
            case 2:
                if (data[i] == 0)
                    nalu_search_state++;
                else
                    nalu_search_state = 0;
                break;
            case 3:
                if (data[i] == 1) {
                    nalu_data[0] = 0;
                    nalu_data[1] = 0;
                    nalu_data[2] = 0;
                    nalu_data[3] = 1;
                    if(cb!=nullptr && nalu_data_position>=4){
                         NALU nalu(nalu_data.data(),nalu_data_position-4);
                         cb(nalu);
                    }
                    nalu_data_position = 4;
                }
                nalu_search_state = 0;
                break;
            default:
                break;
        }
    }
}