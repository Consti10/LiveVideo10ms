/*
 *  Copyright (c) Facebook, Inc. and its affiliates.
 */

#include "h265_profile_tier_level_parser.h"

#include <stdio.h>

#include <cstdint>
#include <vector>

#include "h265_common.h"
#include "absl/types/optional.h"

namespace {
typedef absl::optional<h265nal::H265ProfileInfoParser::ProfileInfoState>
    OptionalProfileInfo;
typedef absl::optional<h265nal::H265ProfileTierLevelParser::
    ProfileTierLevelState> OptionalProfileTierLevel;
}  // namespace

namespace h265nal {

// General note: this is based off the 2016/12 version of the H.265 standard.
// You can find it on this page:
// http://www.itu.int/rec/T-REC-H.265

// Unpack RBSP and parse PROFILE_TIER_LEVEL state from the supplied buffer.
absl::optional<H265ProfileTierLevelParser::ProfileTierLevelState>
H265ProfileTierLevelParser::ParseProfileTierLevel(
    const uint8_t* data, size_t length, const bool profilePresentFlag,
    const unsigned int maxNumSubLayersMinus1) {

  std::vector<uint8_t> unpacked_buffer = UnescapeRbsp(data, length);
  rtc::BitBuffer bit_buffer(unpacked_buffer.data(), unpacked_buffer.size());

  return ParseProfileTierLevel(&bit_buffer, profilePresentFlag,
                               maxNumSubLayersMinus1);
}

absl::optional<H265ProfileTierLevelParser::ProfileTierLevelState>
H265ProfileTierLevelParser::ParseProfileTierLevel(
    rtc::BitBuffer* bit_buffer, const bool profilePresentFlag,
    const unsigned int maxNumSubLayersMinus1) {
  uint32_t bits_tmp;

  // profile_tier_level() parser.
  // Section 7.3.3 ("Profile, tier and level syntax") of the H.265
  // standard for a complete description.
  ProfileTierLevelState profile_tier_level;

  // input parameters
  profile_tier_level.profilePresentFlag = profilePresentFlag;
  profile_tier_level.maxNumSubLayersMinus1 = maxNumSubLayersMinus1;

  if (profilePresentFlag) {
    OptionalProfileInfo profile_info = H265ProfileInfoParser::ParseProfileInfo(
        bit_buffer);
    if (profile_info != absl::nullopt) {
      profile_tier_level.general = *profile_info;
    }
  }

  // general_level_idc  u(8)
  if (!bit_buffer->ReadBits(&(profile_tier_level.general_level_idc), 8)) {
    return absl::nullopt;
  }

  for (uint32_t i = 0; i < maxNumSubLayersMinus1; i++) {
    // sub_layer_profile_present_flag[i]  u(1)
    if (!bit_buffer->ReadBits(&bits_tmp, 1)) {
      return absl::nullopt;
    }
    profile_tier_level.sub_layer_profile_present_flag.push_back(bits_tmp);

    // sub_layer_level_present_flag[i]  u(1)
    if (!bit_buffer->ReadBits(&bits_tmp, 1)) {
      return absl::nullopt;
    }
    profile_tier_level.sub_layer_level_present_flag.push_back(bits_tmp);
  }

  if (maxNumSubLayersMinus1 > 0) {
    for (uint32_t i = maxNumSubLayersMinus1; i < 8; i++) {
      // reserved_zero_2bits[i]  u(2)
      if (!bit_buffer->ReadBits(&bits_tmp, 2)) {
        return absl::nullopt;
      }
      profile_tier_level.reserved_zero_2bits.push_back(bits_tmp);
    }
  }

  for (uint32_t i = 0; i < maxNumSubLayersMinus1; i++) {
    OptionalProfileInfo profile_info =
        H265ProfileInfoParser::ParseProfileInfo(bit_buffer);
    if (profile_info != absl::nullopt) {
      profile_tier_level.sub_layer.push_back(*profile_info);
    }

    // sub_layer_level_idc  u(8)
    if (!bit_buffer->ReadBits(&bits_tmp, 8)) {
      return absl::nullopt;
    }
    profile_tier_level.sub_layer_level_idc.push_back(bits_tmp);
  }

  return OptionalProfileTierLevel(profile_tier_level);
}


absl::optional<H265ProfileInfoParser::ProfileInfoState>
H265ProfileInfoParser::ParseProfileInfo(const uint8_t* data, size_t length) {
  std::vector<uint8_t> unpacked_buffer = UnescapeRbsp(data, length);
  rtc::BitBuffer bit_buffer(unpacked_buffer.data(), unpacked_buffer.size());
  return ParseProfileInfo(&bit_buffer);
}


absl::optional<H265ProfileInfoParser::ProfileInfoState>
H265ProfileInfoParser::ParseProfileInfo(rtc::BitBuffer* bit_buffer) {
  ProfileInfoState profile_info;
  uint32_t bits_tmp, bits_tmp_hi;

  // profile_space  u(2)
  if (!bit_buffer->ReadBits(&profile_info.profile_space, 2)) {
    return absl::nullopt;
  }
  // tier_flag  u(1)
  if (!bit_buffer->ReadBits(&profile_info.tier_flag, 1)) {
    return absl::nullopt;
  }
  // profile_idc  u(5)
  if (!bit_buffer->ReadBits(&profile_info.profile_idc, 5)) {
    return absl::nullopt;
  }
  // for (j = 0; j < 32; j++)
  for (uint32_t j = 0; j < 32; j++) {
    // profile_compatibility_flag[j]  u(1)
    if (!bit_buffer->ReadBits(&profile_info.profile_compatibility_flag[j], 1)) {
      return absl::nullopt;
    }
  }
  // progressive_source_flag  u(1)
  if (!bit_buffer->ReadBits(&profile_info.progressive_source_flag, 1)) {
    return absl::nullopt;
  }
  // interlaced_source_flag  u(1)
  if (!bit_buffer->ReadBits(&profile_info.interlaced_source_flag, 1)) {
    return absl::nullopt;
  }
  // non_packed_constraint_flag  u(1)
  if (!bit_buffer->ReadBits(&profile_info.non_packed_constraint_flag, 1)) {
    return absl::nullopt;
  }
  // frame_only_constraint_flag  u(1)
  if (!bit_buffer->ReadBits(&profile_info.frame_only_constraint_flag, 1)) {
    return absl::nullopt;
  }
  if (profile_info.profile_idc == 4 ||
      profile_info.profile_compatibility_flag[4] == 1 ||
      profile_info.profile_idc == 5 ||
      profile_info.profile_compatibility_flag[5] == 1 ||
      profile_info.profile_idc == 6 ||
      profile_info.profile_compatibility_flag[6] == 1 ||
      profile_info.profile_idc == 7 ||
      profile_info.profile_compatibility_flag[7] == 1 ||
      profile_info.profile_idc == 8 ||
      profile_info.profile_compatibility_flag[8] == 1 ||
      profile_info.profile_idc == 9 ||
      profile_info.profile_compatibility_flag[9] == 1 ||
      profile_info.profile_idc == 10 ||
      profile_info.profile_compatibility_flag[10] == 1) {
    // The number of bits in this syntax structure is not affected by
    // this condition
    // max_12bit_constraint_flag  u(1)
    if (!bit_buffer->ReadBits(&profile_info.max_12bit_constraint_flag, 1)) {
      return absl::nullopt;
    }
    // max_10bit_constraint_flag  u(1)
    if (!bit_buffer->ReadBits(&profile_info.max_10bit_constraint_flag, 1)) {
      return absl::nullopt;
    }
    // max_8bit_constraint_flag  u(1)
    if (!bit_buffer->ReadBits(&profile_info.max_8bit_constraint_flag, 1)) {
      return absl::nullopt;
    }
    // max_422chroma_constraint_flag  u(1)
    if (!bit_buffer->ReadBits(&profile_info.max_422chroma_constraint_flag, 1)) {
      return absl::nullopt;
    }
    // max_420chroma_constraint_flag  u(1)
    if (!bit_buffer->ReadBits(&profile_info.max_420chroma_constraint_flag, 1)) {
      return absl::nullopt;
    }
    // max_monochrome_constraint_flag  u(1)
    if (!bit_buffer->ReadBits(&profile_info.max_monochrome_constraint_flag,
                              1)) {
      return absl::nullopt;
    }
    // intra_constraint_flag  u(1)
    if (!bit_buffer->ReadBits(&profile_info.intra_constraint_flag, 1)) {
      return absl::nullopt;
    }
    // one_picture_only_constraint_flag  u(1)
    if (!bit_buffer->ReadBits(&profile_info.one_picture_only_constraint_flag,
                              1)) {
      return absl::nullopt;
    }
    // lower_bit_rate_constraint_flag  u(1)
    if (!bit_buffer->ReadBits(&profile_info.lower_bit_rate_constraint_flag,
                              1)) {
      return absl::nullopt;
    }
    if (profile_info.profile_idc == 5 ||
        profile_info.profile_compatibility_flag[5] == 1 ||
        profile_info.profile_idc == 9 ||
        profile_info.profile_compatibility_flag[9] == 1 ||
        profile_info.profile_idc == 10 ||
        profile_info.profile_compatibility_flag[10] == 1) {
      // max_14bit_constraint_flag  u(1)
      if (!bit_buffer->ReadBits(&profile_info.max_14bit_constraint_flag, 1)) {
        return absl::nullopt;
      }
      // reserved_zero_33bits  u(33)
      if (!bit_buffer->ReadBits(&bits_tmp_hi, 1)) {
        return absl::nullopt;
      }
      if (!bit_buffer->ReadBits(&bits_tmp, 32)) {
        return absl::nullopt;
      }
      profile_info.reserved_zero_33bits =
          ((uint64_t)bits_tmp_hi << 32) | bits_tmp;
    } else {
      // reserved_zero_34bits  u(34)
      if (!bit_buffer->ReadBits(&bits_tmp_hi, 2)) {
        return absl::nullopt;
      }
      if (!bit_buffer->ReadBits(&bits_tmp, 32)) {
        return absl::nullopt;
      }
      profile_info.reserved_zero_34bits =
          ((uint64_t)bits_tmp_hi << 32) | bits_tmp;
    }
  } else {
    // reserved_zero_43bits  u(43)
    if (!bit_buffer->ReadBits(&bits_tmp_hi, 11)) {
        return absl::nullopt;
      }
    if (!bit_buffer->ReadBits(&bits_tmp, 32)) {
        return absl::nullopt;
      }
    profile_info.reserved_zero_43bits =
        ((uint64_t)bits_tmp_hi << 32) | bits_tmp;
  }
  // The number of bits in this syntax structure is not affected by
  // this condition
  if ((profile_info.profile_idc >= 1 &&
       profile_info.profile_idc <= 5) ||
      profile_info.profile_idc == 9 ||
      profile_info.profile_compatibility_flag[1] == 1 ||
      profile_info.profile_compatibility_flag[2] == 1 ||
      profile_info.profile_compatibility_flag[3] == 1 ||
      profile_info.profile_compatibility_flag[4] == 1 ||
      profile_info.profile_compatibility_flag[5] == 1 ||
      profile_info.profile_compatibility_flag[9] == 1) {
    // inbld_flag  u(1)
    if (!bit_buffer->ReadBits(&profile_info.inbld_flag, 1)) {
      return absl::nullopt;
    }
  } else {
    // reserved_zero_bit  u(1)
    if (!bit_buffer->ReadBits(&profile_info.reserved_zero_bit, 1)) {
      return absl::nullopt;
    }
  }

  return OptionalProfileInfo(profile_info);
}

void H265ProfileInfoParser::ProfileInfoState::fdump(
        XFILE outfp, int indent_level) const {
  XPrintf(outfp, "profile_space: %i", profile_space);
  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "tier_flag: %i", tier_flag);
  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "profile_idc: %i", profile_idc);
  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "profile_compatibility_flag {");
  for (const uint32_t& v : profile_compatibility_flag) {
    XPrintf(outfp, " %i", v);
  }
  XPrintf(outfp, " }");
  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "progressive_source_flag: %i", progressive_source_flag);
  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "interlaced_source_flag: %i", interlaced_source_flag);
  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "non_packed_constraint_flag: %i",
          non_packed_constraint_flag);
  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "frame_only_constraint_flag: %i",
          frame_only_constraint_flag);
  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "max_12bit_constraint_flag: %i",
          max_12bit_constraint_flag);
  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "max_10bit_constraint_flag: %i",
          max_10bit_constraint_flag);
  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "max_8bit_constraint_flag: %i",
          max_8bit_constraint_flag);
  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "max_422chroma_constraint_flag: %i",
          max_422chroma_constraint_flag);
  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "max_420chroma_constraint_flag: %i",
          max_420chroma_constraint_flag);
  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "max_monochrome_constraint_flag: %i",
          max_monochrome_constraint_flag);
  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "intra_constraint_flag: %i", intra_constraint_flag);
  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "one_picture_only_constraint_flag: %i",
          one_picture_only_constraint_flag);
  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "lower_bit_rate_constraint_flag: %i",
          lower_bit_rate_constraint_flag);
  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "max_14bit_constraint_flag: %i", max_14bit_constraint_flag);
  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "reserved_zero_33bits: %llu", reserved_zero_33bits);
  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "reserved_zero_34bits: %llu", reserved_zero_34bits);
  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "reserved_zero_43bits: %llu", reserved_zero_43bits);
  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "inbld_flag: %i", inbld_flag);
  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "reserved_zero_bit: %i", reserved_zero_bit);
}

