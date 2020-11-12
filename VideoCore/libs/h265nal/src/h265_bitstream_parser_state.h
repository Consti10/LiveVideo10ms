/*
 *  Copyright (c) Facebook, Inc. and its affiliates.
 */

#pragma once

#include <stdio.h>

#include <map>

#include "absl/types/optional.h"

#include "h265_pps_parser.h"
#include "h265_sps_parser.h"
#include "h265_vps_parser.h"

namespace h265nal {

// A class for keeping the state of a H265 Bitstream.
// The parsed state of the bitstream.
struct H265BitstreamParserState {
  H265BitstreamParserState() = default;
  H265BitstreamParserState(const H265BitstreamParserState&) = default;
  ~H265BitstreamParserState() = default;

  // VPS state
  std::map<uint32_t, struct H265VpsParser::VpsState> vps;
  // SPS state
  std::map<uint32_t, struct H265SpsParser::SpsState> sps;
  // PPS state
  std::map<uint32_t, struct H265PpsParser::PpsState> pps;
};

}  // namespace h265nal
