//
// Created by Constantin on 2/6/2019.
//


#include "ParseRAW.h"
#include <android/log.h>
#include <AndroidLogger.hpp>

ParseRAW::ParseRAW(NALU_DATA_CALLBACK cb):cb(cb){
}

void ParseRAW::reset(){
    nalu_data_position=4;
    nalu_search_state=0;
    dji_data_buff_size=0;
    /*nalu_data.reserve(NALU::NALU_MAXLEN);
    nalu_data.push_back(0);
    nalu_data.push_back(0);
    nalu_data.push_back(0);
    nalu_data.push_back(1);*/
}

void ParseRAW::getAvailableBuffer(){
    std::lock_guard<std::mutex> lock(mBufferMutex);
    for(const auto buff:allocatedBuffers){

    }
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
                        const size_t naluLen=nalu_data_position-4;
                        //test(nalu_data,naluLen);

                        //auto x=MLOGD;
                        //x<<"Vector holds";
                        //for(auto& el: nalu_data) x << el << ' ';

                        /*nalu_data.resize(dataLen);
                        auto lol=std::vector<uint8_t>(nalu_data.data(),nalu_data.data()+dataLen);
                        lol.resize(dataLen);
                        NALU nalu(lol);
                        nalu_data.resize(0);*/
                        //nalu_data.resize(naluLen);
                        NALU nalu(nalu_data,naluLen);

                        //auto x2=MLOGD;
                        //x2<<"Vector holds";
                        //for(auto& el: nalu_data) x2 << el << ' ';


                        //MLOGD<<"Size is"<<nalu_data.size()<<" capacity is"<<nalu_data.capacity();
                        //std::vector<uint8_t> v;
                        //for(int i=0;i<nalu_data_position-4;i++){
                        //    v.push_back(nalu_data[i]);
                        //}
                        //v.resize(nalu_data_position-4);
                        //nalu_data.resize(nalu_data_position-4);
                        //MLOGD<<"N size is"<<nalu_data_position-4;
                        //NALU nalu(nalu_data);

                        cb(nalu);

                        /*auto x=MLOGD;
                        std::vector<int> c = {1, 2, 3};
                        x << "The vector holds: ";
                        for(auto& el: c) x << el << ' ';
                        x << '\n';
                        c.resize(5);
                        x << "After resize up to 5: ";
                        for(auto& el: c) x << el << ' ';
                        x << '\n';
                        c.resize(2);
                        x << "After resize down to 2: ";
                        for(auto& el: c) x << el << ' ';
                        x << '\n';
                        MLOGD<<"Size is"<<c.size()<<"capacity "<<c.capacity();*/

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
    /*for (size_t i = 0; i < data_length; ++i) {
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
    }*/
}