void H265ProfileTierLevelParser::ProfileTierLevelState::fdump(
        XFILE outfp, int indent_level) const {
  XPrintf(outfp, "profile_tier_level {");
  indent_level = indent_level_incr(indent_level);

  if (profilePresentFlag) {
    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "general {");
    indent_level = indent_level_incr(indent_level);

    fdump_indent_level(outfp, indent_level);
    general.fdump(outfp, indent_level);

    indent_level = indent_level_decr(indent_level);
    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "}");
  }

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "general_level_idc: %i", general_level_idc);

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "sub_layer_profile_present_flag {");
  for (const uint32_t& v : sub_layer_profile_present_flag) {
    XPrintf(outfp, " %i", v);
  }
  XPrintf(outfp, " }");

  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "sub_layer_level_present_flag {");
  for (const uint32_t& v : sub_layer_level_present_flag) {
    XPrintf(outfp, " %i ", v);
  }
  XPrintf(outfp, " }");

  if (maxNumSubLayersMinus1 > 0) {
    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "reserved_zero_2bits {");
    for (const uint32_t& v : reserved_zero_2bits) {
      XPrintf(outfp, " %i ", v);
    }
    XPrintf(outfp, " }");
  }

  if (maxNumSubLayersMinus1 > 0) {
    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "sub_layer {");
    indent_level = indent_level_incr(indent_level);

    for (const struct H265ProfileInfoParser::ProfileInfoState& v : sub_layer) {
      fdump_indent_level(outfp, indent_level);
      XPrintf(outfp, "{");
      v.fdump(outfp, indent_level);
      fdump_indent_level(outfp, indent_level);
      XPrintf(outfp, "}");
    }

    indent_level = indent_level_decr(indent_level);
    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "}");

    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "sub_layer_level_idc {");
    for (const uint32_t& v : sub_layer_level_idc) {
      XPrintf(outfp, " %i ", v);
    }
    XPrintf(outfp, " }");
  }

  indent_level = indent_level_decr(indent_level);
  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "}");
}

}  // namespace h265nal
