/*
 *  Copyright (c) Facebook, Inc. and its affiliates.
 */


#include "h265_slice_parser.h"

#include <stdio.h>

#include <cmath>
#include <cstdint>
#include <vector>

#include "h265_common.h"
#include "h265_st_ref_pic_set_parser.h"
#include "absl/types/optional.h"

namespace {
typedef absl::optional<h265nal::H265StRefPicSetParser::
    StRefPicSetState> OptionalStRefPicSet;
typedef absl::optional<h265nal::H265SliceSegmentHeaderParser::
    SliceSegmentHeaderState> OptionslSliceSegmentHeader;
typedef absl::optional<h265nal::H265SliceSegmentLayerParser::
    SliceSegmentLayerState> OptionalSliceSegmentLayer;
}  // namespace

namespace h265nal {

// General note: this is based off the 2016/12 version of the H.265 standard.
// You can find it on this page:
// http://www.itu.int/rec/T-REC-H.265

// Unpack RBSP and parse slice segment state from the supplied buffer.
absl::optional<H265SliceSegmentLayerParser::SliceSegmentLayerState>
H265SliceSegmentLayerParser::ParseSliceSegmentLayer(
    const uint8_t* data, size_t length, uint32_t nal_unit_type,
    struct H265BitstreamParserState* bitstream_parser_state) {

  std::vector<uint8_t> unpacked_buffer = UnescapeRbsp(data, length);
  rtc::BitBuffer bit_buffer(unpacked_buffer.data(), unpacked_buffer.size());
  return ParseSliceSegmentLayer(&bit_buffer, nal_unit_type,
                                bitstream_parser_state);
}


absl::optional<H265SliceSegmentLayerParser::SliceSegmentLayerState>
H265SliceSegmentLayerParser::ParseSliceSegmentLayer(
    rtc::BitBuffer* bit_buffer, uint32_t nal_unit_type,
    struct H265BitstreamParserState* bitstream_parser_state) {
  // H265 slice segment layer (slice_segment_layer_rbsp()) NAL Unit.
  // Section 7.3.2.9 ("Slice segment layer RBSP syntax") of the H.265
  // standard for a complete description.
  SliceSegmentLayerState slice_segment_layer;

  // input parameters
  slice_segment_layer.nal_unit_type = nal_unit_type;

  // slice_segment_header()
  OptionslSliceSegmentHeader slice_segment_header =
      H265SliceSegmentHeaderParser::ParseSliceSegmentHeader(
          bit_buffer, nal_unit_type, bitstream_parser_state);
  if (slice_segment_header != absl::nullopt) {
    slice_segment_layer.slice_segment_header = *slice_segment_header;
  }

  // slice_segment_data()
  // rbsp_slice_segment_trailing_bits()

  return OptionalSliceSegmentLayer(slice_segment_layer);
}


void H265SliceSegmentLayerParser::SliceSegmentLayerState::fdump(
    XFILE outfp, int indent_level) const {
  XPrintf(outfp, "slice_segment_layer {");
  indent_level = indent_level_incr(indent_level);

  fdump_indent_level(outfp, indent_level);
  slice_segment_header.fdump(outfp, indent_level);

  // slice_segment_data()
  // rbsp_slice_segment_trailing_bits()

  indent_level = indent_level_decr(indent_level);
  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "}");
}


// Unpack RBSP and parse slice segment header state from the supplied buffer.
absl::optional<H265SliceSegmentHeaderParser::SliceSegmentHeaderState>
H265SliceSegmentHeaderParser::ParseSliceSegmentHeader(
    const uint8_t* data, size_t length, uint32_t nal_unit_type,
    struct H265BitstreamParserState* bitstream_parser_state) {

  std::vector<uint8_t> unpacked_buffer = UnescapeRbsp(data, length);
  rtc::BitBuffer bit_buffer(unpacked_buffer.data(), unpacked_buffer.size());
  return ParseSliceSegmentHeader(&bit_buffer, nal_unit_type,
                                 bitstream_parser_state);
}


