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

// A class for parsing out a video sequence parameter set (AUD) data from
// an H265 NALU.
class H265AudParser {
 public:
  // The parsed state of the AUD.
  struct AudState {
    AudState() = default;
    AudState(const AudState&) = default;
    ~AudState() = default;
    void fdump(XFILE outfp, int indent_level) const;

    uint32_t pic_type = 0;
  };

  // Unpack RBSP and parse AUD state from the supplied buffer.
  static absl::optional<AudState> ParseAud(const uint8_t* data, size_t length);
  static absl::optional<AudState> ParseAud(rtc::BitBuffer* bit_buffer);
};

}  // namespace h265nal
