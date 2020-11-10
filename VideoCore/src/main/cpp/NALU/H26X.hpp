//
// Created by consti10 on 04.11.20.
//

#ifndef LIVEVIDEO10MS_H264_H
#define LIVEVIDEO10MS_H264_H

#include <h264_stream.h>

// namespaces for H264 H265 helper
// A H265 NALU is kind of similar to a H264 NALU in that it has the same [0,0,0,1] prefix

namespace H264{
    static std::string get_nal_name(int nal_unit_type){
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
            default :                                           nal_unit_type_name = std::string("Unknown")+std::to_string(nal_unit_type); break;
        }
        return nal_unit_type_name;
    };
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

    static std::string spsAsString(sps_t* sps){
        std::stringstream ss;
        ss<<"[";
        ss<<"profile_idc="<<sps->profile_idc<<",";
        ss<<"constraint_set0_flag="<<sps->constraint_set0_flag<<",";
        ss<<"constraint_set1_flag="<<sps->constraint_set1_flag<<",";
        ss<<"constraint_set2_flag="<<sps->constraint_set2_flag<<",";
        ss<<"constraint_set3_flag="<<sps->constraint_set3_flag<<",";
        ss<<"constraint_set4_flag="<<sps->constraint_set4_flag<<",";
        ss<<"constraint_set5_flag="<<sps->constraint_set5_flag<<",";
        ss<<"reserved_zero_2bits="<<sps->reserved_zero_2bits<<",";
        ss<<"level_idc="<<sps->level_idc<<",";
        ss<<"seq_parameter_set_id="<<sps->seq_parameter_set_id<<",";
        ss<<"chroma_format_idc="<<sps->chroma_format_idc<<",";
        ss<<"residual_colour_transform_flag="<<sps->residual_colour_transform_flag<<",";
        ss<<"bit_depth_luma_minus8="<<sps->bit_depth_luma_minus8<<",";
        ss<<"bit_depth_chroma_minus8="<<sps->bit_depth_chroma_minus8<<",";
        ss<<"qpprime_y_zero_transform_bypass_flag="<<sps->qpprime_y_zero_transform_bypass_flag<<",";
        ss<<"seq_scaling_matrix_present_flag="<<sps->seq_scaling_matrix_present_flag<<",";
        ss<<"log2_max_frame_num_minus4="<<sps->log2_max_frame_num_minus4<<",";
        ss<<"pic_order_cnt_type="<<sps->pic_order_cnt_type<<",";
        ss<<"log2_max_pic_order_cnt_lsb_minus4="<<sps->log2_max_pic_order_cnt_lsb_minus4<<",";
        ss<<"delta_pic_order_always_zero_flag="<<sps->delta_pic_order_always_zero_flag<<",";
        //ss<<"="<<sps-><<",";
        ss<<"]";
        return ss.str();
    }

    static std::string spsAsString(const uint8_t* nalu_data,size_t data_len){
        h264_stream_t* h = h264_new();
        read_nal_unit(h,&nalu_data[4],(int)data_len-4);
        sps_t* sps=h->sps;
        assert(sps!= nullptr);
        auto ret=spsAsString(sps);
        h264_free(h);
        return ret;
    }

    static std::array<int,2> spsGetWidthHeight(const uint8_t* nalu_data,size_t data_len){
        h264_stream_t* h = h264_new();
        read_nal_unit(h,&nalu_data[4],(int)data_len-4);
        sps_t* sps=h->sps;
        assert(sps!= nullptr);
        int Width = ((sps->pic_width_in_mbs_minus1 +1)*16) -sps->frame_crop_right_offset *2 -sps->frame_crop_left_offset *2;
        int Height = ((2 -sps->frame_mbs_only_flag)* (sps->pic_height_in_map_units_minus1 +1) * 16) - (sps->frame_crop_bottom_offset* 2) - (sps->frame_crop_top_offset* 2);
        h264_free(h);
        return {Width,Height};
    }
}

