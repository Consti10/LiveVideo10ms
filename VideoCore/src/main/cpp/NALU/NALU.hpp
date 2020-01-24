//
// Created by Constantin on 2/6/2019.
//

#ifndef LIVE_VIDEO_10MS_ANDROID_NALU_H
#define LIVE_VIDEO_10MS_ANDROID_NALU_H

//https://github.com/Dash-Industry-Forum/Conformance-and-reference-source/blob/master/conformance/TSValidator/h264bitstream/h264_stream.h
#define NAL_UNIT_TYPE_UNSPECIFIED                    0    // Unspecified
#define NAL_UNIT_TYPE_CODED_SLICE_NON_IDR            1    // Coded slice of a non-IDR picture
#define NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_A   2    // Coded slice data partition A
#define NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_B   3    // Coded slice data partition B
#define NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_C   4    // Coded slice data partition C
#define NAL_UNIT_TYPE_CODED_SLICE_IDR                5    // Coded slice of an IDR picture
#define NAL_UNIT_TYPE_SEI                            6    // Supplemental enhancement information (SEI)
#define NAL_UNIT_TYPE_SPS                            7    // Sequence parameter set
#define NAL_UNIT_TYPE_PPS                            8    // Picture parameter set
#define NAL_UNIT_TYPE_AUD                            9    // Access unit delimiter
#define NAL_UNIT_TYPE_END_OF_SEQUENCE               10    // End of sequence
#define NAL_UNIT_TYPE_END_OF_STREAM                 11    // End of stream
#define NAL_UNIT_TYPE_FILLER                        12    // Filler data
#define NAL_UNIT_TYPE_SPS_EXT                       13    // Sequence parameter set extension
// 14..18    // Reserved
#define NAL_UNIT_TYPE_CODED_SLICE_AUX               19    // Coded slice of an auxiliary coded picture without partitioning
// 20..23    // Reserved
// 24..31    // Unspecified
//
//
#define NALU_MAXLEN 1024*1024 //test video white iceland: Max 117

#include <string>
#include <chrono>


//A NALU consists of
//a) DATA buffer
//b) buffer length
//c) creation time

class NALU{
public:
    NALU(const uint8_t* data,const int data_length):
        data(data),
        data_length(data_length),
        creationTime{std::chrono::steady_clock::now()}{

    };
    NALU(const uint8_t* data,const int data_length,const std::chrono::steady_clock::time_point creationTime):
        data(data),
        data_length(data_length),
        creationTime{creationTime}{
    };
    const uint8_t* data;
    const int data_length;
    const std::chrono::steady_clock::time_point creationTime;
public:
    bool isSPS()const{
        return (get_nal_unit_type() == NAL_UNIT_TYPE_SPS);
    }
    bool isPPS()const{
        return (get_nal_unit_type() == NAL_UNIT_TYPE_PPS);
    }
    int get_nal_unit_type()const{
        return data[4]&0x1f;
    }
    std::string get_nal_name(int nal_unit_type)const{
       std::string nal_unit_type_name;
       switch (nal_unit_type)
       {
           case  NAL_UNIT_TYPE_UNSPECIFIED :                   nal_unit_type_name = "Unspecified"; break;
           case  NAL_UNIT_TYPE_CODED_SLICE_NON_IDR :           nal_unit_type_name = "Coded slice of a non-IDR picture"; break;
           case  NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_A :  nal_unit_type_name = "Coded slice data partition A"; break;
           case  NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_B :  nal_unit_type_name = "Coded slice data partition B"; break;
           case  NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_C :  nal_unit_type_name = "Coded slice data partition C"; break;
           case  NAL_UNIT_TYPE_CODED_SLICE_IDR :               nal_unit_type_name = "Coded slice of an IDR picture"; break;
           case  NAL_UNIT_TYPE_SEI :                           nal_unit_type_name = "Supplemental enhancement information (SEI)"; break;
           case  NAL_UNIT_TYPE_SPS :                           nal_unit_type_name = "Sequence parameter set"; break;
           case  NAL_UNIT_TYPE_PPS :                           nal_unit_type_name = "Picture parameter set"; break;
           case  NAL_UNIT_TYPE_AUD :                           nal_unit_type_name = "Access unit delimiter"; break;
           case  NAL_UNIT_TYPE_END_OF_SEQUENCE :               nal_unit_type_name = "End of sequence"; break;
           case  NAL_UNIT_TYPE_END_OF_STREAM :                 nal_unit_type_name = "End of stream"; break;
           case  NAL_UNIT_TYPE_FILLER :                        nal_unit_type_name = "Filler data"; break;
           case  NAL_UNIT_TYPE_SPS_EXT :                       nal_unit_type_name = "Sequence parameter set extension"; break;
               // 14..18    // Reserved
           case  NAL_UNIT_TYPE_CODED_SLICE_AUX :               nal_unit_type_name = "Coded slice of an auxiliary coded picture without partitioning"; break;
               // 20..23    // Reserved
               // 24..31    // Unspecified
           default :                                           nal_unit_type_name = "Unknown"; break;
       }
       return nal_unit_type_name;
   };

    std::string get_nal_name()const{
        return get_nal_name(get_nal_unit_type());
    }
};

typedef std::function<void(const NALU& nalu)> NALU_DATA_CALLBACK;


#endif //LIVE_VIDEO_10MS_ANDROID_NALU_H
