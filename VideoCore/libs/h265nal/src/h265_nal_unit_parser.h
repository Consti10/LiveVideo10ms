/*
 *  Copyright (c) Facebook, Inc. and its affiliates.
 */

#pragma once

#include <stdio.h>

#include <vector>

#include "absl/types/optional.h"

#include "rtc_base/bit_buffer.h"

#include "h265_aud_parser.h"
#include "h265_pps_parser.h"
#include "h265_slice_parser.h"
#include "h265_sps_parser.h"
#include "h265_vps_parser.h"

namespace h265nal {

// A class for parsing out an H265 NAL Unit Header.
class H265NalUnitHeaderParser {
 public:
  // The parsed state of the NAL Unit Header.
  struct NalUnitHeaderState {
    NalUnitHeaderState() = default;
    NalUnitHeaderState(const NalUnitHeaderState&) = default;
    ~NalUnitHeaderState() = default;
    void fdump(XFILE outfp, int indent_level) const;

    uint32_t forbidden_zero_bit = 0;
    uint32_t nal_unit_type = 0;
    uint32_t nuh_layer_id = 0;
    uint32_t nuh_temporal_id_plus1 = 0;
  };

  // Unpack RBSP and parse NAL unit header state from the supplied buffer.
  static absl::optional<NalUnitHeaderState> ParseNalUnitHeader(
      const uint8_t* data, size_t length);
  static absl::optional<NalUnitHeaderState> ParseNalUnitHeader(
      rtc::BitBuffer* bit_buffer);
};


// A class for parsing out an H265 NAL Unit Payload.
class H265NalUnitPayloadParser {
 public:
  // The parsed state of the NAL Unit Payload. Only some select values are
  // stored. Add more as they are actually needed.
  struct NalUnitPayloadState {
    NalUnitPayloadState() = default;
    NalUnitPayloadState(const NalUnitPayloadState&) = default;
    ~NalUnitPayloadState() = default;
    void fdump(XFILE outfp, int indent_level, uint32_t nal_unit_type) const;

    struct H265VpsParser::VpsState vps;
    struct H265SpsParser::SpsState sps;
    struct H265PpsParser::PpsState pps;
    struct H265AudParser::AudState aud;
    struct H265SliceSegmentLayerParser::SliceSegmentLayerState
        slice_segment_layer;
  };

  // Unpack RBSP and parse NAL unit payload state from the supplied buffer.
  static absl::optional<NalUnitPayloadState> ParseNalUnitPayload(
      const uint8_t* data, size_t length,
      uint32_t nal_unit_type,
      struct H265BitstreamParserState* bitstream_parser_state);
  static absl::optional<NalUnitPayloadState> ParseNalUnitPayload(
      rtc::BitBuffer* bit_buffer,
      uint32_t nal_unit_type,
      struct H265BitstreamParserState* bitstream_parser_state);
};

// A class for parsing out an H265 NAL Unit.
class H265NalUnitParser {
 public:
  // The parsed state of the NAL Unit. Only some select values are stored.
  // Add more as they are actually needed.
  struct NalUnitState {
    NalUnitState() = default;
    NalUnitState(const NalUnitState&) = default;
    ~NalUnitState() = default;
    void fdump(XFILE outfp, int indent_level, bool add_offset,
               bool add_length) const;

    size_t offset;
    size_t length;

    struct H265NalUnitHeaderParser::NalUnitHeaderState nal_unit_header;
    struct H265NalUnitPayloadParser::NalUnitPayloadState nal_unit_payload;
  };

  // Unpack RBSP and parse NAL unit state from the supplied buffer.
  static absl::optional<NalUnitState> ParseNalUnit(
      const uint8_t* data, size_t length,
      struct H265BitstreamParserState* bitstream_parser_state);
  static absl::optional<NalUnitState> ParseNalUnit(
      rtc::BitBuffer* bit_buffer,
      struct H265BitstreamParserState* bitstream_parser_state);
};

}  // namespace h265nal