namespace H265{
    enum NalUnitType {
        NAL_UNIT_CODED_SLICE_TRAIL_N = 0,
        NAL_UNIT_CODED_SLICE_TRAIL_R,

        NAL_UNIT_CODED_SLICE_TSA_N,
        NAL_UNIT_CODED_SLICE_TSA_R,

        NAL_UNIT_CODED_SLICE_STSA_N,
        NAL_UNIT_CODED_SLICE_STSA_R,

        NAL_UNIT_CODED_SLICE_RADL_N,
        NAL_UNIT_CODED_SLICE_RADL_R,

        NAL_UNIT_CODED_SLICE_RASL_N,
        NAL_UNIT_CODED_SLICE_RASL_R,

        NAL_UNIT_RESERVED_VCL_N10,
        NAL_UNIT_RESERVED_VCL_R11,
        NAL_UNIT_RESERVED_VCL_N12,
        NAL_UNIT_RESERVED_VCL_R13,
        NAL_UNIT_RESERVED_VCL_N14,
        NAL_UNIT_RESERVED_VCL_R15,

        NAL_UNIT_CODED_SLICE_BLA_W_LP,
        NAL_UNIT_CODED_SLICE_BLA_W_RADL,
        NAL_UNIT_CODED_SLICE_BLA_N_LP,
        NAL_UNIT_CODED_SLICE_IDR_W_RADL,
        NAL_UNIT_CODED_SLICE_IDR_N_LP,
        NAL_UNIT_CODED_SLICE_CRA,
        NAL_UNIT_RESERVED_IRAP_VCL22,
        NAL_UNIT_RESERVED_IRAP_VCL23,

        NAL_UNIT_RESERVED_VCL24,
        NAL_UNIT_RESERVED_VCL25,
        NAL_UNIT_RESERVED_VCL26,
        NAL_UNIT_RESERVED_VCL27,
        NAL_UNIT_RESERVED_VCL28,
        NAL_UNIT_RESERVED_VCL29,
        NAL_UNIT_RESERVED_VCL30,
        NAL_UNIT_RESERVED_VCL31,

        NAL_UNIT_VPS,
        NAL_UNIT_SPS,
        NAL_UNIT_PPS,
        NAL_UNIT_ACCESS_UNIT_DELIMITER,
        NAL_UNIT_EOS,
        NAL_UNIT_EOB,
        NAL_UNIT_FILLER_DATA,
        NAL_UNIT_PREFIX_SEI,
        NAL_UNIT_SUFFIX_SEI,
        NAL_UNIT_RESERVED_NVCL41,
        NAL_UNIT_RESERVED_NVCL42,
        NAL_UNIT_RESERVED_NVCL43,
        NAL_UNIT_RESERVED_NVCL44,
        NAL_UNIT_RESERVED_NVCL45,
        NAL_UNIT_RESERVED_NVCL46,
        NAL_UNIT_RESERVED_NVCL47,
        NAL_UNIT_UNSPECIFIED_48,
        NAL_UNIT_UNSPECIFIED_49,
        NAL_UNIT_UNSPECIFIED_50,
        NAL_UNIT_UNSPECIFIED_51,
        NAL_UNIT_UNSPECIFIED_52,
        NAL_UNIT_UNSPECIFIED_53,
        NAL_UNIT_UNSPECIFIED_54,
        NAL_UNIT_UNSPECIFIED_55,
        NAL_UNIT_UNSPECIFIED_56,
        NAL_UNIT_UNSPECIFIED_57,
        NAL_UNIT_UNSPECIFIED_58,
        NAL_UNIT_UNSPECIFIED_59,
        NAL_UNIT_UNSPECIFIED_60,
        NAL_UNIT_UNSPECIFIED_61,
        NAL_UNIT_UNSPECIFIED_62,
        NAL_UNIT_UNSPECIFIED_63,
        NAL_UNIT_INVALID,
    };

