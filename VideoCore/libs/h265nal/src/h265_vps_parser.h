/*
 *  Copyright (c) Facebook, Inc. and its affiliates.
 */

#pragma once

#include <stdio.h>

#include <vector>

#include "absl/types/optional.h"

#include "rtc_base/bit_buffer.h"

#include "h265_profile_tier_level_parser.h"

namespace h265nal {

// A class for parsing out a video sequence parameter set (VPS) data from
// an H265 NALU.
class H265VpsParser {
 public:
  // The parsed state of the VPS. Only some select values are stored.
  // Add more as they are actually needed.
  struct VpsState {
    VpsState() = default;
    VpsState(const VpsState&) = default;
    ~VpsState() = default;
    void fdump(XFILE outfp, int indent_level) const;
    std::string dump()const{
      std::stringstream ss;
      fdump(ss,-1);
      return ss.str();
    }

    uint32_t vps_video_parameter_set_id = 0;
    uint32_t vps_base_layer_internal_flag = 0;
    uint32_t vps_base_layer_available_flag = 0;
    uint32_t vps_max_layers_minus1 = 0;
    uint32_t vps_max_sub_layers_minus1 = 0;
    uint32_t vps_temporal_id_nesting_flag = 0;
    uint32_t vps_reserved_0xffff_16bits = 0;
    struct H265ProfileTierLevelParser::ProfileTierLevelState profile_tier_level;
    uint32_t vps_sub_layer_ordering_info_present_flag = 0;
    std::vector<uint32_t> vps_max_dec_pic_buffering_minus1;
    std::vector<uint32_t> vps_max_num_reorder_pics;
    std::vector<uint32_t> vps_max_latency_increase_plus1;
    uint32_t vps_max_layer_id = 0;
    uint32_t vps_num_layer_sets_minus1 = 0;
    std::vector<std::vector<uint32_t>> layer_id_included_flag;
    uint32_t vps_timing_info_present_flag = 0;
    uint32_t vps_num_units_in_tick = 0;
    uint32_t vps_time_scale = 0;
    uint32_t vps_poc_proportional_to_timing_flag = 0;
    uint32_t vps_num_ticks_poc_diff_one_minus1 = 0;
    uint32_t vps_num_hrd_parameters = 0;
    std::vector<uint32_t> hrd_layer_set_idx;
    std::vector<uint32_t> cprms_present_flag;
    uint32_t vps_extension_flag = 0;
    uint32_t vps_extension_data_flag = 0;
  };

  // Unpack RBSP and parse VPS state from the supplied buffer.
  static absl::optional<VpsState> ParseVps(const uint8_t* data, size_t length);
  static absl::optional<VpsState> ParseVps(rtc::BitBuffer* bit_buffer);
};

}  // namespace h265nal