absl::optional<H265SliceSegmentHeaderParser::SliceSegmentHeaderState>
H265SliceSegmentHeaderParser::ParseSliceSegmentHeader(
    rtc::BitBuffer* bit_buffer, uint32_t nal_unit_type,
    struct H265BitstreamParserState* bitstream_parser_state) {
  uint32_t bits_tmp;
  uint32_t golomb_tmp;

  // H265 slice segment header (slice_segment_layer_rbsp()) NAL Unit.
  // Section 7.3.6.1 ("General slice segment header syntax") of the H.265
  // standard for a complete description.
  SliceSegmentHeaderState slice_segment_header;

  // input parameters
  slice_segment_header.nal_unit_type = nal_unit_type;

  // first_slice_segment_in_pic_flag  u(1)
  if (!bit_buffer->ReadBits(
     &(slice_segment_header.first_slice_segment_in_pic_flag), 1)) {
    return absl::nullopt;
  }

  if (nal_unit_type >= BLA_W_LP && nal_unit_type <= RSV_IRAP_VCL23) {
    // no_output_of_prior_pics_flag  u(1)
    if (!bit_buffer->ReadBits(
        &(slice_segment_header.no_output_of_prior_pics_flag), 1)) {
      return absl::nullopt;
    }
  }

  // slice_pic_parameter_set_id  ue(v)
  if (!bit_buffer->ReadExponentialGolomb(
      &(slice_segment_header.slice_pic_parameter_set_id))) {
    return absl::nullopt;
  }
  uint32_t pps_id = slice_segment_header.slice_pic_parameter_set_id;
  if (bitstream_parser_state->pps.find(pps_id) ==
      bitstream_parser_state->pps.end()) {
    // non-existent PPS id
    return absl::nullopt;
  }

  uint32_t sps_id =
      bitstream_parser_state->pps[pps_id].pps_seq_parameter_set_id;
  if (bitstream_parser_state->sps.find(sps_id) ==
      bitstream_parser_state->sps.end()) {
    // non-existent SPS id
    return absl::nullopt;
  }

  if (!slice_segment_header.first_slice_segment_in_pic_flag) {
    slice_segment_header.dependent_slice_segments_enabled_flag =
        bitstream_parser_state->pps[pps_id].
            dependent_slice_segments_enabled_flag;
    if (slice_segment_header.dependent_slice_segments_enabled_flag) {
      // dependent_slice_segment_flag  u(1)
      if (!bit_buffer->ReadBits(
          &(slice_segment_header.dependent_slice_segment_flag), 1)) {
        return absl::nullopt;
      }
    }
    size_t PicSizeInCtbsY =
        bitstream_parser_state->sps[sps_id].getPicSizeInCtbsY();
    size_t slice_segment_address_len =
        static_cast<size_t>(std::ceil(std::log2(
            static_cast<float>(PicSizeInCtbsY))));
    // range: 0 to PicSizeInCtbsY - 1
    // slice_segment_address  u(v)
    if (!bit_buffer->ReadBits(
        &(slice_segment_header.slice_segment_address),
        slice_segment_address_len)) {
      return absl::nullopt;
    }
  }

  if (!slice_segment_header.dependent_slice_segment_flag) {
    slice_segment_header.num_extra_slice_header_bits =
        bitstream_parser_state->pps[pps_id].num_extra_slice_header_bits;
    for (uint32_t i = 0; i < slice_segment_header.num_extra_slice_header_bits; i++) {
      // slice_reserved_flag[i]  u(1)
      if (!bit_buffer->ReadBits(&bits_tmp, 1)) {
        return absl::nullopt;
      }
      slice_segment_header.slice_reserved_flag.push_back(bits_tmp);
    }

    // slice_type  ue(v)
    if (!bit_buffer->ReadExponentialGolomb(
        &(slice_segment_header.slice_type))) {
      return absl::nullopt;
    }

    slice_segment_header.output_flag_present_flag =
        bitstream_parser_state->pps[pps_id].output_flag_present_flag;
    if (slice_segment_header.output_flag_present_flag) {
      // pic_output_flag  u(1)
      if (!bit_buffer->ReadBits(
          &(slice_segment_header.pic_output_flag), 1)) {
        return absl::nullopt;
      }
    }

    slice_segment_header.separate_colour_plane_flag =
        bitstream_parser_state->sps[sps_id].separate_colour_plane_flag;
    if (slice_segment_header.separate_colour_plane_flag == 1) {
      // colour_plane_id  u(2)
      if (!bit_buffer->ReadBits(&(slice_segment_header.colour_plane_id), 2)) {
        return absl::nullopt;
      }
    }

    if (nal_unit_type != IDR_W_RADL && nal_unit_type != IDR_N_LP) {
      // length of the slice_pic_order_cnt_lsb syntax element is
      // log2_max_pic_order_cnt_lsb_minus4 + 4 bits. The value of the
      // slice_pic_order_cnt_lsb shall be in the range of 0 to
      // MaxPicOrderCntLsb - 1, inclusive.
      slice_segment_header.log2_max_pic_order_cnt_lsb_minus4 =
        bitstream_parser_state->sps[sps_id].log2_max_pic_order_cnt_lsb_minus4;
      size_t slice_pic_order_cnt_lsb_len =
          slice_segment_header.log2_max_pic_order_cnt_lsb_minus4 + 4;
      // slice_pic_order_cnt_lsb  u(v)
      if (!bit_buffer->ReadBits(
          &(slice_segment_header.slice_pic_order_cnt_lsb),
          slice_pic_order_cnt_lsb_len)) {
        return absl::nullopt;
      }

      // short_term_ref_pic_set_sps_flag  u(1)
      if (!bit_buffer->ReadBits(
          &(slice_segment_header.short_term_ref_pic_set_sps_flag), 1)) {
        return absl::nullopt;
      }

      slice_segment_header.num_short_term_ref_pic_sets =
          bitstream_parser_state->sps[sps_id].num_short_term_ref_pic_sets;
      if (!slice_segment_header.short_term_ref_pic_set_sps_flag) {
        // st_ref_pic_set(num_short_term_ref_pic_sets)
        OptionalStRefPicSet st_ref_pic_set =
            H265StRefPicSetParser::ParseStRefPicSet(
                bit_buffer, false,
                slice_segment_header.num_short_term_ref_pic_sets);
        if (st_ref_pic_set != absl::nullopt) {
          slice_segment_header.st_ref_pic_set = *st_ref_pic_set;
        }

      } else if (slice_segment_header.num_short_term_ref_pic_sets > 1) {
        // Ceil(Log2(num_short_term_ref_pic_sets));
        size_t short_term_ref_pic_set_idx_len =
            static_cast<size_t>(std::ceil(std::log2(
                static_cast<float>(
                    slice_segment_header.num_short_term_ref_pic_sets))));
        // short_term_ref_pic_set_idx  u(v)
        if (!bit_buffer->ReadBits(
            &(slice_segment_header.short_term_ref_pic_set_idx),
            short_term_ref_pic_set_idx_len)) {
          return absl::nullopt;
        }
      }

      slice_segment_header.long_term_ref_pics_present_flag =
          bitstream_parser_state->sps[sps_id].long_term_ref_pics_present_flag;
      if (slice_segment_header.long_term_ref_pics_present_flag) {
        slice_segment_header.num_long_term_ref_pics_sps =
            bitstream_parser_state->sps[sps_id].num_long_term_ref_pics_sps;
        if (slice_segment_header.num_long_term_ref_pics_sps > 0) {
          // num_long_term_sps  ue(v)
          if (!bit_buffer->ReadExponentialGolomb(
              &(slice_segment_header.num_long_term_sps))) {
            return absl::nullopt;
          }
        }

        // num_long_term_pics  ue(v)
        if (!bit_buffer->ReadExponentialGolomb(
            &(slice_segment_header.num_long_term_pics))) {
          return absl::nullopt;
        }

        for (uint32_t i = 0; i < slice_segment_header.num_long_term_sps +
             slice_segment_header.num_long_term_pics; i++) {
          if (i < slice_segment_header.num_long_term_sps) {
            if (slice_segment_header.num_long_term_ref_pics_sps > 1) {
              // lt_idx_sps[i]  u(v)
              // number of bits used to represent lt_idx_sps[i] is equal to
              // Ceil(Log2(num_long_term_ref_pics_sps)).
              size_t lt_idx_sps_len =
                static_cast<size_t>(std::ceil(std::log2(
                    static_cast<float>(
                        slice_segment_header.num_long_term_ref_pics_sps))));
              if (!bit_buffer->ReadBits(&bits_tmp, lt_idx_sps_len)) {
                return absl::nullopt;
              }
              slice_segment_header.lt_idx_sps.push_back(bits_tmp);
            }

          } else {
            // poc_lsb_lt[i]  u(v)
            // length of the poc_lsb_lt[i] syntax element is
            // log2_max_pic_order_cnt_lsb_minus4 + 4 bits. [...]
            // value of lt_idx_sps[i] shall be in the range of 0 to
            // num_long_term_ref_pics_sps - 1, inclusive.
            size_t poc_lsb_lt_len =
                slice_segment_header.log2_max_pic_order_cnt_lsb_minus4 + 4;
            // slice_pic_order_cnt_lsb  u(v)
            if (!bit_buffer->ReadBits(&bits_tmp, poc_lsb_lt_len)) {
              return absl::nullopt;
            }
            slice_segment_header.poc_lsb_lt.push_back(bits_tmp);

            // used_by_curr_pic_lt_flag[i]  u(1)
            if (!bit_buffer->ReadBits(&bits_tmp, 1)) {
              return absl::nullopt;
            }
            slice_segment_header.used_by_curr_pic_lt_flag.push_back(bits_tmp);
          }

          // delta_poc_msb_present_flag[i]  u(1)
          if (!bit_buffer->ReadBits(&(bits_tmp), 1)) {
            return absl::nullopt;
          }
          slice_segment_header.delta_poc_msb_present_flag.push_back(bits_tmp);

          if (slice_segment_header.delta_poc_msb_present_flag[i]) {
            // delta_poc_msb_cycle_lt[i]  ue(v)
            if (!bit_buffer->ReadExponentialGolomb(&golomb_tmp)) {
              return absl::nullopt;
            }
            slice_segment_header.delta_poc_msb_cycle_lt.push_back(golomb_tmp);
          }
        }
      }

      slice_segment_header.sps_temporal_mvp_enabled_flag =
          bitstream_parser_state->sps[sps_id].sps_temporal_mvp_enabled_flag;
      if (slice_segment_header.sps_temporal_mvp_enabled_flag) {
        // slice_temporal_mvp_enabled_flag  u(1)
        if (!bit_buffer->ReadBits(
            &(slice_segment_header.slice_temporal_mvp_enabled_flag), 1)) {
          return absl::nullopt;
        }
      }
    }

    slice_segment_header.sample_adaptive_offset_enabled_flag =
        bitstream_parser_state->sps[sps_id].sample_adaptive_offset_enabled_flag;
    if (slice_segment_header.sample_adaptive_offset_enabled_flag) {
      // slice_sao_luma_flag  u(1)
      if (!bit_buffer->ReadBits(
          &(slice_segment_header.slice_sao_luma_flag), 1)) {
        return absl::nullopt;
      }

      // Depending on the value of separate_colour_plane_flag, the value of
      // the variable ChromaArrayType is assigned as follows:
      // - If separate_colour_plane_flag is equal to 0, ChromaArrayType is
      //   set equal to chroma_format_idc.
      // - Otherwise (separate_colour_plane_flag is equal to 1),
      //   ChromaArrayType is set equal to 0.
       uint32_t chroma_format_idc =
          bitstream_parser_state->sps[sps_id].chroma_format_idc;
      if (slice_segment_header.separate_colour_plane_flag == 0) {
        slice_segment_header.ChromaArrayType = chroma_format_idc;
      } else {
        slice_segment_header.ChromaArrayType = 0;
      }

      if (slice_segment_header.ChromaArrayType != 0) {
        // slice_sao_chroma_flag  u(1)
        if (!bit_buffer->ReadBits(
            &(slice_segment_header.slice_sao_chroma_flag), 1)) {
          return absl::nullopt;
        }
      }
    }

    if (slice_segment_header.slice_type == SliceType_P ||
        slice_segment_header.slice_type == SliceType_B) {
      // num_ref_idx_active_override_flag  u(1)
      if (!bit_buffer->ReadBits(
          &(slice_segment_header.num_ref_idx_active_override_flag), 1)) {
        return absl::nullopt;
      }

      if (slice_segment_header.num_ref_idx_active_override_flag) {
        // num_ref_idx_l0_active_minus1  ue(v)
        if (!bit_buffer->ReadExponentialGolomb(
            &(slice_segment_header.num_ref_idx_l0_active_minus1))) {
          return absl::nullopt;
        }

        if (slice_segment_header.slice_type == SliceType_B) {
          // num_ref_idx_l1_active_minus1  ue(v)
          if (!bit_buffer->ReadExponentialGolomb(
              &(slice_segment_header.num_ref_idx_l1_active_minus1))) {
            return absl::nullopt;
          }
        }
      }

      slice_segment_header.lists_modification_present_flag =
          bitstream_parser_state->pps[pps_id].
              lists_modification_present_flag;
      // TODO(chemag): calculate NumPicTotalCurr support (page 99)
      slice_segment_header.NumPicTotalCurr = 0;
      if (slice_segment_header.lists_modification_present_flag &&
          slice_segment_header.NumPicTotalCurr > 1) {
        // ref_pic_lists_modification()
        // TODO(chemag): add support for ref_pic_lists_modification()
      }

      if (slice_segment_header.slice_type == SliceType_B) {
        // mvd_l1_zero_flag  u(1)
        if (!bit_buffer->ReadBits(
            &(slice_segment_header.mvd_l1_zero_flag), 1)) {
          return absl::nullopt;
        }
      }

      slice_segment_header.cabac_init_present_flag =
          bitstream_parser_state->pps[pps_id].cabac_init_present_flag;
      if (slice_segment_header.cabac_init_present_flag) {
        // cabac_init_flag  u(1)
        if (!bit_buffer->ReadBits(
            &(slice_segment_header.cabac_init_flag), 1)) {
          return absl::nullopt;
        }
      }

      if (slice_segment_header.slice_temporal_mvp_enabled_flag) {
        if (slice_segment_header.slice_type == SliceType_B) {
          // collocated_from_l0_flag  u(1)
          if (!bit_buffer->ReadBits(
              &(slice_segment_header.collocated_from_l0_flag), 1)) {
            return absl::nullopt;
          }
        }
        if ((slice_segment_header.collocated_from_l0_flag &&
             slice_segment_header.num_ref_idx_l0_active_minus1 > 0) ||
            (!slice_segment_header.collocated_from_l0_flag &&
             slice_segment_header.num_ref_idx_l1_active_minus1 > 0)) {
          // collocated_ref_idx  ue(v)
          if (!bit_buffer->ReadExponentialGolomb(
              &(slice_segment_header.collocated_ref_idx))) {
            return absl::nullopt;
          }
        }
      }

      slice_segment_header.weighted_pred_flag =
          bitstream_parser_state->pps[pps_id].weighted_pred_flag;
      slice_segment_header.weighted_bipred_flag =
          bitstream_parser_state->pps[pps_id].weighted_bipred_flag;
      if ((slice_segment_header.weighted_pred_flag &&
           slice_segment_header.slice_type == SliceType_P) ||
          (slice_segment_header.weighted_bipred_flag &&
           slice_segment_header.slice_type == SliceType_B)) {
        // pred_weight_table()
        // TODO(chemag): add support for pred_weight_table()
      }

      // five_minus_max_num_merge_cand  ue(v)
      if (!bit_buffer->ReadExponentialGolomb(
          &(slice_segment_header.five_minus_max_num_merge_cand))) {
        return absl::nullopt;
      }

      // TODO(chemag): add support for sps_scc_extension()
      // uint32_t motion_vector_resolution_control_idc =
      //     bitstream_parser_state->sps[sps_id].
      //         sps_scc_extension.motion_vector_resolution_control_idc;
      slice_segment_header.motion_vector_resolution_control_idc = 0;
      if (slice_segment_header.motion_vector_resolution_control_idc == 2) {
        // use_integer_mv_flag  u(1)
        if (!bit_buffer->ReadBits(
            &(slice_segment_header.use_integer_mv_flag), 1)) {
          return absl::nullopt;
        }
      }
    }
    // slice_qp_delta  se(v)
    if (!bit_buffer->ReadSignedExponentialGolomb(
        &(slice_segment_header.slice_qp_delta))) {
      return absl::nullopt;
    }

    slice_segment_header.pps_slice_chroma_qp_offsets_present_flag =
        bitstream_parser_state->pps[pps_id].
            pps_slice_chroma_qp_offsets_present_flag;
    if (slice_segment_header.pps_slice_chroma_qp_offsets_present_flag) {
      // slice_cb_qp_offset  se(v)
      if (!bit_buffer->ReadSignedExponentialGolomb(
          &(slice_segment_header.slice_cb_qp_offset))) {
        return absl::nullopt;
      }

      // slice_cr_qp_offset  se(v)
      if (!bit_buffer->ReadSignedExponentialGolomb(
          &(slice_segment_header.slice_cr_qp_offset))) {
        return absl::nullopt;
      }
    }

    // TODO(chemag): add support for pps_scc_extension()
    // uint32_t pps_slice_act_qp_offsets_present_flag =
    //    bitstream_parser_state->pps[pps_id].pps_scc_extension(.
    //        pps_slice_act_qp_offsets_present_flag;
    uint32_t pps_slice_act_qp_offsets_present_flag = 0;
    if (pps_slice_act_qp_offsets_present_flag) {
      // slice_act_y_qp_offset  se(v)
      if (!bit_buffer->ReadSignedExponentialGolomb(
          &(slice_segment_header.slice_act_y_qp_offset))) {
        return absl::nullopt;
      }

      // slice_act_cb_qp_offset  se(v)
      if (!bit_buffer->ReadSignedExponentialGolomb(
          &(slice_segment_header.slice_act_cb_qp_offset))) {
        return absl::nullopt;
      }

      // slice_act_cr_qp_offset  se(v)
      if (!bit_buffer->ReadSignedExponentialGolomb(
          &(slice_segment_header.slice_act_cr_qp_offset))) {
        return absl::nullopt;
      }
    }

    // TODO(chemag): add support for pps_range_extension()
    // slice_segment_header.chroma_qp_offset_list_enabled_flag =
    //    bitstream_parser_state->pps[pps_id].pps_range_extension(.
    //        chroma_qp_offset_list_enabled_flag;
    slice_segment_header.chroma_qp_offset_list_enabled_flag = 0;
    if (slice_segment_header.chroma_qp_offset_list_enabled_flag) {
      // cu_chroma_qp_offset_enabled_flag  u(1)
      if (!bit_buffer->ReadBits(
          &(slice_segment_header.cu_chroma_qp_offset_enabled_flag), 1)) {
        return absl::nullopt;
      }
    }

    slice_segment_header.deblocking_filter_override_enabled_flag =
        bitstream_parser_state->pps[pps_id].
            deblocking_filter_override_enabled_flag;
    if (slice_segment_header.deblocking_filter_override_enabled_flag) {
      // deblocking_filter_override_flag  u(1)
      if (!bit_buffer->ReadBits(
          &(slice_segment_header.deblocking_filter_override_flag), 1)) {
        return absl::nullopt;
      }
    }

    if (slice_segment_header.deblocking_filter_override_flag) {
      // slice_deblocking_filter_disabled_flag  u(1)
      if (!bit_buffer->ReadBits(
          &(slice_segment_header.slice_deblocking_filter_disabled_flag), 1)) {
        return absl::nullopt;
      }

      if (!slice_segment_header.slice_deblocking_filter_disabled_flag) {
        // slice_beta_offset_div2 se(v)
        if (!bit_buffer->ReadSignedExponentialGolomb(
            &(slice_segment_header.slice_beta_offset_div2))) {
          return absl::nullopt;
        }

        // slice_tc_offset_div2 se(v)
        if (!bit_buffer->ReadSignedExponentialGolomb(
            &(slice_segment_header.slice_tc_offset_div2))) {
          return absl::nullopt;
        }
      }
    }

    slice_segment_header.pps_loop_filter_across_slices_enabled_flag =
        bitstream_parser_state->pps[pps_id].
            pps_loop_filter_across_slices_enabled_flag;
    if (slice_segment_header.pps_loop_filter_across_slices_enabled_flag &&
        (slice_segment_header.slice_sao_luma_flag ||
         slice_segment_header.slice_sao_chroma_flag ||
        !slice_segment_header.slice_deblocking_filter_disabled_flag)) {
      // slice_loop_filter_across_slices_enabled_flag  u(1)
      if (!bit_buffer->ReadBits(
          &(slice_segment_header.slice_loop_filter_across_slices_enabled_flag),
          1)) {
        return absl::nullopt;
      }
    }
  }

  slice_segment_header.tiles_enabled_flag =
      bitstream_parser_state->pps[pps_id].tiles_enabled_flag;
  slice_segment_header.entropy_coding_sync_enabled_flag =
      bitstream_parser_state->pps[pps_id].entropy_coding_sync_enabled_flag;
  if (slice_segment_header.tiles_enabled_flag ||
      slice_segment_header.entropy_coding_sync_enabled_flag) {
    // num_entry_point_offsets  ue(v)
    if (!bit_buffer->ReadExponentialGolomb(
        &(slice_segment_header.num_entry_point_offsets))) {
      return absl::nullopt;
    }

    if (slice_segment_header.num_entry_point_offsets > 0) {
      // offset_len_minus1  ue(v)
      if (!bit_buffer->ReadExponentialGolomb(
          &(slice_segment_header.offset_len_minus1))) {
        return absl::nullopt;
      }

      for (uint32_t i = 0; i < slice_segment_header.num_entry_point_offsets; i++) {
        // entry_point_offset_minus1[i]  u(v)
        if (!bit_buffer->ReadBits(&bits_tmp,
            slice_segment_header.offset_len_minus1)) {
          return absl::nullopt;
        }
        slice_segment_header.entry_point_offset_minus1.push_back(bits_tmp);
      }
    }
  }

  slice_segment_header.slice_segment_header_extension_present_flag =
      bitstream_parser_state->pps[pps_id].
          slice_segment_header_extension_present_flag;
  if (slice_segment_header.slice_segment_header_extension_present_flag) {
    // slice_segment_header_extension_length  ue(v)
    if (!bit_buffer->ReadExponentialGolomb(
        &(slice_segment_header.slice_segment_header_extension_length))) {
      return absl::nullopt;
    }
    for (uint32_t i = 0;
         i < slice_segment_header.slice_segment_header_extension_length;
         i++) {
      // slice_segment_header_extension_data_byte[i]  u(8)
      if (!bit_buffer->ReadBits(&bits_tmp, 8)) {
        return absl::nullopt;
      }
      slice_segment_header.slice_segment_header_extension_data_byte.push_back(
          bits_tmp);
    }
  }

  // TODO(chemag): implement byte_alignment()
  // byte_alignment()

  return OptionslSliceSegmentHeader(slice_segment_header);
}