    static std::string get_nal_name(const int nal_unit_type){
        std::string nal_unit_type_name="Unimpl";
        switch (nal_unit_type){
            case NalUnitType::NAL_UNIT_CODED_SLICE_TRAIL_N:
                nal_unit_type_name="NAL_UNIT_CODED_SLICE_TRAIL_N";break;
            case NalUnitType::NAL_UNIT_CODED_SLICE_TRAIL_R:
                nal_unit_type_name="NAL_UNIT_CODED_SLICE_TRAIL_R";break;

            case NalUnitType::NAL_UNIT_CODED_SLICE_TSA_N:
                nal_unit_type_name="NAL_UNIT_CODED_SLICE_TSA_N";break;
            case NalUnitType::NAL_UNIT_CODED_SLICE_TSA_R:
                nal_unit_type_name="NAL_UNIT_CODED_SLICE_TSA_R";break;

            case NalUnitType::NAL_UNIT_CODED_SLICE_STSA_N:
                nal_unit_type_name="NAL_UNIT_CODED_SLICE_STSA_N";break;
            case NalUnitType::NAL_UNIT_CODED_SLICE_STSA_R:
                nal_unit_type_name="NAL_UNIT_CODED_SLICE_STSA_R";break;

            case NalUnitType::NAL_UNIT_CODED_SLICE_RADL_N:
                nal_unit_type_name="NAL_UNIT_CODED_SLICE_RADL_N";break;
            case NalUnitType::NAL_UNIT_CODED_SLICE_RADL_R:
                nal_unit_type_name="NAL_UNIT_CODED_SLICE_RADL_R";break;

            case NalUnitType::NAL_UNIT_CODED_SLICE_RASL_N:
                nal_unit_type_name="NAL_UNIT_CODED_SLICE_RASL_N";break;
            case NalUnitType::NAL_UNIT_CODED_SLICE_RASL_R:
                nal_unit_type_name="NAL_UNIT_CODED_SLICE_RASL_R";break;

            case NalUnitType::NAL_UNIT_RESERVED_VCL_N10:
                nal_unit_type_name="NAL_UNIT_RESERVED_VCL_N10";break;
            case NalUnitType::NAL_UNIT_RESERVED_VCL_R11:
                nal_unit_type_name="NAL_UNIT_RESERVED_VCL_R11";break;
            case NalUnitType::NAL_UNIT_RESERVED_VCL_N12:
                nal_unit_type_name="NAL_UNIT_RESERVED_VCL_N12";break;
            case NalUnitType::NAL_UNIT_RESERVED_VCL_R13:
                nal_unit_type_name="NAL_UNIT_RESERVED_VCL_R13";break;
            case NalUnitType::NAL_UNIT_RESERVED_VCL_N14:
                nal_unit_type_name="NAL_UNIT_RESERVED_VCL_N14";break;
            case NalUnitType::NAL_UNIT_RESERVED_VCL_R15:
                nal_unit_type_name="NAL_UNIT_RESERVED_VCL_R15";break;

            case NalUnitType::NAL_UNIT_CODED_SLICE_BLA_W_LP:
                nal_unit_type_name="NAL_UNIT_CODED_SLICE_BLA_W_LP";break;
            case NalUnitType::NAL_UNIT_CODED_SLICE_BLA_W_RADL:
                nal_unit_type_name="NAL_UNIT_CODED_SLICE_BLA_W_RADL";break;
            case NalUnitType::NAL_UNIT_CODED_SLICE_BLA_N_LP:
                nal_unit_type_name="NAL_UNIT_CODED_SLICE_BLA_N_LP";break;
            case NalUnitType::NAL_UNIT_CODED_SLICE_IDR_W_RADL:
                nal_unit_type_name="NAL_UNIT_CODED_SLICE_IDR_W_RADL";break;
            case NalUnitType::NAL_UNIT_CODED_SLICE_IDR_N_LP:
                nal_unit_type_name="NAL_UNIT_CODED_SLICE_IDR_N_LP";break;
            case NalUnitType::NAL_UNIT_CODED_SLICE_CRA:
                nal_unit_type_name="NAL_UNIT_CODED_SLICE_CRA";break;
            case NalUnitType::NAL_UNIT_RESERVED_IRAP_VCL22:
                nal_unit_type_name="NAL_UNIT_RESERVED_IRAP_VCL22";break;
            case NalUnitType:: NAL_UNIT_RESERVED_IRAP_VCL23:
                nal_unit_type_name="NAL_UNIT_RESERVED_IRAP_VCL23";break;

            case NalUnitType::NAL_UNIT_RESERVED_VCL24:
                nal_unit_type_name="NAL_UNIT_RESERVED_VCL24";break;
            case NalUnitType::NAL_UNIT_RESERVED_VCL25:
                nal_unit_type_name="NAL_UNIT_RESERVED_VCL25";break;
            case NalUnitType::NAL_UNIT_RESERVED_VCL26:
                nal_unit_type_name="NAL_UNIT_RESERVED_VCL26";break;
            case NalUnitType::NAL_UNIT_RESERVED_VCL27:
                nal_unit_type_name="NAL_UNIT_RESERVED_VCL27";break;
            case NalUnitType::NAL_UNIT_RESERVED_VCL28:
                nal_unit_type_name="NAL_UNIT_RESERVED_VCL28";break;
            case NalUnitType::NAL_UNIT_RESERVED_VCL29:
                nal_unit_type_name="NAL_UNIT_RESERVED_VCL29";break;
            case NalUnitType::NAL_UNIT_RESERVED_VCL30:
                nal_unit_type_name="NAL_UNIT_RESERVED_VCL30";break;
            case NalUnitType::NAL_UNIT_RESERVED_VCL31:
                nal_unit_type_name="NAL_UNIT_RESERVED_VCL31";break;

            case NalUnitType::NAL_UNIT_VPS:
                nal_unit_type_name="NAL_UNIT_VPS";break;
            case NalUnitType::NAL_UNIT_SPS:
                nal_unit_type_name="NAL_UNIT_SPS";break;
            case NalUnitType::NAL_UNIT_PPS:
                nal_unit_type_name="NAL_UNIT_PPS";break;
            case NalUnitType::NAL_UNIT_ACCESS_UNIT_DELIMITER:
                nal_unit_type_name="NAL_UNIT_ACCESS_UNIT_DELIMITER";break;
            case NalUnitType::NAL_UNIT_EOS:
                nal_unit_type_name="NAL_UNIT_EOS";break;
            case NalUnitType::NAL_UNIT_EOB:
                nal_unit_type_name="NAL_UNIT_EOB";break;
            case NalUnitType:: NAL_UNIT_FILLER_DATA:
                nal_unit_type_name="NAL_UNIT_FILLER_DATA";break;
            case NalUnitType::NAL_UNIT_PREFIX_SEI:
                nal_unit_type_name="NAL_UNIT_PREFIX_SEI";break;
            case NalUnitType::NAL_UNIT_SUFFIX_SEI:
                nal_unit_type_name="NAL_UNIT_SUFFIX_SEI";break;
            case NalUnitType::NAL_UNIT_RESERVED_NVCL41:
                nal_unit_type_name="NAL_UNIT_RESERVED_NVCL41";break;
            case NalUnitType::NAL_UNIT_RESERVED_NVCL42:
                nal_unit_type_name="NAL_UNIT_RESERVED_NVCL42";break;
            case NalUnitType::NAL_UNIT_RESERVED_NVCL43:
                nal_unit_type_name="NAL_UNIT_RESERVED_NVCL43";break;
            case NalUnitType::NAL_UNIT_RESERVED_NVCL44:
                nal_unit_type_name="NAL_UNIT_RESERVED_NVCL44";break;
            case NalUnitType:: NAL_UNIT_RESERVED_NVCL45:
                nal_unit_type_name="NAL_UNIT_RESERVED_NVCL45";break;
            case NalUnitType:: NAL_UNIT_RESERVED_NVCL46:
                nal_unit_type_name="NAL_UNIT_RESERVED_NVCL46";break;
            case NalUnitType::NAL_UNIT_RESERVED_NVCL47:
                nal_unit_type_name="NAL_UNIT_RESERVED_NVCL47";break;
            case NalUnitType::NAL_UNIT_UNSPECIFIED_48:
                nal_unit_type_name="NAL_UNIT_UNSPECIFIED_48";break;
            case NalUnitType::NAL_UNIT_UNSPECIFIED_49:
                nal_unit_type_name="NAL_UNIT_UNSPECIFIED_49";break;
            case NalUnitType::NAL_UNIT_UNSPECIFIED_50:
                nal_unit_type_name="NAL_UNIT_UNSPECIFIED_50";break;
            case NalUnitType::NAL_UNIT_UNSPECIFIED_51:
                nal_unit_type_name="NAL_UNIT_UNSPECIFIED_51";break;
            case NalUnitType:: NAL_UNIT_UNSPECIFIED_52:
                nal_unit_type_name="NAL_UNIT_UNSPECIFIED_52";break;
            case NalUnitType:: NAL_UNIT_UNSPECIFIED_53:
                nal_unit_type_name="NAL_UNIT_UNSPECIFIED_53";break;
            case NalUnitType::NAL_UNIT_UNSPECIFIED_54:
                nal_unit_type_name="NAL_UNIT_UNSPECIFIED_54";break;
            case NalUnitType::NAL_UNIT_UNSPECIFIED_55:
                nal_unit_type_name="NAL_UNIT_UNSPECIFIED_55";break;
            case NalUnitType:: NAL_UNIT_UNSPECIFIED_56:
                nal_unit_type_name="NAL_UNIT_UNSPECIFIED_56";break;
            case NalUnitType::NAL_UNIT_UNSPECIFIED_57:
                nal_unit_type_name="NAL_UNIT_UNSPECIFIED_57";break;
            case NalUnitType:: NAL_UNIT_UNSPECIFIED_58:
                nal_unit_type_name="NAL_UNIT_UNSPECIFIED_58";break;
            case NalUnitType:: NAL_UNIT_UNSPECIFIED_59:
                nal_unit_type_name="NAL_UNIT_UNSPECIFIED_59";break;
            case NalUnitType::NAL_UNIT_UNSPECIFIED_60:
                nal_unit_type_name="NAL_UNIT_UNSPECIFIED_60";break;
            case NalUnitType:: NAL_UNIT_UNSPECIFIED_61:
                nal_unit_type_name="NAL_UNIT_UNSPECIFIED_61";break;
            case NalUnitType::NAL_UNIT_UNSPECIFIED_62:
                nal_unit_type_name="NAL_UNIT_UNSPECIFIED_62";break;
            case NalUnitType:: NAL_UNIT_UNSPECIFIED_63:
                nal_unit_type_name="NAL_UNIT_UNSPECIFIED_63";break;
            case NalUnitType:: NAL_UNIT_INVALID:
                nal_unit_type_name="NAL_UNIT_INVALID";break;
            default :
                nal_unit_type_name = std::string("Unknown")+std::to_string(nal_unit_type); break;
        }
        return nal_unit_type_name;
    }
    typedef struct nal_unit_header{
        uint8_t forbidden_zero_bit:1;
        uint8_t nal_unit_type:6;
        uint8_t nuh_layer_id:6;
        uint8_t nuh_temporal_id_plus1:3;
    }__attribute__ ((packed)) nal_unit_header_t;
    static_assert(sizeof(nal_unit_header_t)==2);
}

#endif //LIVEVIDEO10MS_H264_H
