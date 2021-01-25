/*
 *  Copyright (c) Facebook, Inc. and its affiliates.
 */


#include "h265_vui_parameters_parser.h"

#include <stdio.h>

#include <cstdint>
#include <vector>

#include "h265_common.h"
#include "absl/types/optional.h"

namespace {
typedef absl::optional<h265nal::H265VuiParametersParser::
    VuiParametersState> OptionalVuiParameters;
}  // namespace

namespace h265nal {

// General note: this is based off the 2016/12 version of the H.265 standard.
// You can find it on this page:
// http://www.itu.int/rec/T-REC-H.265

// Unpack RBSP and parse VUI Parameters state from the supplied buffer.
absl::optional<H265VuiParametersParser::VuiParametersState>
H265VuiParametersParser::ParseVuiParameters(
    const uint8_t* data, size_t length) {

  std::vector<uint8_t> unpacked_buffer = UnescapeRbsp(data, length);
  rtc::BitBuffer bit_buffer(unpacked_buffer.data(), unpacked_buffer.size());
  return ParseVuiParameters(&bit_buffer);
}


absl::optional<H265VuiParametersParser::VuiParametersState>
H265VuiParametersParser::ParseVuiParameters(
    rtc::BitBuffer* bit_buffer) {
  // H265 vui_parameters() parser.
  // Section E.2.1 ("VUI parameters syntax") of the H.265 standard for
  // a complete description.
  VuiParametersState vui;

  // vui_video_parameter_set_id  u(1)
  if (!bit_buffer->ReadBits(&(vui.aspect_ratio_info_present_flag), 1)) {
    return absl::nullopt;
  }

  if (vui.aspect_ratio_info_present_flag) {
    // aspect_ratio_idc  u(8)
    if (!bit_buffer->ReadBits(&(vui.aspect_ratio_idc), 8)) {
      return absl::nullopt;
    }
    if (vui.aspect_ratio_idc == AR_EXTENDED_SAR) {
      // sar_width  u(16)
      if (!bit_buffer->ReadBits(&(vui.sar_width), 8)) {
        return absl::nullopt;
      }
      // sar_height  u(16)
      if (!bit_buffer->ReadBits(&(vui.sar_height), 8)) {
        return absl::nullopt;
      }
    }
  }

  // overscan_info_present_flag  u(1)
  if (!bit_buffer->ReadBits(&(vui.overscan_info_present_flag), 1)) {
    return absl::nullopt;
  }

  if (vui.overscan_info_present_flag) {
    // overscan_appropriate_flag  u(1)
    if (!bit_buffer->ReadBits(&(vui.overscan_appropriate_flag), 1)) {
      return absl::nullopt;
    }
  }

  // video_signal_type_present_flag  u(1)
  if (!bit_buffer->ReadBits(&(vui.video_signal_type_present_flag), 1)) {
    return absl::nullopt;
  }

  if (vui.video_signal_type_present_flag) {
    // video_format  u(3)
    if (!bit_buffer->ReadBits(&(vui.video_format), 3)) {
      return absl::nullopt;
    }
    // video_full_range_flag  u(1)
    if (!bit_buffer->ReadBits(&(vui.video_full_range_flag), 1)) {
      return absl::nullopt;
    }
    // colour_description_present_flag  u(1)
    if (!bit_buffer->ReadBits(&(vui.colour_description_present_flag), 1)) {
      return absl::nullopt;
    }
    if (vui.colour_description_present_flag) {
      // colour_primaries  u(8)
      if (!bit_buffer->ReadBits(&(vui.colour_primaries), 8)) {
        return absl::nullopt;
      }
      // transfer_characteristics  u(8)
      if (!bit_buffer->ReadBits(&(vui.transfer_characteristics), 8)) {
        return absl::nullopt;
      }
      // matrix_coeffs  u(8)
      if (!bit_buffer->ReadBits(&(vui.matrix_coeffs), 8)) {
        return absl::nullopt;
      }
    }
  }

  // chroma_loc_info_present_flag  u(1)
  if (!bit_buffer->ReadBits(&(vui.chroma_loc_info_present_flag), 1)) {
    return absl::nullopt;
  }
  if (vui.chroma_loc_info_present_flag) {
    // chroma_sample_loc_type_top_field  ue(v)
    if (!bit_buffer->ReadExponentialGolomb(
        &(vui.chroma_sample_loc_type_top_field))) {
      return absl::nullopt;
    }
    // chroma_sample_loc_type_bottom_field  ue(v)
    if (!bit_buffer->ReadExponentialGolomb(
        &(vui.chroma_sample_loc_type_bottom_field))) {
      return absl::nullopt;
    }
  }

  // neutral_chroma_indication_flag  u(1)
  if (!bit_buffer->ReadBits(&(vui.neutral_chroma_indication_flag), 1)) {
    return absl::nullopt;
  }

  // field_seq_flag  u(1)
  if (!bit_buffer->ReadBits(&(vui.field_seq_flag), 1)) {
    return absl::nullopt;
  }

  // frame_field_info_present_flag  u(1)
  if (!bit_buffer->ReadBits(&(vui.frame_field_info_present_flag), 1)) {
    return absl::nullopt;
  }

  // default_display_window_flag  u(1)
  if (!bit_buffer->ReadBits(&(vui.default_display_window_flag), 1)) {
    return absl::nullopt;
  }
  if (vui.default_display_window_flag) {
    // def_disp_win_left_offset ue(v)
    if (!bit_buffer->ReadExponentialGolomb(&(vui.def_disp_win_left_offset))) {
      return absl::nullopt;
    }
    // def_disp_win_right_offset  ue(v)
    if (!bit_buffer->ReadExponentialGolomb(&(vui.def_disp_win_right_offset))) {
      return absl::nullopt;
    }
    // def_disp_win_top_offset  ue(v)
    if (!bit_buffer->ReadExponentialGolomb(&(vui.def_disp_win_top_offset))) {
      return absl::nullopt;
    }
    // def_disp_win_bottom_offset  ue(v)
    if (!bit_buffer->ReadExponentialGolomb(&(vui.def_disp_win_bottom_offset))) {
      return absl::nullopt;
    }
  }

  // vui_timing_info_present_flag  u(1)
  if (!bit_buffer->ReadBits(&(vui.vui_timing_info_present_flag), 1)) {
    return absl::nullopt;
  }
  if (vui.vui_timing_info_present_flag) {
    // vui_num_units_in_tick  u(32)
    if (!bit_buffer->ReadBits(&(vui.vui_num_units_in_tick), 32)) {
      return absl::nullopt;
    }
    // vui_time_scale  u(32)
    if (!bit_buffer->ReadBits(&(vui.vui_time_scale), 32)) {
      return absl::nullopt;
    }
    // vui_poc_proportional_to_timing_flag  u(1)
    if (!bit_buffer->ReadBits(&(vui.vui_poc_proportional_to_timing_flag), 1)) {
      return absl::nullopt;
    }
    if (vui.vui_poc_proportional_to_timing_flag) {
      // vui_num_ticks_poc_diff_one_minus1  ue(v)
      if (!bit_buffer->ReadExponentialGolomb(
          &(vui.vui_num_ticks_poc_diff_one_minus1))) {
        return absl::nullopt;
      }
    }
    // vui_hrd_parameters_present_flag  u(1)
    if (!bit_buffer->ReadBits(&(vui.vui_hrd_parameters_present_flag), 1)) {
      return absl::nullopt;
    }
    if (vui.vui_hrd_parameters_present_flag) {
      // hrd_parameters( 1, sps_max_sub_layers_minus1 )
      // TODO(chemag): add support for hrd_parameters()
      XPrintf(stderr, "error: unimplemented hrd_parameters in vui\n");
      return absl::nullopt;
    }
  }

  // bitstream_restriction_flag  u(1)
  if (!bit_buffer->ReadBits(&(vui.bitstream_restriction_flag), 1)) {
    return absl::nullopt;
  }
  if (vui.bitstream_restriction_flag) {
    // tiles_fixed_structure_flag u(1)
    if (!bit_buffer->ReadBits(&(vui.tiles_fixed_structure_flag), 1)) {
      return absl::nullopt;
    }
    // motion_vectors_over_pic_boundaries_flag  u(1)
    if (!bit_buffer->ReadBits(
        &(vui.motion_vectors_over_pic_boundaries_flag), 1)) {
      return absl::nullopt;
    }
    // restricted_ref_pic_lists_flag  u(1)
    if (!bit_buffer->ReadBits(&(vui.restricted_ref_pic_lists_flag), 1)) {
      return absl::nullopt;
    }
    // min_spatial_segmentation_idc  ue(v)
    if (!bit_buffer->ReadExponentialGolomb(
        &(vui.min_spatial_segmentation_idc))) {
      return absl::nullopt;
    }
    // max_bytes_per_pic_denom  ue(v)
    if (!bit_buffer->ReadExponentialGolomb(&(vui.max_bytes_per_pic_denom))) {
      return absl::nullopt;
    }
    // max_bits_per_min_cu_denom  ue(v)
    if (!bit_buffer->ReadExponentialGolomb(&(vui.max_bits_per_min_cu_denom))) {
      return absl::nullopt;
    }
    // log2_max_mv_length_horizontal  ue(v)
    if (!bit_buffer->ReadExponentialGolomb(
        &(vui.log2_max_mv_length_horizontal))) {
      return absl::nullopt;
    }
    // log2_max_mv_length_vertical  ue(v)
    if (!bit_buffer->ReadExponentialGolomb(
        &(vui.log2_max_mv_length_vertical))) {
      return absl::nullopt;
    }
  }

  return OptionalVuiParameters(vui);
}

void H265VuiParametersParser::VuiParametersState::fdump(
    XFILE outfp, int indent_level) const {
  XPrintf(outfp, "vui_parameters {");
  indent_level = indent_level_incr(indent_level);
  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "aspect_ratio_info_present_flag: %i",
          aspect_ratio_info_present_flag);
  fdump_indent_level(outfp, indent_level);
  if (aspect_ratio_info_present_flag) {
    XPrintf(outfp, "aspect_ratio_idc: %i", aspect_ratio_idc);
    fdump_indent_level(outfp, indent_level);
    if (aspect_ratio_idc == AR_EXTENDED_SAR) {
    XPrintf(outfp, "sar_width: %i", sar_width);
    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "sar_height: %i", sar_height);
    fdump_indent_level(outfp, indent_level);
    }
  }
  XPrintf(outfp, "overscan_info_present_flag: %i", overscan_info_present_flag);
  fdump_indent_level(outfp, indent_level);
  if (overscan_info_present_flag) {
    XPrintf(outfp, "overscan_appropriate_flag: %i", overscan_appropriate_flag);
    fdump_indent_level(outfp, indent_level);
  }
  XPrintf(outfp, "video_signal_type_present_flag: %i",
          video_signal_type_present_flag);
  fdump_indent_level(outfp, indent_level);
  if (video_signal_type_present_flag) {
    XPrintf(outfp, "video_format: %i", video_format);
    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "video_full_range_flag: %i", video_full_range_flag);
    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "colour_description_present_flag: %i",
            colour_description_present_flag);
    fdump_indent_level(outfp, indent_level);
    if (colour_description_present_flag) {
      XPrintf(outfp, "colour_primaries: %i", colour_primaries);
      fdump_indent_level(outfp, indent_level);
      XPrintf(outfp, "transfer_characteristics: %i", transfer_characteristics);
      fdump_indent_level(outfp, indent_level);
      XPrintf(outfp, "matrix_coeffs: %i", matrix_coeffs);
      fdump_indent_level(outfp, indent_level);
    }
  }
  XPrintf(outfp, "chroma_loc_info_present_flag: %i",
          chroma_loc_info_present_flag);
  fdump_indent_level(outfp, indent_level);
  if (chroma_loc_info_present_flag) {
    XPrintf(outfp, "chroma_sample_loc_type_top_field: %i",
            chroma_sample_loc_type_top_field);
    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "chroma_sample_loc_type_bottom_field: %i",
            chroma_sample_loc_type_bottom_field);
    fdump_indent_level(outfp, indent_level);
  }
  XPrintf(outfp, "neutral_chroma_indication_flag: %i",
          neutral_chroma_indication_flag);
  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "field_seq_flag: %i", field_seq_flag);
  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "frame_field_info_present_flag: %i",
          frame_field_info_present_flag);
  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "default_display_window_flag: %i",
          default_display_window_flag);
  fdump_indent_level(outfp, indent_level);

  if (default_display_window_flag) {
    XPrintf(outfp, "def_disp_win_left_offset: %i", def_disp_win_left_offset);
    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "def_disp_win_right_offset: %i", def_disp_win_right_offset);
    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "def_disp_win_top_offset: %i", def_disp_win_top_offset);
    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "def_disp_win_bottom_offset: %i",
            def_disp_win_bottom_offset);
    fdump_indent_level(outfp, indent_level);
  }
  XPrintf(outfp, "vui_timing_info_present_flag: %i",
          vui_timing_info_present_flag);
  fdump_indent_level(outfp, indent_level);
  if (vui_timing_info_present_flag) {
    XPrintf(outfp, "vui_num_units_in_tick: %i", vui_num_units_in_tick);
    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "vui_time_scale: %i", vui_time_scale);
    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "vui_poc_proportional_to_timing_flag: %i",
            vui_poc_proportional_to_timing_flag);
    fdump_indent_level(outfp, indent_level);
    if (vui_poc_proportional_to_timing_flag) {
      XPrintf(outfp, "vui_num_ticks_poc_diff_one_minus1: %i",
              vui_num_ticks_poc_diff_one_minus1);
      fdump_indent_level(outfp, indent_level);
    }
    XPrintf(outfp, "vui_hrd_parameters_present_flag: %i",
            vui_hrd_parameters_present_flag);
    fdump_indent_level(outfp, indent_level);
    if (vui_hrd_parameters_present_flag) {
      // hrd_parameters( 1, sps_max_sub_layers_minus1 )
      // TODO(chemag): add support for hrd_parameters()
    }
  }
  XPrintf(outfp, "bitstream_restriction_flag: %i", bitstream_restriction_flag);
  fdump_indent_level(outfp, indent_level);
  if (bitstream_restriction_flag) {
    XPrintf(outfp, "tiles_fixed_structure_flag: %i",
            tiles_fixed_structure_flag);
    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "motion_vectors_over_pic_boundaries_flag: %i",
            motion_vectors_over_pic_boundaries_flag);
    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "restricted_ref_pic_lists_flag: %i",
            restricted_ref_pic_lists_flag);
    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "min_spatial_segmentation_idc: %i",
            min_spatial_segmentation_idc);
    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "max_bytes_per_pic_denom: %i", max_bytes_per_pic_denom);
    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "max_bits_per_min_cu_denom: %i", max_bits_per_min_cu_denom);
    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "log2_max_mv_length_horizontal: %i",
            log2_max_mv_length_horizontal);
    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "log2_max_mv_length_vertical: %i",
            log2_max_mv_length_vertical);
    fdump_indent_level(outfp, indent_level);
  }

  indent_level = indent_level_decr(indent_level);
  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "}");
}

}  // namespace h265nal
