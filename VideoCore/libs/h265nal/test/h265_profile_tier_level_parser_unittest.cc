/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "h265_profile_tier_level_parser.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "absl/types/optional.h"
#include "rtc_base/arraysize.h"
#include "rtc_base/bit_buffer.h"

namespace h265nal {

class H265ProfileTierLevelParserTest : public ::testing::Test {
 public:
  H265ProfileTierLevelParserTest() {}
  ~H265ProfileTierLevelParserTest() override {}

  absl::optional<H265ProfileTierLevelParser::ProfileTierLevelState> ptls_;
};

TEST_F(H265ProfileTierLevelParserTest, TestSampleValue) {
  const uint8_t buffer[] = {0x01, 0x60, 0x00, 0x00, 0x03, 0x00, 0xb0, 0x00,
                            0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x5d, 0xac,
                            0x59};
  ptls_ = H265ProfileTierLevelParser::ParseProfileTierLevel(
      buffer, arraysize(buffer), true, 0);
  EXPECT_TRUE(ptls_ != absl::nullopt);
  EXPECT_EQ(0, ptls_->general.profile_space);
  EXPECT_EQ(0, ptls_->general.tier_flag);
  EXPECT_EQ(1, ptls_->general.profile_idc);
  EXPECT_THAT(ptls_->general.profile_compatibility_flag,
              ::testing::ElementsAreArray(
              {0, 1, 1, 0, 0, 0, 0, 0,
               0, 0, 0, 0, 0, 0, 0, 0,
               0, 0, 0, 0, 0, 0, 0, 0,
               0, 0, 0, 0, 0, 0, 0, 0}));
  EXPECT_EQ(1, ptls_->general.progressive_source_flag);
  EXPECT_EQ(0, ptls_->general.interlaced_source_flag);
  EXPECT_EQ(1, ptls_->general.non_packed_constraint_flag);
  EXPECT_EQ(1, ptls_->general.frame_only_constraint_flag);
  EXPECT_EQ(0, ptls_->general.max_12bit_constraint_flag);
  EXPECT_EQ(0, ptls_->general.max_10bit_constraint_flag);
  EXPECT_EQ(0, ptls_->general.max_8bit_constraint_flag);
  EXPECT_EQ(0, ptls_->general.max_422chroma_constraint_flag);
  EXPECT_EQ(0, ptls_->general.max_420chroma_constraint_flag);
  EXPECT_EQ(0, ptls_->general.max_monochrome_constraint_flag);
  EXPECT_EQ(0, ptls_->general.intra_constraint_flag);
  EXPECT_EQ(0, ptls_->general.one_picture_only_constraint_flag);
  EXPECT_EQ(0, ptls_->general.lower_bit_rate_constraint_flag);
  EXPECT_EQ(0, ptls_->general.max_14bit_constraint_flag);
  EXPECT_EQ(0, ptls_->general.reserved_zero_33bits);
  EXPECT_EQ(0, ptls_->general.reserved_zero_34bits);
  EXPECT_EQ(0, ptls_->general.reserved_zero_43bits);
  EXPECT_EQ(0, ptls_->general.inbld_flag);
  EXPECT_EQ(0, ptls_->general.reserved_zero_bit);
  EXPECT_EQ(93, ptls_->general_level_idc);

  EXPECT_EQ(0, ptls_->sub_layer_profile_present_flag.size());
  EXPECT_EQ(0, ptls_->sub_layer_level_present_flag.size());
  EXPECT_EQ(0, ptls_->reserved_zero_2bits.size());
  EXPECT_EQ(0, ptls_->sub_layer.size());
}

}  // namespace h265nal
