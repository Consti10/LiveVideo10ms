/*
 *  Copyright (c) Facebook, Inc. and its affiliates.
 */


#include "h265_sps_parser.h"

#include <stdio.h>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <cmath>
#include <vector>

#include "h265_common.h"
#include "h265_profile_tier_level_parser.h"
#include "h265_vui_parameters_parser.h"
#include "absl/types/optional.h"

namespace {
typedef absl::optional<h265nal::H265ProfileTierLevelParser::
    ProfileTierLevelState> OptionalProfileTierLevel;
typedef absl::optional<h265nal::H265StRefPicSetParser::
    StRefPicSetState> OptionalStRefPicSet;
typedef absl::optional<h265nal::H265VuiParametersParser::
    VuiParametersState> OptionalVuiParameters;
typedef absl::optional<h265nal::H265SpsParser::
    SpsState> OptionalSps;
}  // namespace

namespace h265nal {

// General note: this is based off the 2016/12 version of the H.265 standard.
// You can find it on this page:
// http://www.itu.int/rec/T-REC-H.265

// Unpack RBSP and parse SPS state from the supplied buffer.
absl::optional<H265SpsParser::SpsState> H265SpsParser::ParseSps(
    const uint8_t* data, size_t length) {

  std::vector<uint8_t> unpacked_buffer = UnescapeRbsp(data, length);
  rtc::BitBuffer bit_buffer(unpacked_buffer.data(), unpacked_buffer.size());
  return ParseSps(&bit_buffer);
}


absl::optional<H265SpsParser::SpsState> H265SpsParser::ParseSps(
    rtc::BitBuffer* bit_buffer) {
  uint32_t bits_tmp;
  uint32_t golomb_tmp;

  // H265 SPS Nal Unit (seq_parameter_set_rbsp()) parser.
  // Section 7.3.2.2 ("Sequence parameter set data syntax") of the H.265
  // standard for a complete description.
  SpsState sps;

  // sps_video_parameter_set_id  u(4)
  if (!bit_buffer->ReadBits(&(sps.sps_video_parameter_set_id), 4)) {
    return absl::nullopt;
  }

  // sps_max_sub_layers_minus1  u(3)
  if (!bit_buffer->ReadBits(&(sps.sps_max_sub_layers_minus1), 3)) {
    return absl::nullopt;
  }

  // sps_temporal_id_nesting_flag  u(1)
  if (!bit_buffer->ReadBits(&(sps.sps_temporal_id_nesting_flag), 1)) {
    return absl::nullopt;
  }

  // profile_tier_level(1, sps_max_sub_layers_minus1)
  OptionalProfileTierLevel profile_tier_level =
      H265ProfileTierLevelParser::ParseProfileTierLevel(
          bit_buffer, true, sps.sps_max_sub_layers_minus1);
  if (profile_tier_level != absl::nullopt) {
    sps.profile_tier_level = *profile_tier_level;
  }

  // sps_seq_parameter_set_id  ue(v)
  if (!bit_buffer->ReadExponentialGolomb(&(sps.sps_seq_parameter_set_id))) {
    return absl::nullopt;
  }

  // chroma_format_idc  ue(v)
  if (!bit_buffer->ReadExponentialGolomb(&(sps.chroma_format_idc))) {
    return absl::nullopt;
  }
  if (sps.chroma_format_idc == 3) {
    // separate_colour_plane_flag  u(1)
    if (!bit_buffer->ReadBits(&(sps.separate_colour_plane_flag), 1)) {
      return absl::nullopt;
    }
  }

  // pic_width_in_luma_samples  ue(v)
  if (!bit_buffer->ReadExponentialGolomb(&(sps.pic_width_in_luma_samples))) {
    return absl::nullopt;
  }

  // pic_height_in_luma_samples  ue(v)
  if (!bit_buffer->ReadExponentialGolomb(&(sps.pic_height_in_luma_samples))) {
    return absl::nullopt;
  }

  // conformance_window_flag  u(1)
  if (!bit_buffer->ReadBits(&(sps.conformance_window_flag), 1)) {
     return absl::nullopt;
  }

  if (sps.conformance_window_flag) {
    // conf_win_left_offset  ue(v)
    if (!bit_buffer->ReadExponentialGolomb(&(sps.conf_win_left_offset))) {
      return absl::nullopt;
    }
    // conf_win_right_offset  ue(v)
    if (!bit_buffer->ReadExponentialGolomb(&(sps.conf_win_right_offset))) {
      return absl::nullopt;
    }
    // conf_win_top_offset  ue(v)
    if (!bit_buffer->ReadExponentialGolomb(&(sps.conf_win_top_offset))) {
      return absl::nullopt;
    }
    // conf_win_bottom_offset  ue(v)
    if (!bit_buffer->ReadExponentialGolomb(&(sps.conf_win_bottom_offset))) {
      return absl::nullopt;
    }
  }

  // bit_depth_luma_minus8  ue(v)
  if (!bit_buffer->ReadExponentialGolomb(&(sps.bit_depth_luma_minus8))) {
    return absl::nullopt;
  }
  // bit_depth_chroma_minus8  ue(v)
  if (!bit_buffer->ReadExponentialGolomb(&(sps.bit_depth_chroma_minus8))) {
    return absl::nullopt;
  }
  // log2_max_pic_order_cnt_lsb_minus4  ue(v)
  if (!bit_buffer->ReadExponentialGolomb(
      &(sps.log2_max_pic_order_cnt_lsb_minus4))) {
    return absl::nullopt;
  }
  // sps_sub_layer_ordering_info_present_flag  u(1)
  if (!bit_buffer->ReadBits(
      &(sps.sps_sub_layer_ordering_info_present_flag), 1)) {
     return absl::nullopt;
  }

  for (uint32_t i = (sps.sps_sub_layer_ordering_info_present_flag ? 0 :
                sps.sps_max_sub_layers_minus1);
       i <= sps.sps_max_sub_layers_minus1; i++) {
    // sps_max_dec_pic_buffering_minus1[i]  ue(v)
    if (!bit_buffer->ReadExponentialGolomb(&golomb_tmp)) {
      return absl::nullopt;
    }
    sps.sps_max_dec_pic_buffering_minus1.push_back(golomb_tmp);
    // sps_max_num_reorder_pics[i]  ue(v)
    if (!bit_buffer->ReadExponentialGolomb(&golomb_tmp)) {
      return absl::nullopt;
    }
    sps.sps_max_num_reorder_pics.push_back(golomb_tmp);
    // sps_max_latency_increase_plus1[i]  ue(v)
    if (!bit_buffer->ReadExponentialGolomb(&golomb_tmp)) {
      return absl::nullopt;
    }
    sps.sps_max_latency_increase_plus1.push_back(golomb_tmp);
  }

  // log2_min_luma_coding_block_size_minus3  ue(v)
  if (!bit_buffer->ReadExponentialGolomb(
      &(sps.log2_min_luma_coding_block_size_minus3))) {
    return absl::nullopt;
  }

  // log2_diff_max_min_luma_coding_block_size  ue(v)
  if (!bit_buffer->ReadExponentialGolomb(
      &(sps.log2_diff_max_min_luma_coding_block_size))) {
    return absl::nullopt;
  }

  // log2_min_luma_transform_block_size_minus2  ue(v)
  if (!bit_buffer->ReadExponentialGolomb(
      &(sps.log2_min_luma_transform_block_size_minus2))) {
    return absl::nullopt;
  }

  // log2_diff_max_min_luma_transform_block_size  ue(v)
  if (!bit_buffer->ReadExponentialGolomb(
      &(sps.log2_diff_max_min_luma_transform_block_size))) {
    return absl::nullopt;
  }

  // max_transform_hierarchy_depth_inter  ue(v)
  if (!bit_buffer->ReadExponentialGolomb(
      &(sps.max_transform_hierarchy_depth_inter))) {
    return absl::nullopt;
  }

  // max_transform_hierarchy_depth_intra  ue(v)
  if (!bit_buffer->ReadExponentialGolomb(
      &(sps.max_transform_hierarchy_depth_intra))) {
    return absl::nullopt;
  }

  // scaling_list_enabled_flag  u(1)
  if (!bit_buffer->ReadBits(&(sps.scaling_list_enabled_flag), 1)) {
     return absl::nullopt;
  }

  if (sps.scaling_list_enabled_flag) {
    // sps_scaling_list_data_present_flag  u(1)
    if (!bit_buffer->ReadBits(
        &(sps.sps_scaling_list_data_present_flag), 1)) {
      return absl::nullopt;
    }
    if (sps.sps_scaling_list_data_present_flag) {
      // scaling_list_data()
      // TODO(chemag): add support for scaling_list_data()
      XPrintf(stderr, "error: unimplemented scaling_list_data() in sps\n");
      return absl::nullopt;
    }
  }

  // amp_enabled_flag  u(1)
  if (!bit_buffer->ReadBits(&(sps.amp_enabled_flag), 1)) {
    return absl::nullopt;
  }

  // sample_adaptive_offset_enabled_flag  u(1)
  if (!bit_buffer->ReadBits(&(sps.sample_adaptive_offset_enabled_flag), 1)) {
    return absl::nullopt;
  }

  // pcm_enabled_flag  u(1)
  if (!bit_buffer->ReadBits(&(sps.pcm_enabled_flag), 1)) {
    return absl::nullopt;
  }

  if (sps.pcm_enabled_flag) {
    // pcm_sample_bit_depth_luma_minus1  u(4)
    if (!bit_buffer->ReadBits(&(sps.pcm_sample_bit_depth_luma_minus1), 4)) {
      return absl::nullopt;
    }

    // pcm_sample_bit_depth_chroma_minus1  u(4)
    if (!bit_buffer->ReadBits(&(sps.pcm_sample_bit_depth_chroma_minus1), 4)) {
      return absl::nullopt;
    }

    // log2_min_pcm_luma_coding_block_size_minus3  ue(v)
    if (!bit_buffer->ReadExponentialGolomb(
        &(sps.log2_min_pcm_luma_coding_block_size_minus3))) {
      return absl::nullopt;
    }

    // log2_diff_max_min_pcm_luma_coding_block_size  ue(v)
    if (!bit_buffer->ReadExponentialGolomb(
        &(sps.log2_diff_max_min_pcm_luma_coding_block_size))) {
      return absl::nullopt;
    }

    // pcm_loop_filter_disabled_flag  u(1)
    if (!bit_buffer->ReadBits(&(sps.pcm_loop_filter_disabled_flag), 1)) {
      return absl::nullopt;
    }
  }

  // num_short_term_ref_pic_sets
  if (!bit_buffer->ReadExponentialGolomb(&(sps.num_short_term_ref_pic_sets))) {
    return absl::nullopt;
  }

  for (uint32_t i = 0; i < sps.num_short_term_ref_pic_sets; i++) {
    // st_ref_pic_set(i)
    OptionalStRefPicSet st_ref_pic_set =
        H265StRefPicSetParser::ParseStRefPicSet(
            bit_buffer, i, sps.num_short_term_ref_pic_sets);
    if (st_ref_pic_set != absl::nullopt) {
      sps.st_ref_pic_set.push_back(*st_ref_pic_set);
    }
  }

  // long_term_ref_pics_present_flag  u(1)
  if (!bit_buffer->ReadBits(&(sps.long_term_ref_pics_present_flag), 1)) {
    return absl::nullopt;
  }

  if (sps.long_term_ref_pics_present_flag) {
    // num_long_term_ref_pics_sps  ue(v)
    if (!bit_buffer->ReadExponentialGolomb(&(sps.num_long_term_ref_pics_sps))) {
      return absl::nullopt;
    }

    for (uint32_t i = 0; i < sps.num_long_term_ref_pics_sps; i++) {
      // lt_ref_pic_poc_lsb_sps[i] u(v)  log2_max_pic_order_cnt_lsb_minus4 + 4
      if (!bit_buffer->ReadBits(&bits_tmp,
          sps.log2_max_pic_order_cnt_lsb_minus4 + 4)) {
        return absl::nullopt;
      }
      sps.lt_ref_pic_poc_lsb_sps.push_back(bits_tmp);

      // used_by_curr_pic_lt_sps_flag[i]  u(1)
      if (!bit_buffer->ReadBits(&bits_tmp, 1)) {
        return absl::nullopt;
      }
      sps.used_by_curr_pic_lt_sps_flag.push_back(bits_tmp);
    }
  }

  // sps_temporal_mvp_enabled_flag  u(1)
  if (!bit_buffer->ReadBits(&(sps.sps_temporal_mvp_enabled_flag), 1)) {
    return absl::nullopt;
  }

  // strong_intra_smoothing_enabled_flag  u(1)
  if (!bit_buffer->ReadBits(&(sps.strong_intra_smoothing_enabled_flag), 1)) {
    return absl::nullopt;
  }

  // vui_parameters_present_flag  u(1)
  if (!bit_buffer->ReadBits(&(sps.vui_parameters_present_flag), 1)) {
    return absl::nullopt;
  }

  if (sps.vui_parameters_present_flag) {
    // vui_parameters()
    OptionalVuiParameters vui_parameters =
        H265VuiParametersParser::ParseVuiParameters(bit_buffer);
    if (vui_parameters != absl::nullopt) {
      sps.vui_parameters = *vui_parameters;
    }
  }

  // sps_extension_present_flag  u(1)
  if (!bit_buffer->ReadBits(&(sps.sps_extension_present_flag), 1)) {
    return absl::nullopt;
  }

  if (sps.sps_extension_present_flag) {
    // sps_range_extension_flag  u(1)
    if (!bit_buffer->ReadBits(&(sps.sps_range_extension_flag), 1)) {
      return absl::nullopt;
    }

    // sps_multilayer_extension_flag  u(1)
    if (!bit_buffer->ReadBits(&(sps.sps_multilayer_extension_flag), 1)) {
      return absl::nullopt;
    }

    // sps_3d_extension_flag  u(1)
    if (!bit_buffer->ReadBits(&(sps.sps_3d_extension_flag), 1)) {
      return absl::nullopt;
    }

    // sps_scc_extension_flag  u(1)
    if (!bit_buffer->ReadBits(&(sps.sps_scc_extension_flag), 1)) {
      return absl::nullopt;
    }

    // sps_extension_4bits  u(4)
    if (!bit_buffer->ReadBits(&(sps.sps_extension_4bits), 4)) {
      return absl::nullopt;
    }
  }

  if (sps.sps_range_extension_flag) {
    // sps_range_extension()
    // TODO(chemag): add support for sps_range_extension()
    XPrintf(stderr, "error: unimplemented sps_range_extension() in sps\n");
    return absl::nullopt;
  }
  if (sps.sps_multilayer_extension_flag) {
    // sps_multilayer_extension() // specified in Annex F
    // TODO(chemag): add support for sps_multilayer_extension()
    XPrintf(stderr, "error: unimplemented sps_multilayer_extension() in sps\n");
    return absl::nullopt;
  }

  if (sps.sps_3d_extension_flag) {
    // sps_3d_extension() // specified in Annex I
    // TODO(chemag): add support for sps_3d_extension()
    XPrintf(stderr, "error: unimplemented sps_3d_extension() in sps\n");
    return absl::nullopt;
  }

  if (sps.sps_scc_extension_flag) {
    // sps_scc_extension()
    // TODO(chemag): add support for sps_scc_extension()
    XPrintf(stderr, "error: unimplemented sps_scc_extension() in sps\n");
    return absl::nullopt;
  }

  if (sps.sps_extension_4bits) {
    while (more_rbsp_data(bit_buffer)) {
      // sps_extension_data_flag  u(1)
      if (!bit_buffer->ReadBits(&(sps.sps_extension_data_flag), 1)) {
        return absl::nullopt;
      }
    }
  }

  rbsp_trailing_bits(bit_buffer);

  return OptionalSps(sps);
}

void H265SpsParser::SpsState::fdump(XFILE outfp, int indent_level) const {
  XPrintf(outfp, "sps {");
  indent_level = indent_level_incr(indent_level);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "sps_video_parameter_set_id: %i", sps_video_parameter_set_id);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "sps_max_sub_layers_minus1: %i", sps_max_sub_layers_minus1);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "sps_temporal_id_nesting_flag: %i",
          sps_temporal_id_nesting_flag);

  fdump_indent_level(outfp, indent_level);
  profile_tier_level.fdump(outfp, indent_level);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "sps_seq_parameter_set_id: %i", sps_seq_parameter_set_id);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "chroma_format_idc: %i", chroma_format_idc);

  if (chroma_format_idc == 3) {
    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "separate_colour_plane_flag: %i",
            separate_colour_plane_flag);
  }

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "pic_width_in_luma_samples: %i", pic_width_in_luma_samples);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "pic_height_in_luma_samples: %i", pic_height_in_luma_samples);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "conformance_window_flag: %i", conformance_window_flag);

  if (conformance_window_flag) {
    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "conf_win_left_offset: %i", conf_win_left_offset);

    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "conf_win_right_offset: %i", conf_win_right_offset);

    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "conf_win_top_offset: %i", conf_win_top_offset);

    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "conf_win_bottom_offset: %i", conf_win_bottom_offset);
  }

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "bit_depth_luma_minus8: %i", bit_depth_luma_minus8);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "bit_depth_chroma_minus8: %i", bit_depth_chroma_minus8);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "log2_max_pic_order_cnt_lsb_minus4: %i",
          log2_max_pic_order_cnt_lsb_minus4);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "sps_sub_layer_ordering_info_present_flag: %i",
          sps_sub_layer_ordering_info_present_flag);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "sps_max_dec_pic_buffering_minus1 {");
  for (const uint32_t& v : sps_max_dec_pic_buffering_minus1) {
    XPrintf(outfp, " %i", v);
  }
  XPrintf(outfp, " }");

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "sps_max_num_reorder_pics {");
  for (const uint32_t& v : sps_max_num_reorder_pics) {
    XPrintf(outfp, " %i", v);
  }
  XPrintf(outfp, " }");

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "sps_max_latency_increase_plus1 {");
  for (const uint32_t& v : sps_max_latency_increase_plus1) {
    XPrintf(outfp, " %i", v);
  }
  XPrintf(outfp, " }");

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "log2_min_luma_coding_block_size_minus3: %i",
          log2_min_luma_coding_block_size_minus3);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "log2_diff_max_min_luma_coding_block_size: %i",
          log2_diff_max_min_luma_coding_block_size);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "log2_min_luma_transform_block_size_minus2: %i",
          log2_min_luma_transform_block_size_minus2);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "log2_diff_max_min_luma_transform_block_size: %i",
          log2_diff_max_min_luma_transform_block_size);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "max_transform_hierarchy_depth_inter: %i",
          max_transform_hierarchy_depth_inter);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "max_transform_hierarchy_depth_intra: %i",
          max_transform_hierarchy_depth_intra);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "scaling_list_enabled_flag: %i",
          scaling_list_enabled_flag);

  if (scaling_list_enabled_flag) {
    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "sps_scaling_list_data_present_flag: %i",
            sps_scaling_list_data_present_flag);

    if (sps_scaling_list_data_present_flag) {
      // scaling_list_data()
      // TODO(chemag): add support for scaling_list_data()
      XPrintf(stderr, "error: unimplemented scaling_list_data() in sps\n");
    }
  }

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "amp_enabled_flag: %i", amp_enabled_flag);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "sample_adaptive_offset_enabled_flag: %i",
          sample_adaptive_offset_enabled_flag);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "pcm_enabled_flag: %i", pcm_enabled_flag);

  if (pcm_enabled_flag) {
    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "pcm_sample_bit_depth_luma_minus1: %i",
            pcm_sample_bit_depth_luma_minus1);

    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "pcm_sample_bit_depth_chroma_minus1: %i",
            pcm_sample_bit_depth_chroma_minus1);

    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "log2_min_pcm_luma_coding_block_size_minus3: %i",
            log2_min_pcm_luma_coding_block_size_minus3);

    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "log2_diff_max_min_pcm_luma_coding_block_size: %i",
            log2_diff_max_min_pcm_luma_coding_block_size);

    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "pcm_loop_filter_disabled_flag: %i",
            pcm_loop_filter_disabled_flag);
  }

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "num_short_term_ref_pic_sets: %i",
          num_short_term_ref_pic_sets);

  for (uint32_t i = 0; i < num_short_term_ref_pic_sets; i++) {
    fdump_indent_level(outfp, indent_level);
    st_ref_pic_set[i].fdump(outfp, indent_level);
  }

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "long_term_ref_pics_present_flag: %i",
          long_term_ref_pics_present_flag);

  if (long_term_ref_pics_present_flag) {
    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "num_long_term_ref_pics_sps: %i",
            num_long_term_ref_pics_sps);

    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "lt_ref_pic_poc_lsb_sps {");
    for (const uint32_t& v : lt_ref_pic_poc_lsb_sps) {
      XPrintf(outfp, " %i", v);
    }
    XPrintf(outfp, " }");

    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "used_by_curr_pic_lt_sps_flag {");
    for (const uint32_t& v : used_by_curr_pic_lt_sps_flag) {
      XPrintf(outfp, " %i", v);
    }
    XPrintf(outfp, " }");
  }

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "sps_temporal_mvp_enabled_flag: %i",
          sps_temporal_mvp_enabled_flag);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "strong_intra_smoothing_enabled_flag: %i",
          strong_intra_smoothing_enabled_flag);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "vui_parameters_present_flag: %i",
          vui_parameters_present_flag);

  fdump_indent_level(outfp, indent_level);
  if (vui_parameters_present_flag) {
    vui_parameters.fdump(outfp, indent_level);
  }

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "sps_extension_present_flag: %i",
          sps_extension_present_flag);

  if (sps_extension_present_flag) {
    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "sps_range_extension_flag: %i", sps_range_extension_flag);

    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "sps_multilayer_extension_flag: %i",
            sps_multilayer_extension_flag);

    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "sps_3d_extension_flag: %i", sps_3d_extension_flag);

    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "sps_scc_extension_flag: %i", sps_scc_extension_flag);

    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "sps_extension_4bits: %i", sps_extension_4bits);
  }

  if (sps_range_extension_flag) {
    // sps_range_extension()
    // TODO(chemag): add support for sps_range_extension()
    XPrintf(stderr, "error: unimplemented sps_range_extension() in sps\n");
  }

  if (sps_multilayer_extension_flag) {
    // sps_multilayer_extension() // specified in Annex F
    // TODO(chemag): add support for sps_multilayer_extension()
    XPrintf(stderr, "error: unimplemented sps_multilayer_extension_flag() in "
            "sps\n");
  }

  if (sps_3d_extension_flag) {
    // sps_3d_extension() // specified in Annex I
    // TODO(chemag): add support for sps_3d_extension()
    XPrintf(stderr, "error: unimplemented sps_3d_extension_flag() in sps\n");
  }

  if (sps_scc_extension_flag) {
    // sps_scc_extension()
    // TODO(chemag): add support for sps_scc_extension()
    XPrintf(stderr, "error: unimplemented sps_scc_extension_flag() in sps\n");
  }

  indent_level = indent_level_decr(indent_level);
  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "}");
}

