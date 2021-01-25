/*
 *  Copyright (c) Facebook, Inc. and its affiliates.
 */

#pragma once

#include <stdio.h>

#include <vector>

#include "absl/types/optional.h"

#include "rtc_base/bit_buffer.h"

#include "h265_nal_unit_parser.h"

namespace h265nal {

// A class for parsing out an RTP FU (Fragmentation Units) data.
class H265RtpFuParser {
 public:
  // The parsed state of the RTP FU.
  struct RtpFuState {
    RtpFuState() = default;
    RtpFuState(const RtpFuState&) = default;
    ~RtpFuState() = default;
    void fdump(FILE* outfp, int indent_level) const;

    // common header
    struct H265NalUnitHeaderParser::NalUnitHeaderState header;

    // fu header
    uint32_t s_bit;
    uint32_t e_bit;
    uint32_t fu_type;

    // optional payload
    struct H265NalUnitPayloadParser::NalUnitPayloadState nal_unit_payload;
  };

  // Unpack RBSP and parse RTP FU state from the supplied buffer.
  static absl::optional<RtpFuState>
      ParseRtpFu(const uint8_t* data, size_t length,
      struct H265BitstreamParserState* bitstream_parser_state);
  static absl::optional<RtpFuState>
      ParseRtpFu(rtc::BitBuffer* bit_buffer,
      struct H265BitstreamParserState* bitstream_parser_state);
};

}  // namespace h265nal
