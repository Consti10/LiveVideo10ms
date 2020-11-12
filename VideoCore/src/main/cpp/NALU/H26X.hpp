//
// Created by consti10 on 04.11.20.
//

#ifndef LIVEVIDEO10MS_H264_H
#define LIVEVIDEO10MS_H264_H

#include <h264_stream.h>
#include "NALUnitType.hpp"

// namespaces for H264 H265 helper
// A H265 NALU is kind of similar to a H264 NALU in that it has the same [0,0,0,1] prefix

namespace AnnexBHelper{
    // The rbsp buffer starts after the 0,0,0,1 header
    // This function *reverts' the fact that a NAL unit mustn't contain the [0,0,0,1] pattern (Annex B)
    static std::vector<uint8_t> nalu_annexB_to_rbsp_buff(const std::vector<uint8_t>& naluData){
        int nal_size = naluData.size()-4;
        const uint8_t* nal_data=&naluData[4];
        int rbsp_size=nal_size;
        std::vector<uint8_t> rbsp_buf;
        rbsp_buf.resize(rbsp_size);
        int rc = nal_to_rbsp(nal_data, &nal_size, rbsp_buf.data(), &rbsp_size);
        assert(rc>0);
        //MLOGD<<"X "<<rbsp_buf.size()<<" Y"<<rbsp_size;
        //assert(rbsp_buf.size()==rbsp_size);
        rbsp_buf.resize(rbsp_size);
        return rbsp_buf;
    }
    static std::vector<uint8_t> nalu_annexB_to_rbsp_buff(const uint8_t* nalu_data, std::size_t nalu_data_size){
        return nalu_annexB_to_rbsp_buff(std::vector<uint8_t>(nalu_data, nalu_data + nalu_data_size));
    }
}

namespace H264{
    // reverse order due to architecture
    typedef struct nal_unit_header{
        uint8_t nal_unit_type:5;
        uint8_t nal_ref_idc:2;
        uint8_t forbidden_zero_bit:1;
        std::string asString()const{
            std::stringstream ss;
            ss<<"nal_unit_type:"<<(int)nal_unit_type<<" nal_ref_idc:"<<(int)nal_ref_idc<<" forbidden_zero_bit:"<<(int)forbidden_zero_bit;
            return ss.str();
        }
    }__attribute__ ((packed)) nal_unit_header_t;
    static_assert(sizeof(nal_unit_header_t)==1);
    typedef struct slice_header{
        uint8_t frame_num:2;
        uint8_t pic_parameter_set_id:2;
        uint8_t slice_type:2;
        uint8_t first_mb_in_slice:2 ;
        std::string asString()const{
            std::stringstream ss;
            ss<<"first_mb_in_slice:"<<(int)first_mb_in_slice<<" slice_type:"<<(int)slice_type<<" pic_parameter_set_id:"<<(int)pic_parameter_set_id<<" frame_num:"<<(int)frame_num;
            return ss.str();
        }
    }__attribute__ ((packed)) slice_header_t;
    static_assert(sizeof(slice_header_t)==1);

    // Parse raw NALU data into an sps struct (using the h264bitstream library)
    class SPS{
    public:
        nal_unit_header_t nal_header;
        sps_t parsed;
    public:
        // data buffer= NALU data with prefix
        SPS(const uint8_t* nalu_data,size_t data_len){
            auto rbsp_buf= AnnexBHelper::nalu_annexB_to_rbsp_buff(nalu_data, data_len);
            BitStream b(rbsp_buf);
            nal_header.forbidden_zero_bit=b.read_u1();
            nal_header.nal_ref_idc = b.read_u2();
            nal_header.nal_unit_type = b.read_u5();
            assert(nal_header.forbidden_zero_bit==0);
            assert(nal_header.nal_unit_type==NAL_UNIT_TYPE_SPS);
            read_seq_parameter_set_rbsp(&parsed, b.bs_t());
            read_rbsp_trailing_bits(b.bs_t());
        }
        std::array<int,2> getWidthHeightPx()const{
            int Width = ((parsed.pic_width_in_mbs_minus1 +1)*16) -parsed.frame_crop_right_offset *2 -parsed.frame_crop_left_offset *2;
            int Height = ((2 -parsed.frame_mbs_only_flag)* (parsed.pic_height_in_map_units_minus1 +1) * 16) - (parsed.frame_crop_bottom_offset* 2) - (parsed.frame_crop_top_offset* 2);
            return {Width,Height};
        }
        std::string asString()const{
            //return spsAsString(&parsed);
            return H264Stream::spsAsString(&parsed);
        }
    };
}

namespace H265{
    typedef struct nal_unit_header{
        uint8_t forbidden_zero_bit:1;
        uint8_t nal_unit_type:6;
        uint8_t nuh_layer_id:6;
        uint8_t nuh_temporal_id_plus1:3;
    }__attribute__ ((packed)) nal_unit_header_t;
    static_assert(sizeof(nal_unit_header_t)==2);
}

#endif //LIVEVIDEO10MS_H264_H
