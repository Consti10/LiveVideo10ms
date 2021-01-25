/*
 *  Copyright (c) Facebook, Inc. and its affiliates.
 */


#include "h265_rtp_ap_parser.h"

#include <stdio.h>

#include <cstdint>
#include <vector>

#include "h265_common.h"
#include "h265_bitstream_parser_state.h"
#include "h265_nal_unit_parser.h"
#include "absl/types/optional.h"
#include "rtc_base/bit_buffer.h"

namespace {
typedef absl::optional<h265nal::H265NalUnitHeaderParser::
    NalUnitHeaderState> OptionalNalUnitHeader;
typedef absl::optional<h265nal::H265NalUnitPayloadParser::
    NalUnitPayloadState> OptionalNalUnitPayload;
typedef absl::optional<h265nal::H265RtpApParser::
    RtpApState> OptionalRtpAp;
}  // namespace

namespace h265nal {

// General note: this is based off rfc7798/
// You can find it on this page:
// https://tools.ietf.org/html/rfc7798#section-4.4.2

// Unpack RBSP and parse RTP AP state from the supplied buffer.
absl::optional<H265RtpApParser::RtpApState> H265RtpApParser::ParseRtpAp(
    const uint8_t* data, size_t length,
    struct H265BitstreamParserState* bitstream_parser_state) {

  std::vector<uint8_t> unpacked_buffer = UnescapeRbsp(data, length);
  rtc::BitBuffer bit_buffer(unpacked_buffer.data(), unpacked_buffer.size());
  return ParseRtpAp(&bit_buffer, bitstream_parser_state);
}


absl::optional<H265RtpApParser::RtpApState> H265RtpApParser::ParseRtpAp(
    rtc::BitBuffer* bit_buffer,
    struct H265BitstreamParserState* bitstream_parser_state) {
  // H265 RTP AP pseudo-NAL Unit.
  RtpApState rtp_ap;

  // first read the common header
  OptionalNalUnitHeader nal_unit_header_common =
      H265NalUnitHeaderParser::ParseNalUnitHeader(bit_buffer);
  if (nal_unit_header_common != absl::nullopt) {
    rtp_ap.header = *nal_unit_header_common;
  }

  while (bit_buffer->RemainingBitCount() > 0) {
    // NALU size
    uint32_t nalu_size;
    if (!bit_buffer->ReadBits(&nalu_size, 16)) {
      return absl::nullopt;
    }
    rtp_ap.nal_unit_sizes.push_back(nalu_size);

    // NALU header
    OptionalNalUnitHeader nal_unit_header =
        H265NalUnitHeaderParser::ParseNalUnitHeader(bit_buffer);
    if (nal_unit_header != absl::nullopt) {
      rtp_ap.nal_unit_headers.push_back(*nal_unit_header);
    }

    // NALU payload
    OptionalNalUnitPayload nal_unit_payload =
        H265NalUnitPayloadParser::ParseNalUnitPayload(
        bit_buffer, nal_unit_header->nal_unit_type, bitstream_parser_state);
    if (nal_unit_payload != absl::nullopt) {
      rtp_ap.nal_unit_payloads.push_back(*nal_unit_payload);
    }
  }
  return OptionalRtpAp(rtp_ap);
}

void H265RtpApParser::RtpApState::fdump(FILE* outfp, int indent_level) const {
  fprintf(outfp, "rtp_ap {");
  indent_level = indent_level_incr(indent_level);

  fdump_indent_level(outfp, indent_level);
  //header.fdump(outfp, indent_level);

  for (unsigned int i = 0; i < nal_unit_sizes.size(); ++i) {
    fdump_indent_level(outfp, indent_level);
    fprintf(outfp, "nal_unit_size: %u", nal_unit_sizes[i]);

    fdump_indent_level(outfp, indent_level);
    //nal_unit_headers[i].fdump(outfp, indent_level);

    fdump_indent_level(outfp, indent_level);
    //nal_unit_payloads[i].fdump(outfp, nal_unit_headers[i].nal_unit_type,
    //                           indent_level);
  }

  indent_level = indent_level_decr(indent_level);
  fdump_indent_level(outfp, indent_level);
  fprintf(outfp, "}");
}

}  // namespace h265nal
