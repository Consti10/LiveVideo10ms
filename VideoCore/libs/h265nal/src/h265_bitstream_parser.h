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

// A class for parsing out an H265 Bitstream.
class H265BitstreamParser {
 public:
  // The parsed state of the bitstream (a list of parsed NAL units plus
  // the accumulated VPS/PPS/SPS state).
  struct BitstreamState {
    BitstreamState() = default;
    BitstreamState(const BitstreamState&) = default;
    ~BitstreamState() = default;
    void fdump(XFILE outfp, int indent_level) const;

    bool add_offset;
    bool add_length;
    // NAL units
    std::vector<struct H265NalUnitParser::NalUnitState> nal_units;

    // bitstream parser state
    struct H265BitstreamParserState bitstream_parser_state;
  };

  // Unpack RBSP and parse bitstream state from the supplied buffer.
  static absl::optional<BitstreamState> ParseBitstream(
      const uint8_t* data, size_t length);

  struct NaluIndex {
    // Start index of NALU, including start sequence.
    size_t start_offset;
    // Start index of NALU payload, typically type header.
    size_t payload_start_offset;
    // Length of NALU payload, in bytes, counting from payload_start_offset.
    size_t payload_size;
  };
  // Returns a vector of the NALU indices in the given buffer.
  static std::vector<NaluIndex> FindNaluIndices(
      const uint8_t* data, size_t length);
};

}  // namespace h265nal
