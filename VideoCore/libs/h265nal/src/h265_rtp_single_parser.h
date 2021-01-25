/*
 *  Copyright (c) Facebook, Inc. and its affiliates.
 */

#pragma once

#include <stdio.h>

#include <vector>

#include "absl/types/optional.h"

#include "rtc_base/bit_buffer.h"

#include "h265_bitstream_parser_state.h"
#include "h265_nal_unit_parser.h"

namespace h265nal {

// A class for parsing out an RTP Single NAL Unit data.
class H265RtpSingleParser {
 public:
  // The parsed state of the RTP Single NAL Unit.
  struct RtpSingleState {
    RtpSingleState() = default;
    RtpSingleState(const RtpSingleState&) = default;
    ~RtpSingleState() = default;
    void fdump(FILE* outfp, int indent_level) const;

    struct H265NalUnitHeaderParser::NalUnitHeaderState nal_unit_header;
    struct H265NalUnitPayloadParser::NalUnitPayloadState nal_unit_payload;
  };

  // Unpack RBSP and parse RTP Single NAL Unit state from the supplied buffer.
  static absl::optional<RtpSingleState>
      ParseRtpSingle(const uint8_t* data, size_t length,
      struct H265BitstreamParserState* bitstream_parser_state);
  static absl::optional<RtpSingleState>
      ParseRtpSingle(rtc::BitBuffer* bit_buffer,
      struct H265BitstreamParserState* bitstream_parser_state);
};

}  // namespace h265nal
