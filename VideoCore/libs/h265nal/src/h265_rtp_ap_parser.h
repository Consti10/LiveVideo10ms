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

// A class for parsing out an RTP AP (Aggregated Packet) data.
class H265RtpApParser {
 public:
  // The parsed state of the RTP AP.
  struct RtpApState {
    RtpApState() = default;
    RtpApState(const RtpApState&) = default;
    ~RtpApState() = default;
    void fdump(FILE* outfp, int indent_level) const;

    // common header
    struct H265NalUnitHeaderParser::NalUnitHeaderState header;

    // payload
    std::vector<size_t> nal_unit_sizes;
    std::vector<struct H265NalUnitHeaderParser::NalUnitHeaderState>
        nal_unit_headers;
    std::vector<struct H265NalUnitPayloadParser::NalUnitPayloadState>
        nal_unit_payloads;
  };

  // Unpack RBSP and parse RTP AP state from the supplied buffer.
  static absl::optional<RtpApState>
      ParseRtpAp(const uint8_t* data, size_t length,
      struct H265BitstreamParserState* bitstream_parser_state);
  static absl::optional<RtpApState>
      ParseRtpAp(rtc::BitBuffer* bit_buffer,
      struct H265BitstreamParserState* bitstream_parser_state);
};

}  // namespace h265nal
