//
// Created by consti10 on 04.11.20.
//

#ifndef LIVEVIDEO10MS_H264_H
#define LIVEVIDEO10MS_H264_H

#include <h264_stream.h>
#include "NALUnitType.hpp"
#include <h265_common.h>

// namespaces for H264 H265 helper
// A H265 NALU is kind of similar to a H264 NALU in that it has the same [0,0,0,1] prefix

namespace RBSPHelper{
    // The rbsp buffer starts after 5 bytes for h264 (4 bytes prefix and 1 byte unit header)
    // and after 6 bytes for h265 (4 bytes prefix and 2 byte unit header)
    // the h264bitstream nal_to_rbsp function is buggy !
    // Also, its implementation unescapes the NAL unit header which is wrong (only the payload should be unescaped)
    // escaping/unescaping rbsp is the same for h264 and h265

    static std::vector<uint8_t> unescapeRbsp(const uint8_t* rbsp_buff,const std::size_t rbsp_buff_size){
        auto ret=h265nal::UnescapeRbsp(rbsp_buff,rbsp_buff_size);
        return ret;
    }
    static std::vector<uint8_t> unescapeRbsp(const std::vector<uint8_t>& rbspData){
        return unescapeRbsp(rbspData.data(),rbspData.size());
    }

    static std::vector<uint8_t> escapeRbsp(const std::vector<uint8_t>& rbspBuff){
        std::vector<uint8_t> naluBuff;
        naluBuff.resize(rbspBuff.size()+32);
        int rbspSize=rbspBuff.size();
        int nalSize=naluBuff.size();
        auto tmp=rbsp_to_nal(rbspBuff.data(),&rbspSize,naluBuff.data(),&nalSize);
        assert(tmp>0);
        naluBuff.resize(tmp);
        // workaround h264bitstream library
        return std::vector<uint8_t>(&naluBuff.data()[1],&naluBuff.data()[1]+naluBuff.size()-1);
    }

