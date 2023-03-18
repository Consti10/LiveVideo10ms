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
}

void ParseRAW::parseData(const uint8_t* data,const size_t data_length,const bool isH265){
    //if(nalu_data== nullptr){
    //    nalu_data=new uint8_t[NALU::NALU_MAXLEN];
    //}
    //MLOGD<<"NALU data "<<data_length;
    for (size_t i = 0; i < data_length; ++i) {
        nalu_data[nalu_data_position++] = data[i];
        if (nalu_data_position >= NALU::NALU_MAXLEN - 1) {
            // This should never happen, but rather continue parsing than
            // possibly raising an 'memory access' exception
            nalu_data_position = 0;
        }
        // Since the '0,0,0,1' is written by the loop,
        // The 5th byte is the first byte that is actually 'parsed'
        if(nalu_data_position==5){
            timePointStartOfReceivingNALU=std::chrono::steady_clock::now();
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
                    // Forward NALU only if it has enough data
                    //if(cb!=nullptr && nalu_data_position>=4){
                    if(cb!=nullptr && nalu_data_position>=4 ){
                        const size_t naluLen=nalu_data_position-4;
                        const size_t minNaluSize=NALU::getMinimumNaluSize(isH265);
                        if(naluLen>=minNaluSize){
                            NALU nalu(nalu_data.data(),naluLen,isH265);
                            cb(nalu);
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

void ParseRAW::parseDjiLiveVideoDataH264(const uint8_t* data,const size_t data_length){
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
                        const size_t minNaluSize=NALU::getMinimumNaluSize(false);
                        if(naluLen>=minNaluSize){
                            NALU nalu(nalu_data.data(),naluLen);
                            if(nalu.isSPS() || nalu.isPPS()){
                                cb(nalu);
                                dji_data_buff_size=0;
                            }else if(nalu.get_nal_unit_type()==NAL_UNIT_TYPE_AUD){
                                if(dji_data_buff_size>0){
                                    NALU nalu2(dji_data_buff.data(),dji_data_buff_size);
                                    cb(nalu2);
                                    dji_data_buff_size=0;
                                    // do not forget to also forward the AUD NALU
                                    cb(nalu);
                                }
                            }else if(nalu.get_nal_unit_type()==NAL_UNIT_TYPE_CODED_SLICE_NON_IDR){
                                memcpy(&dji_data_buff[dji_data_buff_size],nalu.getData(),nalu.getSize());
                                dji_data_buff_size+=nalu.getSize();
                            }
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

void ParseRAW::parseJetsonRawSlicedH264(const uint8_t* data, const size_t data_length){
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
                        const size_t minNaluSize=NALU::getMinimumNaluSize(false);
                        if(naluLen>=minNaluSize){
                            NALU nalu(nalu_data.data(),naluLen);
                            //MLOGD<<"ParseRawJ NALU type:"<<nalu.get_nal_name();
                            if(nalu.isSPS() || nalu.isPPS()){
                                cb(nalu);
                                //dji_data_buff_size=0;
                            }else{
                                //accumulateSlicedNALUsByAUD(nalu);
                                accumulateSlicedNALUsByOther(nalu);
                            }
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

void ParseRAW::accumulateSlicedNALUsByAUD(const NALU& nalu){
    // Make sure we do not crash when AUDs were not received properly
    if(nalu.getSize()+dji_data_buff_size>dji_data_buff.size()){
        dji_data_buff_size=0;
        return;
    }
    if(nalu.get_nal_unit_type()==NAL_UNIT_TYPE_AUD) {
        if (dji_data_buff_size > 0) {
            // add the AUD,too:
            memcpy(&dji_data_buff[dji_data_buff_size],nalu.getData(),nalu.getSize());
            dji_data_buff_size+=nalu.getSize();
            // and then forward them together as a single unit
            NALU nalu2(dji_data_buff.data(), dji_data_buff_size,nalu.IS_H265_PACKET,timePointFirstNALUToMerge);
            cb(nalu2);
            dji_data_buff_size = 0;
            //cb(nalu);
            //MLOGD<<"Merged "<<nMergedNALUs<<" nalus";
            nMergedNALUs=0;
        }
    }else{
        if(nMergedNALUs==0){
            timePointFirstNALUToMerge=nalu.creationTime;
        }
        memcpy(&dji_data_buff[dji_data_buff_size],nalu.getData(),nalu.getSize());
        dji_data_buff_size+=nalu.getSize();
        nMergedNALUs++;
    }
}

void ParseRAW::accumulateSlicedNALUsByOther(const NALU& nalu){
    const auto N_SLICES_PER_FRAME=4;
    if(nMergedNALUs==0){
        timePointFirstNALUToMerge=nalu.creationTime;
    }

    memcpy(&dji_data_buff[dji_data_buff_size],nalu.getData(),nalu.getSize());
    dji_data_buff_size+=nalu.getSize();
    nMergedNALUs++;

    if(nMergedNALUs==N_SLICES_PER_FRAME){
        NALU nalu2(dji_data_buff.data(), dji_data_buff_size,nalu.IS_H265_PACKET,timePointFirstNALUToMerge);
        cb(nalu2);
        dji_data_buff_size = 0;
        nMergedNALUs=0;
    }
}

