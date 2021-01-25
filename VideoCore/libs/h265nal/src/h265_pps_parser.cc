/*
 *  Copyright (c) Facebook, Inc. and its affiliates.
 */


#include "h265_pps_parser.h"

#include <stdio.h>

#include <cstdint>
#include <vector>

#include "h265_common.h"
#include "h265_profile_tier_level_parser.h"
#include "h265_vui_parameters_parser.h"
#include "absl/types/optional.h"

namespace {
typedef absl::optional<h265nal::H265ProfileTierLevelParser::
    ProfileTierLevelState> OptionalProfileTierLevel;
typedef absl::optional<h265nal::H265VuiParametersParser::
    VuiParametersState> OptionalVuiParameters;
typedef absl::optional<h265nal::H265PpsParser::
    PpsState> OptionalPps;
}  // namespace

namespace h265nal {

// General note: this is based off the 2016/12 version of the H.265 standard.
// You can find it on this page:
// http://www.itu.int/rec/T-REC-H.265

// Unpack RBSP and parse PPS state from the supplied buffer.
absl::optional<H265PpsParser::PpsState> H265PpsParser::ParsePps(
    const uint8_t* data, size_t length) {

  std::vector<uint8_t> unpacked_buffer = UnescapeRbsp(data, length);
  rtc::BitBuffer bit_buffer(unpacked_buffer.data(), unpacked_buffer.size());
  return ParsePps(&bit_buffer);
}


absl::optional<H265PpsParser::PpsState> H265PpsParser::ParsePps(
    rtc::BitBuffer* bit_buffer) {
  uint32_t golomb_tmp;

  // H265 PPS NAL Unit (pic_parameter_set_rbsp()) parser.
  // Section 7.3.2.3 ("Parameter parameter set data syntax") of the H.265
  // standard for a complete description.
  PpsState pps;

  // pps_pic_parameter_set_id  ue(v)
  if (!bit_buffer->ReadExponentialGolomb(&(pps.pps_pic_parameter_set_id))) {
    return absl::nullopt;
  }

  // pps_seq_parameter_set_id  ue(v)
  if (!bit_buffer->ReadExponentialGolomb(&(pps.pps_seq_parameter_set_id))) {
    return absl::nullopt;
  }

  // dependent_slice_segments_enabled_flag  u(1)
  if (!bit_buffer->ReadBits(&(pps.dependent_slice_segments_enabled_flag), 1)) {
    return absl::nullopt;
  }

  // output_flag_present_flag  u(1)
  if (!bit_buffer->ReadBits(&(pps.output_flag_present_flag), 1)) {
    return absl::nullopt;
  }

  // num_extra_slice_header_bits  u(3)
  if (!bit_buffer->ReadBits(&(pps.num_extra_slice_header_bits), 3)) {
    return absl::nullopt;
  }

  // sign_data_hiding_enabled_flag  u(1)
  if (!bit_buffer->ReadBits(&(pps.sign_data_hiding_enabled_flag), 1)) {
    return absl::nullopt;
  }

  // cabac_init_present_flag  u(1)
  if (!bit_buffer->ReadBits(&(pps.cabac_init_present_flag), 1)) {
    return absl::nullopt;
  }

  // num_ref_idx_l0_default_active_minus1  ue(v)
  if (!bit_buffer->ReadExponentialGolomb(
      &(pps.num_ref_idx_l0_default_active_minus1))) {
    return absl::nullopt;
  }

  // num_ref_idx_l1_default_active_minus1  ue(v)
  if (!bit_buffer->ReadExponentialGolomb(
      &(pps.num_ref_idx_l1_default_active_minus1))) {
    return absl::nullopt;
  }

  // init_qp_minus26  se(v)
  if (!bit_buffer->ReadExponentialGolomb(&(pps.init_qp_minus26))) {
    return absl::nullopt;
  }

  // constrained_intra_pred_flag  u(1)
  if (!bit_buffer->ReadBits(&(pps.constrained_intra_pred_flag), 1)) {
    return absl::nullopt;
  }

  // transform_skip_enabled_flag  u(1)
  if (!bit_buffer->ReadBits(&(pps.transform_skip_enabled_flag), 1)) {
    return absl::nullopt;
  }

  // cu_qp_delta_enabled_flag  u(1)
  if (!bit_buffer->ReadBits(&(pps.cu_qp_delta_enabled_flag), 1)) {
    return absl::nullopt;
  }

  if (pps.cu_qp_delta_enabled_flag) {
    // diff_cu_qp_delta_depth  ue(v)
    if (!bit_buffer->ReadExponentialGolomb(&(pps.diff_cu_qp_delta_depth))) {
      return absl::nullopt;
    }
  }

  // pps_cb_qp_offset  se(v)
  if (!bit_buffer->ReadSignedExponentialGolomb(&(pps.pps_cb_qp_offset))) {
    return absl::nullopt;
  }

  // pps_cr_qp_offset  se(v)
  if (!bit_buffer->ReadSignedExponentialGolomb(&(pps.pps_cr_qp_offset))) {
    return absl::nullopt;
  }

  // pps_slice_chroma_qp_offsets_present_flag  u(1)
  if (!bit_buffer->ReadBits(
      &(pps.pps_slice_chroma_qp_offsets_present_flag), 1)) {
    return absl::nullopt;
  }

  // weighted_pred_flag  u(1)
  if (!bit_buffer->ReadBits(&(pps.weighted_pred_flag), 1)) {
    return absl::nullopt;
  }

  // weighted_bipred_flag  u(1)
  if (!bit_buffer->ReadBits(&(pps.weighted_bipred_flag), 1)) {
    return absl::nullopt;
  }

  // transquant_bypass_enabled_flag  u(1)
  if (!bit_buffer->ReadBits(&(pps.transquant_bypass_enabled_flag), 1)) {
    return absl::nullopt;
  }

  // tiles_enabled_flag  u(1)
  if (!bit_buffer->ReadBits(&(pps.tiles_enabled_flag), 1)) {
    return absl::nullopt;
  }

  // entropy_coding_sync_enabled_flag  u(1)
  if (!bit_buffer->ReadBits(&(pps.entropy_coding_sync_enabled_flag), 1)) {
    return absl::nullopt;
  }

  if (pps.tiles_enabled_flag) {
    // num_tile_columns_minus1  ue(v)
    if (!bit_buffer->ReadExponentialGolomb(&(pps.num_tile_columns_minus1))) {
      return absl::nullopt;
    }
    // num_tile_rows_minus1  ue(v)
    if (!bit_buffer->ReadExponentialGolomb(&(pps.num_tile_rows_minus1))) {
      return absl::nullopt;
    }
    // uniform_spacing_flag  u(1)
    if (!bit_buffer->ReadBits(&(pps.uniform_spacing_flag), 1)) {
      return absl::nullopt;
    }

    if (!pps.uniform_spacing_flag) {
      for (uint32_t i = 0; i < pps.num_tile_columns_minus1; i++) {
        // column_width_minus1[i]  ue(v)
        if (!bit_buffer->ReadExponentialGolomb(&golomb_tmp)) {
          return absl::nullopt;
        }
        pps.column_width_minus1.push_back(golomb_tmp);
      }
      for (uint32_t i = 0; i < pps.num_tile_rows_minus1; i++) {
        // row_height_minus1[i]  ue(v)
        if (!bit_buffer->ReadExponentialGolomb(&golomb_tmp)) {
          return absl::nullopt;
        }
        pps.row_height_minus1.push_back(golomb_tmp);
      }
    }

    // loop_filter_across_tiles_enabled_flag u(1)
    if (!bit_buffer->ReadBits(
        &(pps.loop_filter_across_tiles_enabled_flag), 1)) {
      return absl::nullopt;
    }
  }

  // pps_loop_filter_across_slices_enabled_flag u(1)
  if (!bit_buffer->ReadBits(
      &(pps.pps_loop_filter_across_slices_enabled_flag), 1)) {
    return absl::nullopt;
  }

  // deblocking_filter_control_present_flag  u(1)
  if (!bit_buffer->ReadBits(&(pps.deblocking_filter_control_present_flag), 1)) {
    return absl::nullopt;
  }

  if (pps.deblocking_filter_control_present_flag) {
    // deblocking_filter_override_enabled_flag u(1)
    if (!bit_buffer->ReadBits(
        &(pps.deblocking_filter_override_enabled_flag), 1)) {
      return absl::nullopt;
    }

    // pps_deblocking_filter_disabled_flag  u(1)
    if (!bit_buffer->ReadBits(&(pps.pps_deblocking_filter_disabled_flag), 1)) {
      return absl::nullopt;
    }

    if (!pps.pps_deblocking_filter_disabled_flag) {
      // pps_beta_offset_div2  se(v)
      if (!bit_buffer->ReadSignedExponentialGolomb(
          &(pps.pps_beta_offset_div2))) {
        return absl::nullopt;
      }

      // pps_tc_offset_div2  se(v)
      if (!bit_buffer->ReadSignedExponentialGolomb(&(pps.pps_tc_offset_div2))) {
        return absl::nullopt;
      }
    }
  }

  // pps_scaling_list_data_present_flag  u(1)
  if (!bit_buffer->ReadBits(&(pps.pps_scaling_list_data_present_flag), 1)) {
    return absl::nullopt;
  }

  if (pps.pps_scaling_list_data_present_flag) {
    // scaling_list_data()
    // TODO(chemag): add support for scaling_list_data()
    XPrintf(stderr, "error: unimplemented scaling_list_data() in pps\n");
    return absl::nullopt;
  }

  // lists_modification_present_flag  u(1)
  if (!bit_buffer->ReadBits(&(pps.lists_modification_present_flag), 1)) {
    return absl::nullopt;
  }

  // log2_parallel_merge_level_minus2  ue(v)
  if (!bit_buffer->ReadExponentialGolomb(
      &(pps.log2_parallel_merge_level_minus2))) {
    return absl::nullopt;
  }

  // slice_segment_header_extension_present_flag  u(1)
  if (!bit_buffer->ReadBits(
      &(pps.slice_segment_header_extension_present_flag), 1)) {
    return absl::nullopt;
  }

  // pps_extension_present_flag  u(1)
  if (!bit_buffer->ReadBits(&(pps.pps_extension_present_flag), 1)) {
    return absl::nullopt;
  }

  if (pps.pps_extension_present_flag) {
    // pps_range_extension_flag  u(1)
    if (!bit_buffer->ReadBits(&(pps.pps_range_extension_flag), 1)) {
      return absl::nullopt;
    }

    // pps_multilayer_extension_flag  u(1)
    if (!bit_buffer->ReadBits(&(pps.pps_multilayer_extension_flag), 1)) {
      return absl::nullopt;
    }

    // pps_3d_extension_flag  u(1)
    if (!bit_buffer->ReadBits(&(pps.pps_3d_extension_flag), 1)) {
      return absl::nullopt;
    }

    // pps_scc_extension_flag  u(1)
    if (!bit_buffer->ReadBits(&(pps.pps_scc_extension_flag), 1)) {
      return absl::nullopt;
    }

    // pps_extension_4bits  u(4)
    if (!bit_buffer->ReadBits(&(pps.pps_extension_4bits), 4)) {
      return absl::nullopt;
    }
  }

  if (pps.pps_range_extension_flag) {
    // pps_range_extension()
    // TODO(chemag): add support for pps_range_extension()
    XPrintf(stderr, "error: unimplemented pps_range_extension() in pps\n");
    return absl::nullopt;
  }

  if (pps.pps_multilayer_extension_flag) {
    // pps_multilayer_extension() // specified in Annex F
    // TODO(chemag): add support for pps_multilayer_extension()
    XPrintf(stderr, "error: unimplemented pps_multilayer_extension() in pps\n");
    return absl::nullopt;
  }

  if (pps.pps_3d_extension_flag) {
    // pps_3d_extension() // specified in Annex I
    // TODO(chemag): add support for pps_3d_extension()
    XPrintf(stderr, "error: unimplemented pps_3d_extension() in pps\n");
    return absl::nullopt;
  }

  if (pps.pps_scc_extension_flag) {
    // pps_scc_extension()
    // TODO(chemag): add support for pps_scc_extension()
    XPrintf(stderr, "error: unimplemented pps_scc_extension() in pps\n");
    return absl::nullopt;
  }

  if (pps.pps_extension_4bits) {
    while (more_rbsp_data(bit_buffer)) {
      // pps_extension_data_flag  u(1)
      if (!bit_buffer->ReadBits(&(pps.pps_extension_data_flag), 1)) {
        return absl::nullopt;
      }
    }
  }

  rbsp_trailing_bits(bit_buffer);

  return OptionalPps(pps);
}

void H265PpsParser::PpsState::fdump(XFILE outfp, int indent_level) const {
  XPrintf(outfp, "pps {");
  indent_level = indent_level_incr(indent_level);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "pps_pic_parameter_set_id: %i", pps_pic_parameter_set_id);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "pps_seq_parameter_set_id: %i", pps_seq_parameter_set_id);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "dependent_slice_segments_enabled_flag: %i",
          dependent_slice_segments_enabled_flag);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "output_flag_present_flag: %i", output_flag_present_flag);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "num_extra_slice_header_bits: %i",
          num_extra_slice_header_bits);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "sign_data_hiding_enabled_flag: %i",
          sign_data_hiding_enabled_flag);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "cabac_init_present_flag: %i", cabac_init_present_flag);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "num_ref_idx_l0_default_active_minus1: %i",
          num_ref_idx_l0_default_active_minus1);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "num_ref_idx_l1_default_active_minus1: %i",
          num_ref_idx_l1_default_active_minus1);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "init_qp_minus26: %i", init_qp_minus26);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "constrained_intra_pred_flag: %i",
          constrained_intra_pred_flag);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "transform_skip_enabled_flag: %i",
          transform_skip_enabled_flag);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "cu_qp_delta_enabled_flag: %i", cu_qp_delta_enabled_flag);

  if (cu_qp_delta_enabled_flag) {
    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "cu_qp_delta_enabled_flag: %i", cu_qp_delta_enabled_flag);
  }

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "pps_cb_qp_offset: %i", pps_cb_qp_offset);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "pps_cr_qp_offset: %i", pps_cr_qp_offset);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "pps_slice_chroma_qp_offsets_present_flag: %i",
          pps_slice_chroma_qp_offsets_present_flag);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "weighted_pred_flag: %i", weighted_pred_flag);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "weighted_bipred_flag: %i", weighted_bipred_flag);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "transquant_bypass_enabled_flag: %i",
          transquant_bypass_enabled_flag);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "tiles_enabled_flag: %i", tiles_enabled_flag);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "entropy_coding_sync_enabled_flag: %i",
          entropy_coding_sync_enabled_flag);

  if (tiles_enabled_flag) {
    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "num_tile_columns_minus1: %i", num_tile_columns_minus1);

    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "num_tile_rows_minus1: %i", num_tile_rows_minus1);

    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "uniform_spacing_flag: %i", uniform_spacing_flag);


    if (!uniform_spacing_flag) {
      fdump_indent_level(outfp, indent_level);
      XPrintf(outfp, "column_width_minus1 {");
      for (const uint32_t& v : column_width_minus1) {
        XPrintf(outfp, " %i", v);
      }
      XPrintf(outfp, " }");

      fdump_indent_level(outfp, indent_level);
      XPrintf(outfp, "row_height_minus1 {");
      for (const uint32_t& v : row_height_minus1) {
        XPrintf(outfp, " %i", v);
      }
      XPrintf(outfp, " }");
    }

    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "loop_filter_across_tiles_enabled_flag: %i",
            loop_filter_across_tiles_enabled_flag);
  }

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "pps_loop_filter_across_slices_enabled_flag: %i",
          pps_loop_filter_across_slices_enabled_flag);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "deblocking_filter_control_present_flag: %i",
          deblocking_filter_control_present_flag);

  if (deblocking_filter_control_present_flag) {
    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "deblocking_filter_override_enabled_flag: %i",
          deblocking_filter_override_enabled_flag);

    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "pps_deblocking_filter_disabled_flag: %i",
          pps_deblocking_filter_disabled_flag);

    if (!pps_deblocking_filter_disabled_flag) {
      fdump_indent_level(outfp, indent_level);
      XPrintf(outfp, "pps_beta_offset_div2: %i", pps_beta_offset_div2);

      fdump_indent_level(outfp, indent_level);
      XPrintf(outfp, "pps_tc_offset_div2: %i", pps_tc_offset_div2);
    }
  }

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "pps_scaling_list_data_present_flag: %i",
          pps_scaling_list_data_present_flag);

  if (pps_scaling_list_data_present_flag) {
    // scaling_list_data()
    // TODO(chemag): add support for scaling_list_data()
    XPrintf(stderr, "error: unimplemented scaling_list_data() in pps\n");
  }

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "lists_modification_present_flag: %i",
          lists_modification_present_flag);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "log2_parallel_merge_level_minus2: %i",
          log2_parallel_merge_level_minus2);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "slice_segment_header_extension_present_flag: %i",
          slice_segment_header_extension_present_flag);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "pps_extension_present_flag: %i",
          pps_extension_present_flag);

  if (pps_extension_present_flag) {
    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "pps_range_extension_flag: %i", pps_range_extension_flag);

    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "pps_multilayer_extension_flag: %i",
            pps_multilayer_extension_flag);

    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "pps_3d_extension_flag: %i", pps_3d_extension_flag);

    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "pps_scc_extension_flag: %i", pps_scc_extension_flag);

    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "pps_extension_4bits: %i", pps_extension_4bits);
  }

  if (pps_range_extension_flag) {
    // pps_range_extension()
    // TODO(chemag): add support for pps_range_extension()
    XPrintf(stderr, "error: unimplemented pps_range_extension() in pps\n");
  }

  if (pps_multilayer_extension_flag) {
    // pps_multilayer_extension() // specified in Annex F
    // TODO(chemag): add support for pps_multilayer_extension()
    XPrintf(stderr, "error: unimplemented pps_multilayer_extension_flag() in "
            "pps\n");
  }

  if (pps_3d_extension_flag) {
    // pps_3d_extension() // specified in Annex I
    // TODO(chemag): add support for pps_3d_extension()
    XPrintf(stderr, "error: unimplemented pps_3d_extension_flag() in pps\n");
  }

  if (pps_scc_extension_flag) {
    // pps_scc_extension()
    // TODO(chemag): add support for pps_scc_extension()
    XPrintf(stderr, "error: unimplemented pps_scc_extension_flag() in pps\n");
  }

  indent_level = indent_level_decr(indent_level);
  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "}");
}

}  // namespace h265nal
