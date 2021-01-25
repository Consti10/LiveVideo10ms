/*
 *  Copyright (c) Facebook, Inc. and its affiliates.
 */


#include "h265_st_ref_pic_set_parser.h"

#include <stdio.h>

#include <cstdint>
#include <vector>

#include "h265_common.h"
#include "absl/types/optional.h"

namespace {
typedef absl::optional<h265nal::H265StRefPicSetParser::
    StRefPicSetState> OptionalStRefPicSet;
}  // namespace

namespace h265nal {

// General note: this is based off the 2016/12 version of the H.265 standard.
// You can find it on this page:
// http://www.itu.int/rec/T-REC-H.265

// Unpack RBSP and parse st_ref_pic_set state from the supplied buffer.
absl::optional<H265StRefPicSetParser::StRefPicSetState>
H265StRefPicSetParser::ParseStRefPicSet(
    const uint8_t* data, size_t length,
    uint32_t stRpsIdx, uint32_t num_short_term_ref_pic_sets) {

  std::vector<uint8_t> unpacked_buffer = UnescapeRbsp(data, length);
  rtc::BitBuffer bit_buffer(unpacked_buffer.data(), unpacked_buffer.size());
  return ParseStRefPicSet(&bit_buffer, stRpsIdx, num_short_term_ref_pic_sets);
}


absl::optional<H265StRefPicSetParser::StRefPicSetState>
H265StRefPicSetParser::ParseStRefPicSet(
    rtc::BitBuffer* bit_buffer,
    uint32_t stRpsIdx, uint32_t num_short_term_ref_pic_sets) {
  uint32_t bits_tmp;
  uint32_t golomb_tmp;

  // H265 st_ref_pic_set() NAL Unit.
  // Section 7.3.7 ("Short-term reference picture set syntax parameter set
  // syntax") of the H.265 standard for a complete description.
  StRefPicSetState st_ref_pic_set;

  st_ref_pic_set.stRpsIdx = stRpsIdx;
  st_ref_pic_set.num_short_term_ref_pic_sets = num_short_term_ref_pic_sets;

  if (stRpsIdx != 0) {
    // inter_ref_pic_set_prediction_flag  u(1)
    if (!bit_buffer->ReadBits(
        &(st_ref_pic_set.inter_ref_pic_set_prediction_flag), 1)) {
      return absl::nullopt;
    }
  }

  if (st_ref_pic_set.inter_ref_pic_set_prediction_flag) {
    if (stRpsIdx == num_short_term_ref_pic_sets) {
      // delta_idx_minus1  ue(v)
      if (!bit_buffer->ReadExponentialGolomb(
          &(st_ref_pic_set.delta_idx_minus1))) {
        return absl::nullopt;
      }

      // delta_rps_sign  u(1)
      if (!bit_buffer->ReadBits(&(st_ref_pic_set.delta_rps_sign), 1)) {
        return absl::nullopt;
      }

      // abs_delta_rps_minus1  ue(v)
      if (!bit_buffer->ReadExponentialGolomb(
          &(st_ref_pic_set.abs_delta_rps_minus1))) {
        return absl::nullopt;
      }

#if 0
      // TODO(chemag): add support for NumDeltaPocs
      // NumDeltaPocs[stRpsIdx] = NumNegativePics[stRpsIdx] +
      //                          NumPositivePics[stRpsIdx]

      // NumNegativePics[stRpsIdx] = num_negative_pics
      // NumPositivePics[stRpsIdx] = num_positive_pics

      for (j = 0; j <= NumDeltaPocs[RefRpsIdx]; j++) {
        // used_by_curr_pic_flag[j]  u(1)
        if (!used_by_curr_pic_flag[j]) {
          // use_delta_flag[j]  u(1)
        }
      }
#endif
    }

  } else {
    // num_negative_pics  ue(v)
    if (!bit_buffer->ReadExponentialGolomb(
        &(st_ref_pic_set.num_negative_pics))) {
      return absl::nullopt;
    }

    // num_positive_pics  ue(v)
    if (!bit_buffer->ReadExponentialGolomb(
        &(st_ref_pic_set.num_positive_pics))) {
      return absl::nullopt;
    }

    for (uint32_t i = 0; i < st_ref_pic_set.num_negative_pics; i++) {
      // delta_poc_s0_minus1[i] ue(v)
      if (!bit_buffer->ReadExponentialGolomb(&golomb_tmp)) {
        return absl::nullopt;
      }
      st_ref_pic_set.delta_poc_s0_minus1.push_back(golomb_tmp);

      // used_by_curr_pic_s0_flag[i] u(1)
      if (!bit_buffer->ReadBits(&bits_tmp, 1)) {
        return absl::nullopt;
      }
      st_ref_pic_set.used_by_curr_pic_s0_flag.push_back(bits_tmp);
    }

    for (uint32_t i = 0; i < st_ref_pic_set.num_positive_pics; i++) {
      // delta_poc_s1_minus1[i] ue(v)
      if (!bit_buffer->ReadExponentialGolomb(&golomb_tmp)) {
        return absl::nullopt;
      }
      st_ref_pic_set.delta_poc_s1_minus1.push_back(golomb_tmp);

      // used_by_curr_pic_s1_flag[i] u(1)
      if (!bit_buffer->ReadBits(&bits_tmp, 1)) {
        return absl::nullopt;
      }
      st_ref_pic_set.used_by_curr_pic_s1_flag.push_back(bits_tmp);
    }
  }

  return OptionalStRefPicSet(st_ref_pic_set);
}

void H265StRefPicSetParser::StRefPicSetState::fdump(
    XFILE outfp, int indent_level) const {
  XPrintf(outfp, "st_ref_pic_set {");
  indent_level = indent_level_incr(indent_level);

  if (stRpsIdx != 0) {
    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "inter_ref_pic_set_prediction_flag: %i",
            inter_ref_pic_set_prediction_flag);
  }