    static void test_unescape_escape(const std::vector<uint8_t>& rbspData){
        auto unescapedData=unescapeRbsp(rbspData.data(),rbspData.size());
        auto unescapedThenEscapedData=escapeRbsp(unescapedData);
        MLOGD<<"Y1:"<<StringHelper::vectorAsString(rbspData);
        MLOGD<<"Y2:"<<StringHelper::vectorAsString(unescapedData);
        MLOGD<<"Y3:"<<StringHelper::vectorAsString(unescapedThenEscapedData);
        // check if the data stayed the same after unescaping then escaping it again
        assert(rbspData.size()==unescapedThenEscapedData.size());
        assert(std::memcmp(rbspData.data(),unescapedThenEscapedData.data(),rbspData.size())==0);
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
            memcpy(&nal_header,&nalu_data[4],1);
            assert(nal_header.forbidden_zero_bit==0);
            assert(nal_header.nal_unit_type==NAL_UNIT_TYPE_SPS);
            auto rbsp_buf= RBSPHelper::unescapeRbsp(&nalu_data[5], data_len-5);
            BitStream b(rbsp_buf);
            read_seq_parameter_set_rbsp(&parsed, b.bs_t());
            read_rbsp_trailing_bits(b.bs_t());
        }
        std::array<int,2> getWidthHeightPx()const{
            int Width = ((parsed.pic_width_in_mbs_minus1 +1)*16) -parsed.frame_crop_right_offset *2 -parsed.frame_crop_left_offset *2;
            int Height = ((2 -parsed.frame_mbs_only_flag)* (parsed.pic_height_in_map_units_minus1 +1) * 16) - (parsed.frame_crop_bottom_offset* 2) - (parsed.frame_crop_top_offset* 2);
            return {Width,Height};
        }
        std::string asString()const{
            return H264Stream::spsAsString(&parsed);
        }
// --------------------------------------------- crude hacking --------------------------------------------
        std::vector<uint8_t> asNALU()const{
            std::vector<uint8_t> naluBuff;
            // write prefix and nalu header
            naluBuff.resize(5);
            naluBuff[0]=0;
            naluBuff[1]=0;
            naluBuff[2]=0;
            naluBuff[3]=1;
            std::memcpy(&naluBuff.data()[4],&nal_header,1);
            // write sps payload into rbspBuff
            std::vector<uint8_t> rbspBuff;
            rbspBuff.resize(14+64*4);
            //MLOGD<<"rbspbuffSize"<<rbspBuff.size();
            BitStream b(rbspBuff);
            //
            write_seq_parameter_set_rbsp(&parsed,b.bs_t());
            write_rbsp_trailing_bits(b.bs_t());

            if (bs_overrun(b.bs_t())) {
                MLOGE<<"BS overrun ";
            }
            auto rbsp_size = bs_pos(b.bs_t());
            rbspBuff.resize(rbsp_size);
            // escape sps payload and append to nalu buffer
            auto rbspBuffEscaped=RBSPHelper::escapeRbsp(rbspBuff);
            naluBuff.insert(naluBuff.end(), rbspBuffEscaped.begin(), rbspBuffEscaped.end());
            return naluBuff;
        }
        void increaseLatency(){
            parsed.pic_order_cnt_type=0;
            parsed.log2_max_pic_order_cnt_lsb_minus4=4;
        }
        void decreaseLatency(){
            //parsed.level_idc=40;
            parsed.pic_order_cnt_type=2;
            parsed.log2_max_pic_order_cnt_lsb_minus4=0;
        }
        void experiment(){
            //parsed.level_idc=40;
        }
        void addVUI(){
            parsed.vui_parameters_present_flag=1;
            parsed.vui.aspect_ratio_info_present_flag=0;
            parsed.vui.aspect_ratio_idc=0;
            parsed.vui.sar_width=0;
            parsed.vui.sar_height=0;
            parsed.vui.overscan_info_present_flag=0;
            parsed.vui.overscan_appropriate_flag=0;
            parsed.vui.video_signal_type_present_flag=1;
            parsed.vui.video_format=5;
            parsed.vui.video_full_range_flag=0;
            parsed.vui.colour_description_present_flag=1;
            parsed.vui.colour_primaries=0;
            parsed.vui.transfer_characteristics=0;
            parsed.vui.matrix_coefficients=5;
            parsed.vui.chroma_loc_info_present_flag=0;
            parsed.vui.chroma_sample_loc_type_top_field=0;
            parsed.vui.chroma_sample_loc_type_bottom_field=0;
            parsed.vui.timing_info_present_flag=1;
            parsed.vui.num_units_in_tick=1;
            parsed.vui.time_scale=120;
            parsed.vui.fixed_frame_rate_flag=1;
            parsed.vui.nal_hrd_parameters_present_flag=0;
            parsed.vui.vcl_hrd_parameters_present_flag=0;
            parsed.vui.low_delay_hrd_flag=0;
            parsed.vui.pic_struct_present_flag=0;
            parsed.vui.bitstream_restriction_flag=1;
            parsed.vui.motion_vectors_over_pic_boundaries_flag=1;
            parsed.vui.max_bytes_per_pic_denom=0;
            parsed.vui.max_bits_per_mb_denom=0;
            parsed.vui.log2_max_mv_length_horizontal=16;
            parsed.vui.log2_max_mv_length_vertical=16;
            parsed.vui.num_reorder_frames=0;
            parsed.vui.max_dec_frame_buffering=1;
        }
    };
    static void testSPSConversion(const uint8_t* nalu_data,size_t data_len){
        auto sps=H264::SPS(nalu_data,data_len);
        auto naluBuff=sps.asNALU();
        MLOGD<<"Z1:"<<StringHelper::vectorAsString(std::vector<uint8_t>(nalu_data,nalu_data+data_len));
        MLOGD<<"Z2:"<<StringHelper::vectorAsString(naluBuff);
        assert(data_len==naluBuff.size());
        assert(std::memcmp(nalu_data,naluBuff.data(),data_len)==0);
    }
// --------------------------------------------- crude hacking --------------------------------------------

    class PPS{
        public:
            nal_unit_header_t nal_header;
            pps_t parsed;
        public:
            // data buffer= NALU data with prefix
            PPS(const uint8_t* nalu_data,size_t data_len){
                memcpy(&nal_header,&nalu_data[4],1);
                assert(nal_header.forbidden_zero_bit==0);
                assert(nal_header.nal_unit_type==NAL_UNIT_TYPE_PPS);
                auto rbsp_buf= RBSPHelper::unescapeRbsp(&nalu_data[5], data_len-5);
                BitStream b(rbsp_buf);
                read_pic_parameter_set_rbsp(&parsed, b.bs_t());
                read_rbsp_trailing_bits(b.bs_t());
            }
            std::string asString()const{
                return H264Stream::ppsAsString(parsed);
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
