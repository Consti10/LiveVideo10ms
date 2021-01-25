/*
 *  Copyright (c) Facebook, Inc. and its affiliates.
 */


#include "h265_vps_parser.h"

#include <stdio.h>

#include <cstdint>
#include <vector>

#include "h265_common.h"
#include "absl/types/optional.h"

namespace {
typedef absl::optional<h265nal::H265ProfileTierLevelParser::
    ProfileTierLevelState> OptionalProfileTierLevel;
typedef absl::optional<h265nal::H265VpsParser::
    VpsState> OptionalVps;
}  // namespace

namespace h265nal {

// General note: this is based off the 2016/12 version of the H.265 standard.
// You can find it on this page:
// http://www.itu.int/rec/T-REC-H.265

// Unpack RBSP and parse VPS state from the supplied buffer.
absl::optional<H265VpsParser::VpsState> H265VpsParser::ParseVps(
    const uint8_t* data, size_t length) {

  std::vector<uint8_t> unpacked_buffer = UnescapeRbsp(data, length);
  rtc::BitBuffer bit_buffer(unpacked_buffer.data(), unpacked_buffer.size());
  return ParseVps(&bit_buffer);
}


absl::optional<H265VpsParser::VpsState> H265VpsParser::ParseVps(
    rtc::BitBuffer* bit_buffer) {
  uint32_t golomb_tmp;

  // H265 VPS (video_parameter_set_rbsp()) NAL Unit.
  // Section 7.3.2.1 ("Video parameter set data syntax") of the H.265
  // standard for a complete description.
  VpsState vps;

  // vps_video_parameter_set_id  u(4)
  if (!bit_buffer->ReadBits(&(vps.vps_video_parameter_set_id), 4)) {
    return absl::nullopt;
  }

  // vps_base_layer_internal_flag  u(1)
  if (!bit_buffer->ReadBits(&(vps.vps_base_layer_internal_flag), 1)) {
    return absl::nullopt;
  }

  // vps_base_layer_available_flag  u(1)
  if (!bit_buffer->ReadBits(&(vps.vps_base_layer_available_flag), 1)) {
    return absl::nullopt;
  }

  // vps_max_layers_minus1  u(6)
  if (!bit_buffer->ReadBits(&(vps.vps_max_layers_minus1), 6)) {
    return absl::nullopt;
  }

  // vps_max_sub_layers_minus1  u(3)
  if (!bit_buffer->ReadBits(&(vps.vps_max_sub_layers_minus1), 3)) {
    return absl::nullopt;
  }

  // vps_temporal_id_nesting_flag  u(1)
  if (!bit_buffer->ReadBits(&(vps.vps_temporal_id_nesting_flag), 1)) {
    return absl::nullopt;
  }

  // vps_reserved_0xffff_16bits  u(16)
  if (!bit_buffer->ReadBits(&(vps.vps_reserved_0xffff_16bits), 16)) {
    return absl::nullopt;
  }

  // profile_tier_level(1, vps_max_sub_layers_minus1)
  OptionalProfileTierLevel profile_tier_level =
      H265ProfileTierLevelParser::ParseProfileTierLevel(
          bit_buffer, true, vps.vps_max_sub_layers_minus1);
  if (profile_tier_level != absl::nullopt) {
    vps.profile_tier_level = *profile_tier_level;
  }

  // vps_sub_layer_ordering_info_present_flag  u(1)
  if (!bit_buffer->ReadBits(&(vps.vps_sub_layer_ordering_info_present_flag),
                            1)) {
    return absl::nullopt;
  }

  for (uint32_t i = (vps.vps_sub_layer_ordering_info_present_flag ?
       0 : vps.vps_max_sub_layers_minus1);
       i <= vps.vps_max_sub_layers_minus1; i++) {
    // vps_max_dec_pic_buffering_minus1[i]  ue(v)
    if (!bit_buffer->ReadExponentialGolomb(&golomb_tmp)) {
      return absl::nullopt;
    }
    vps.vps_max_dec_pic_buffering_minus1.push_back(golomb_tmp);
    // vps_max_num_reorder_pics[i]  ue(v)
    if (!bit_buffer->ReadExponentialGolomb(&golomb_tmp)) {
      return absl::nullopt;
    }
    vps.vps_max_num_reorder_pics.push_back(golomb_tmp);
    // vps_max_latency_increase_plus1[i]  ue(v)
    if (!bit_buffer->ReadExponentialGolomb(&golomb_tmp)) {
      return absl::nullopt;
    }
    vps.vps_max_latency_increase_plus1.push_back(golomb_tmp);
  }

  // vps_max_layer_id  u(6)
  if (!bit_buffer->ReadBits(&(vps.vps_max_layer_id), 6)) {
    return absl::nullopt;
  }

  // vps_num_layer_sets_minus1  ue(v)
  if (!bit_buffer->ReadExponentialGolomb(&(vps.vps_num_layer_sets_minus1))) {
    return absl::nullopt;
  }

  for (uint32_t i = 1; i <= vps.vps_num_layer_sets_minus1; i++) {
    vps.layer_id_included_flag[i-1].emplace_back();
    for (uint32_t j = 0; j <= vps.vps_max_layer_id; j++) {
      // layer_id_included_flag[i][j]  u(1)
      vps.layer_id_included_flag[i-1].push_back(golomb_tmp);
    }
  }

  // vps_timing_info_present_flag  u(1)
  if (!bit_buffer->ReadBits(&(vps.vps_timing_info_present_flag), 1)) {
    return absl::nullopt;
  }

  if (vps.vps_timing_info_present_flag) {
    // vps_num_units_in_tick  u(32)
    if (!bit_buffer->ReadBits(&(vps.vps_num_units_in_tick), 32)) {
      return absl::nullopt;
    }

    // vps_time_scale  u(32)
    if (!bit_buffer->ReadBits(&(vps.vps_time_scale), 32)) {
      return absl::nullopt;
    }

    // vps_poc_proportional_to_timing_flag  u(1)
    if (!bit_buffer->ReadBits(&(vps.vps_poc_proportional_to_timing_flag), 1)) {
      return absl::nullopt;
    }

    if (vps.vps_poc_proportional_to_timing_flag) {
      // vps_num_ticks_poc_diff_one_minus1  ue(v)
      if (!bit_buffer->ReadExponentialGolomb(
          &(vps.vps_num_ticks_poc_diff_one_minus1))) {
        return absl::nullopt;
      }
    }
    // vps_num_hrd_parameters  ue(v)
    if (!bit_buffer->ReadExponentialGolomb(&(vps.vps_num_hrd_parameters))) {
      return absl::nullopt;
    }

    for (uint32_t i = 0; i < vps.vps_num_hrd_parameters; i++) {
      // hrd_layer_set_idx[i]  ue(v)
      if (!bit_buffer->ReadExponentialGolomb(&golomb_tmp)) {
        return absl::nullopt;
      }
      vps.hrd_layer_set_idx.push_back(golomb_tmp);

      if (i > 0) {
        // cprms_present_flag[i]  u(1)
        if (!bit_buffer->ReadExponentialGolomb(&golomb_tmp)) {
          return absl::nullopt;
        }
        vps.cprms_present_flag.push_back(golomb_tmp);
      }
      // hrd_parameters(cprms_present_flag[i], vps_max_sub_layers_minus1)
      // TODO(chemag): add support for hrd_parameters()
      XPrintf(stderr, "error: unimplemented hrd_parameters in vps\n");
      return absl::nullopt;
    }
  }

  // vps_extension_flag  u(1)
  if (!bit_buffer->ReadBits(&(vps.vps_extension_flag), 1)) {
    return absl::nullopt;
  }

  if (vps.vps_extension_flag) {
    while (more_rbsp_data(bit_buffer)) {
      // vps_extension_data_flag  u(1)
      if (!bit_buffer->ReadBits(&(vps.vps_extension_data_flag), 1)) {
        return absl::nullopt;
      }
    }
  }
  rbsp_trailing_bits(bit_buffer);

  return OptionalVps(vps);
}

void H265VpsParser::VpsState::fdump(XFILE outfp, int indent_level) const {
  XPrintf(outfp, "vps {");
  indent_level = indent_level_incr(indent_level);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "vps_video_parameter_set_id: %i",
          vps_video_parameter_set_id);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "vps_base_layer_internal_flag: %i",
          vps_base_layer_internal_flag);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "vps_base_layer_available_flag: %i",
          vps_base_layer_available_flag);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "vps_max_layers_minus1: %i", vps_max_layers_minus1);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "vps_max_sub_layers_minus1: %i",
          vps_max_sub_layers_minus1);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "vps_temporal_id_nesting_flag: %i",
          vps_temporal_id_nesting_flag);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "vps_reserved_0xffff_16bits: 0x%04x",
          vps_reserved_0xffff_16bits);

  fdump_indent_level(outfp, indent_level);
  profile_tier_level.fdump(outfp, indent_level);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "vps_sub_layer_ordering_info_present_flag: %i",
          vps_sub_layer_ordering_info_present_flag);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "vps_max_dec_pic_buffering_minus1 {");
  for (const uint32_t& v : vps_max_dec_pic_buffering_minus1) {
    XPrintf(outfp, " %i", v);
  }
  XPrintf(outfp, " }");

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "vps_max_num_reorder_pics {");
  for (const uint32_t& v : vps_max_num_reorder_pics) {
    XPrintf(outfp, " %i", v);
  }
  XPrintf(outfp, " }");

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "vps_max_latency_increase_plus1 {");
  for (const uint32_t& v : vps_max_latency_increase_plus1) {
    XPrintf(outfp, " %i", v);
  }
  XPrintf(outfp, " }");

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "vps_max_layer_id: %i", vps_max_layer_id);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "vps_num_layer_sets_minus1: %i",
          vps_num_layer_sets_minus1);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "layer_id_included_flag {");
  for (const std::vector<uint32_t>& vv : layer_id_included_flag) {
    XPrintf(outfp, " {");
    for (const uint32_t& v : vv) {
      XPrintf(outfp, " %i", v);
    }
    XPrintf(outfp, " }");
  }
  XPrintf(outfp, " }");

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "vps_timing_info_present_flag: %i",
          vps_timing_info_present_flag);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "vps_num_units_in_tick: %i", vps_num_units_in_tick);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "vps_time_scale: %i", vps_time_scale);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "vps_poc_proportional_to_timing_flag: %i",
          vps_poc_proportional_to_timing_flag);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "vps_num_ticks_poc_diff_one_minus1: %i",
          vps_num_ticks_poc_diff_one_minus1);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "vps_num_hrd_parameters: %i", vps_num_hrd_parameters);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "hrd_layer_set_idx {");
  for (const uint32_t& v : hrd_layer_set_idx) {
    XPrintf(outfp, " %i", v);
  }
  XPrintf(outfp, " }");

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "cprms_present_flag {");
  for (const uint32_t& v : cprms_present_flag) {
    XPrintf(outfp, " %i", v);
  }
  XPrintf(outfp, " }");

  // hrd_parameters(cprms_present_flag[i], vps_max_sub_layers_minus1)
  // TODO(chemag): add support for hrd_parameters()

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "vps_extension_flag: %i", vps_extension_flag);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "vps_extension_data_flag: %i", vps_extension_data_flag);

  indent_level = indent_level_decr(indent_level);
  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "}");
}

}  // namespace h265nal