void H265SliceSegmentHeaderParser::SliceSegmentHeaderState::fdump(
    XFILE outfp, int indent_level) const {
  XPrintf(outfp, "slice_segment_header {");
  indent_level = indent_level_incr(indent_level);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "first_slice_segment_in_pic_flag: %i",
          first_slice_segment_in_pic_flag);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "no_output_of_prior_pics_flag: %i",
          no_output_of_prior_pics_flag);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "slice_pic_parameter_set_id: %i", slice_pic_parameter_set_id);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "dependent_slice_segment_flag: %i",
          dependent_slice_segment_flag);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "slice_segment_address: %i",
          slice_segment_address);

  if (!dependent_slice_segment_flag) {
    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "slice_reserved_flag {");
    for (const uint32_t& v : slice_reserved_flag) {
      XPrintf(outfp, " %i", v);
    }
    XPrintf(outfp, " }");

    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "slice_type: %i", slice_type);

    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "pic_output_flag: %i", pic_output_flag);

    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "colour_plane_id: %i", colour_plane_id);

    if (nal_unit_type != IDR_W_RADL && nal_unit_type != IDR_N_LP) {
      fdump_indent_level(outfp, indent_level);
      XPrintf(outfp, "slice_pic_order_cnt_lsb: %i", slice_pic_order_cnt_lsb);

      fdump_indent_level(outfp, indent_level);
      XPrintf(outfp, "short_term_ref_pic_set_sps_flag: %i",
              short_term_ref_pic_set_sps_flag);

      if (!short_term_ref_pic_set_sps_flag) {
        fdump_indent_level(outfp, indent_level);
        st_ref_pic_set.fdump(outfp, indent_level);

      } else if (num_short_term_ref_pic_sets > 1) {
        fdump_indent_level(outfp, indent_level);
        XPrintf(outfp, "short_term_ref_pic_set_idx: %i",
                short_term_ref_pic_set_idx);
      }

      if (long_term_ref_pics_present_flag) {
        if (num_long_term_ref_pics_sps > 0) {
          fdump_indent_level(outfp, indent_level);
          XPrintf(outfp, "num_long_term_sps: %i", num_long_term_sps);
        }

        fdump_indent_level(outfp, indent_level);
        XPrintf(outfp, "num_long_term_pics: %i", num_long_term_pics);

        for (uint32_t i = 0; i < num_long_term_sps + num_long_term_pics; i++) {
          if (i < num_long_term_sps) {
            if (num_long_term_ref_pics_sps > 1) {
              fdump_indent_level(outfp, indent_level);
              XPrintf(outfp, "lt_idx_sps {");
              for (const uint32_t& v : lt_idx_sps) {
                XPrintf(outfp, " %i", v);
              }
              XPrintf(outfp, " }");
            }

          } else {
            fdump_indent_level(outfp, indent_level);
            XPrintf(outfp, "poc_lsb_lt {");
            for (const uint32_t& v : poc_lsb_lt) {
              XPrintf(outfp, " %i", v);
            }
            XPrintf(outfp, " }");

            fdump_indent_level(outfp, indent_level);
            XPrintf(outfp, "used_by_curr_pic_lt_flag {");
            for (const uint32_t& v : used_by_curr_pic_lt_flag) {
              XPrintf(outfp, " %i", v);
            }
            XPrintf(outfp, " }");
          }

          fdump_indent_level(outfp, indent_level);
          XPrintf(outfp, "delta_poc_msb_present_flag {");
          for (const uint32_t& v : delta_poc_msb_present_flag) {
            XPrintf(outfp, " %i", v);
          }
          XPrintf(outfp, " }");

          fdump_indent_level(outfp, indent_level);
          XPrintf(outfp, "delta_poc_msb_cycle_lt {");
          for (const uint32_t& v : delta_poc_msb_cycle_lt) {
            XPrintf(outfp, " %i", v);
          }
          XPrintf(outfp, " }");
        }
      }

      if (sps_temporal_mvp_enabled_flag) {
        fdump_indent_level(outfp, indent_level);
        XPrintf(outfp, "slice_temporal_mvp_enabled_flag: %i",
                slice_temporal_mvp_enabled_flag);
      }
    }

    if (sample_adaptive_offset_enabled_flag) {
      fdump_indent_level(outfp, indent_level);
      XPrintf(outfp, "slice_sao_luma_flag: %i", slice_sao_luma_flag);

      if (ChromaArrayType != 0) {
        fdump_indent_level(outfp, indent_level);
        XPrintf(outfp, "slice_sao_chroma_flag: %i", slice_sao_chroma_flag);
      }
    }

    if (slice_type == SliceType_P || slice_type == SliceType_B) {
      fdump_indent_level(outfp, indent_level);
      XPrintf(outfp, "num_ref_idx_active_override_flag: %i",
              num_ref_idx_active_override_flag);

      if (num_ref_idx_active_override_flag) {
        fdump_indent_level(outfp, indent_level);
        XPrintf(outfp, "num_ref_idx_l0_active_minus1: %i",
                num_ref_idx_l0_active_minus1);

        if (slice_type == SliceType_B) {
          fdump_indent_level(outfp, indent_level);
          XPrintf(outfp, "num_ref_idx_l1_active_minus1: %i",
                  num_ref_idx_l1_active_minus1);
        }
      }

      if (lists_modification_present_flag && NumPicTotalCurr > 1) {
        // TODO(chemag): add support for ref_pic_lists_modification()
      }

      if (slice_type == SliceType_B) {
        fdump_indent_level(outfp, indent_level);
        XPrintf(outfp, "mvd_l1_zero_flag: %i", mvd_l1_zero_flag);
      }

      if (cabac_init_present_flag) {
        fdump_indent_level(outfp, indent_level);
        XPrintf(outfp, "cabac_init_flag: %i", cabac_init_flag);
      }

      if (slice_temporal_mvp_enabled_flag) {
        if (slice_type == SliceType_B) {
          fdump_indent_level(outfp, indent_level);
          XPrintf(outfp, "collocated_from_l0_flag: %i",
                  collocated_from_l0_flag);
        }

        if ((collocated_from_l0_flag && num_ref_idx_l0_active_minus1 > 0) ||
            (!collocated_from_l0_flag && num_ref_idx_l1_active_minus1 > 0)) {
          fdump_indent_level(outfp, indent_level);
          XPrintf(outfp, "collocated_ref_idx: %i", collocated_ref_idx);
        }
      }

      if ((weighted_pred_flag && slice_type == SliceType_P) ||
          (weighted_bipred_flag && slice_type == SliceType_B)) {
        // TODO(chemag): add support for pred_weight_table()
      }

      fdump_indent_level(outfp, indent_level);
      XPrintf(outfp, "five_minus_max_num_merge_cand: %i",
              five_minus_max_num_merge_cand);

      if (motion_vector_resolution_control_idc == 2) {
        fdump_indent_level(outfp, indent_level);
        XPrintf(outfp, "use_integer_mv_flag: %i", use_integer_mv_flag);
      }
    }

    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "slice_qp_delta: %i", slice_qp_delta);

    if (pps_slice_chroma_qp_offsets_present_flag) {
      fdump_indent_level(outfp, indent_level);
      XPrintf(outfp, "slice_cb_qp_offset: %i", slice_cb_qp_offset);

      fdump_indent_level(outfp, indent_level);
      XPrintf(outfp, "slice_cr_qp_offset: %i", slice_cr_qp_offset);
    }

    if (pps_slice_act_qp_offsets_present_flag) {
      fdump_indent_level(outfp, indent_level);
      XPrintf(outfp, "slice_act_y_qp_offset: %i", slice_act_y_qp_offset);

      fdump_indent_level(outfp, indent_level);
      XPrintf(outfp, "slice_act_cb_qp_offset: %i", slice_act_cb_qp_offset);

      fdump_indent_level(outfp, indent_level);
      XPrintf(outfp, "slice_act_cr_qp_offset: %i", slice_act_cr_qp_offset);
    }

    if (chroma_qp_offset_list_enabled_flag) {
      fdump_indent_level(outfp, indent_level);
      XPrintf(outfp, "cu_chroma_qp_offset_enabled_flag: %i",
              cu_chroma_qp_offset_enabled_flag);
    }

    if (deblocking_filter_override_enabled_flag) {
      fdump_indent_level(outfp, indent_level);
      XPrintf(outfp, "deblocking_filter_override_flag: %i",
              deblocking_filter_override_flag);
    }

    if (deblocking_filter_override_flag) {
      fdump_indent_level(outfp, indent_level);
      XPrintf(outfp, "slice_deblocking_filter_disabled_flag: %i",
              slice_deblocking_filter_disabled_flag);

      if (!slice_deblocking_filter_disabled_flag) {
        fdump_indent_level(outfp, indent_level);
        XPrintf(outfp, "slice_beta_offset_div2: %i", slice_beta_offset_div2);

        fdump_indent_level(outfp, indent_level);
        XPrintf(outfp, "slice_tc_offset_div2: %i", slice_tc_offset_div2);
      }
    }

    if (pps_loop_filter_across_slices_enabled_flag &&
        (slice_sao_luma_flag || slice_sao_chroma_flag ||
        !slice_deblocking_filter_disabled_flag)) {
      fdump_indent_level(outfp, indent_level);
      XPrintf(outfp, "slice_loop_filter_across_slices_enabled_flag: %i",
              slice_loop_filter_across_slices_enabled_flag);
    }
  }

  if (tiles_enabled_flag || entropy_coding_sync_enabled_flag) {
    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "num_entry_point_offsets: %i", num_entry_point_offsets);

    if (num_entry_point_offsets > 0) {
      fdump_indent_level(outfp, indent_level);
      XPrintf(outfp, "offset_len_minus1: %i", offset_len_minus1);

      fdump_indent_level(outfp, indent_level);
      XPrintf(outfp, "entry_point_offset_minus1 {");
      for (const uint32_t& v : entry_point_offset_minus1) {
        XPrintf(outfp, " %i", v);
      }
      XPrintf(outfp, " }");
    }
  }

  if (slice_segment_header_extension_present_flag) {
    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "slice_segment_header_extension_length: %i",
            slice_segment_header_extension_length);

    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "slice_segment_header_extension_data_byte {");
    for (const uint32_t& v : slice_segment_header_extension_data_byte) {
      XPrintf(outfp, " %i", v);
    }
    XPrintf(outfp, " }");
  }

  // byte_alignment()

  indent_level = indent_level_decr(indent_level);
  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "}");
}

}  // namespace h265nal
