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
    dji_data_buff_size=0;
    //nalu_data.reserve(NALU::NALU_MAXLEN);
}

void ParseRAW::parseData(const uint8_t* data,const size_t data_length){
    //if(nalu_data== nullptr){
    //    nalu_data=new uint8_t[NALU::NALU_MAXLEN];
    //}
    for (size_t i = 0; i < data_length; ++i) {
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
                         //nalu_data.resize(nalu_data_position-4);
                         //NALU nalu(nalu_data);
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

void ParseRAW::parseDjiLiveVideoData(const uint8_t* data,const size_t data_length){
    for (size_t i = 0; i < data_length; ++i) {
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
                        if(nalu.isSPS() || nalu.isPPS()){
                            cb(nalu);
                            dji_data_buff_size=0;
                        }else if(nalu.get_nal_unit_type()==NAL_UNIT_TYPE_AUD){
                            if(dji_data_buff_size>0){
                                NALU nalu2(dji_data_buff.data(),dji_data_buff_size);
                                cb(nalu2);
                                dji_data_buff_size=0;
                            }
                        }else if(nalu.get_nal_unit_type()==NAL_UNIT_TYPE_CODED_SLICE_NON_IDR){
                            memcpy(&dji_data_buff[dji_data_buff_size],nalu.getData(),nalu.getSize());
                            dji_data_buff_size+=nalu.getSize();
                        }
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
