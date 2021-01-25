/*
 *  Copyright (c) Facebook, Inc. and its affiliates.
 */


#include "h265_rtp_single_parser.h"

#include <stdio.h>

#include <cstdint>
#include <vector>

#include "h265_common.h"
#include "h265_nal_unit_parser.h"
#include "absl/types/optional.h"

namespace {
typedef absl::optional<h265nal::H265NalUnitHeaderParser::
    NalUnitHeaderState> OptionalNalUnitHeader;
typedef absl::optional<h265nal::H265NalUnitPayloadParser::
    NalUnitPayloadState> OptionalNalUnitPayload;
typedef absl::optional<h265nal::H265RtpSingleParser::
    RtpSingleState> OptionalRtpSingle;
}  // namespace

namespace h265nal {

// General note: this is based off rfc7798.
// You can find it on this page:
// https://tools.ietf.org/html/rfc7798#section-4.4.1

// Unpack RBSP and parse RTP Single NAL Unit state from the supplied buffer.
absl::optional<H265RtpSingleParser::RtpSingleState>
H265RtpSingleParser::ParseRtpSingle(
    const uint8_t* data, size_t length,
    struct H265BitstreamParserState* bitstream_parser_state) {

  std::vector<uint8_t> unpacked_buffer = UnescapeRbsp(data, length);
  rtc::BitBuffer bit_buffer(unpacked_buffer.data(), unpacked_buffer.size());
  return ParseRtpSingle(&bit_buffer, bitstream_parser_state);
}

absl::optional<H265RtpSingleParser::RtpSingleState>
H265RtpSingleParser::ParseRtpSingle(
    rtc::BitBuffer* bit_buffer,
    struct H265BitstreamParserState* bitstream_parser_state) {
  // H265 RTP Single NAL Unit pseudo-NAL Unit.
  RtpSingleState rtp_single;

  // nal_unit_header()
  OptionalNalUnitHeader nal_unit_header =
      H265NalUnitHeaderParser::ParseNalUnitHeader(bit_buffer);
  if (nal_unit_header != absl::nullopt) {
    rtp_single.nal_unit_header = *nal_unit_header;
  }

  // nal_unit_payload()
  OptionalNalUnitPayload nal_unit_payload =
      H265NalUnitPayloadParser::ParseNalUnitPayload(
          bit_buffer, rtp_single.nal_unit_header.nal_unit_type,
          bitstream_parser_state);
  if (nal_unit_payload != absl::nullopt) {
    rtp_single.nal_unit_payload = *nal_unit_payload;
  }

  return OptionalRtpSingle(rtp_single);
}

void H265RtpSingleParser::RtpSingleState::fdump(
        FILE* outfp, int indent_level) const {
  fprintf(outfp, "rtp_single {");
  indent_level = indent_level_incr(indent_level);

  // header
  fdump_indent_level(outfp, indent_level);
  //nal_unit_header.fdump(outfp, indent_level);

  // payload
  fdump_indent_level(outfp, indent_level);
  //nal_unit_payload.fdump(outfp, indent_level, nal_unit_header.nal_unit_type);

  indent_level = indent_level_decr(indent_level);
  fdump_indent_level(outfp, indent_level);
  fprintf(outfp, "}");
}

}  // namespace h265nal