  if (inter_ref_pic_set_prediction_flag) {
    if (stRpsIdx == num_short_term_ref_pic_sets) {
      fdump_indent_level(outfp, indent_level);
      XPrintf(outfp, "delta_idx_minus1: %i", delta_idx_minus1);
    }

    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "delta_rps_sign: %i", delta_rps_sign);

    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "abs_delta_rps_minus1: %i", abs_delta_rps_minus1);

#if 0
    // TODO(chemag): add support for NumDeltaPocs
    for (uint32_t j = 0; j <= NumDeltaPocs[RefRpsIdx]; j++) {
      // used_by_curr_pic_flag[j] u(1)
      if (!used_by_curr_pic_flag[j]) {
        // use_delta_flag[j] u(1)
      }
    }
#endif

  } else {
    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "num_negative_pics: %i", num_negative_pics);

    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "num_positive_pics: %i", num_positive_pics);

    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "delta_poc_s0_minus1 {");
    for (const uint32_t& v : delta_poc_s0_minus1) {
      XPrintf(outfp, " %i", v);
    }
    XPrintf(outfp, " }");

    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "used_by_curr_pic_s0_flag {");
    for (const uint32_t& v : used_by_curr_pic_s0_flag) {
      XPrintf(outfp, " %i", v);
    }
    XPrintf(outfp, " }");

    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "delta_poc_s1_minus1 {");
    for (const uint32_t& v : delta_poc_s1_minus1) {
      XPrintf(outfp, " %i", v);
    }
    XPrintf(outfp, " }");

    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "used_by_curr_pic_s1_flag {");
    for (const uint32_t& v : used_by_curr_pic_s1_flag) {
      XPrintf(outfp, " %i", v);
    }
    XPrintf(outfp, " }");
  }

  indent_level = indent_level_decr(indent_level);
  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "}");
}

}  // namespace h265nal
