/*
 *  Copyright (c) Facebook, Inc. and its affiliates.
 */

#pragma once

#include <stdio.h>

#include <vector>

#include "absl/types/optional.h"

#include "rtc_base/bit_buffer.h"
#include "h265_common.h"


namespace h265nal {

// A class for parsing out a Short-term reference picture set syntax
// (`st_ref_pic_set()`, as defined in Section 7.3.7 of the 2016-12
// standard) from an H265 NALU.
class H265StRefPicSetParser {
 public:
  // The parsed state of the StRefPicSet.
  struct StRefPicSetState {
    StRefPicSetState() = default;
    StRefPicSetState(const StRefPicSetState&) = default;
    ~StRefPicSetState() = default;
    void fdump(XFILE outfp, int indent_level) const;

    // input parameters
    uint32_t stRpsIdx = 0;
    uint32_t num_short_term_ref_pic_sets = 0;

    // contents
    uint32_t inter_ref_pic_set_prediction_flag = 0;
    uint32_t delta_idx_minus1 = 0;
    uint32_t delta_rps_sign = 0;
    uint32_t abs_delta_rps_minus1 = 0;
    std::vector<uint32_t> used_by_curr_pic_flag;
    std::vector<uint32_t> use_delta_flag;
    uint32_t num_negative_pics = 0;
    uint32_t num_positive_pics = 0;
    std::vector<uint32_t> delta_poc_s0_minus1;
    std::vector<uint32_t> used_by_curr_pic_s0_flag;
    std::vector<uint32_t> delta_poc_s1_minus1;
    std::vector<uint32_t> used_by_curr_pic_s1_flag;
  };

  // Unpack RBSP and parse StRefPicSet state from the supplied buffer.
  static absl::optional<StRefPicSetState> ParseStRefPicSet(
      const uint8_t* data, size_t length, uint32_t stRpsIdx,
      uint32_t num_short_term_ref_pic_sets);
  static absl::optional<StRefPicSetState> ParseStRefPicSet(
      rtc::BitBuffer* bit_buffer, uint32_t stRpsIdx,
      uint32_t num_short_term_ref_pic_sets);
};

}  // namespace h265nal