uint32_t H265SpsParser::SpsState::getPicSizeInCtbsY() {
  // PicSizeInCtbsY support (page 77)
  // Equation (7-10)
  uint32_t MinCbLog2SizeY = log2_min_luma_coding_block_size_minus3 + 3;
  // Equation (7-11)
  uint32_t CtbLog2SizeY = MinCbLog2SizeY +
      log2_diff_max_min_luma_coding_block_size;
  // Equation (7-12)
  // uint32_t MinCbSizeY = 1 << MinCbLog2SizeY;
  // Equation (7-13)
  uint32_t CtbSizeY = 1 << CtbLog2SizeY;
  // Equation (7-14)
  // uint32_t PicWidthInMinCbsY = pic_width_in_luma_samples / MinCbSizeY;
  // Equation (7-15)
  uint32_t PicWidthInCtbsY = static_cast<uint32_t>(std::ceil(
      pic_width_in_luma_samples / CtbSizeY));
  // Equation (7-16)
  // uint32_t PicHeightInMinCbsY = pic_height_in_luma_samples / MinCbSizeY;
  // Equation (7-17)
  uint32_t PicHeightInCtbsY = static_cast<uint32_t>(std::ceil(
      pic_height_in_luma_samples / CtbSizeY));
  // Equation (7-18)
  // uint32_t PicSizeInMinCbsY = PicWidthInMinCbsY * PicHeightInMinCbsY;
  // Equation (7-19)
  uint32_t PicSizeInCtbsY = PicWidthInCtbsY * PicHeightInCtbsY;
  // PicSizeInSamplesY = pic_width_in_luma_samples *
  //     pic_height_in_luma_samples (7-20)
  // PicWidthInSamplesC = pic_width_in_luma_samples / SubWidthC (7-21)
  // PicHeightInSamplesC = pic_height_in_luma_samples / SubHeightC (7-22)
  return PicSizeInCtbsY;
}



}  // namespace h265nal
