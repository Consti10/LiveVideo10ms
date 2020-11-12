/*
 *  Copyright (c) Facebook, Inc. and its affiliates.
 */


#include "h265_vps_parser.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "h265_common.h"
#include "absl/types/optional.h"
#include "rtc_base/arraysize.h"
#include "rtc_base/bit_buffer.h"

namespace h265nal {

class H265VpsParserTest : public ::testing::Test {
 public:
  H265VpsParserTest() {}
  ~H265VpsParserTest() override {}

  absl::optional<H265VpsParser::VpsState> vps_;
};

TEST_F(H265VpsParserTest, TestSampleVPS) {
  const uint8_t buffer[] = {
      0x0c, 0x01, 0xff, 0xff, 0x01, 0x60, 0x00, 0x00,
      0x03, 0x00, 0xb0, 0x00, 0x00, 0x03, 0x00, 0x00,
      0x03, 0x00, 0x5d, 0xac, 0x59, 0x00
  };
  vps_ = H265VpsParser::ParseVps(buffer, arraysize(buffer));
  EXPECT_TRUE(vps_ != absl::nullopt);

  EXPECT_EQ(0, vps_->vps_video_parameter_set_id);
  EXPECT_EQ(1, vps_->vps_base_layer_internal_flag);
  EXPECT_EQ(1, vps_->vps_base_layer_available_flag);
  EXPECT_EQ(0, vps_->vps_max_layers_minus1);
  EXPECT_EQ(0, vps_->vps_max_sub_layers_minus1);
  EXPECT_EQ(1, vps_->vps_temporal_id_nesting_flag);
  EXPECT_EQ(0xffff, vps_->vps_reserved_0xffff_16bits);
  // profile_tier_level start
  EXPECT_EQ(0, vps_->profile_tier_level.general.profile_space);
  EXPECT_EQ(0, vps_->profile_tier_level.general.tier_flag);
  EXPECT_EQ(1, vps_->profile_tier_level.general.profile_idc);
  EXPECT_THAT(vps_->profile_tier_level.general.profile_compatibility_flag,
              ::testing::ElementsAreArray(
              {0, 1, 1, 0, 0, 0, 0, 0,
               0, 0, 0, 0, 0, 0, 0, 0,
               0, 0, 0, 0, 0, 0, 0, 0,
               0, 0, 0, 0, 0, 0, 0, 0}));
  EXPECT_EQ(1, vps_->profile_tier_level.general.progressive_source_flag);
  EXPECT_EQ(0, vps_->profile_tier_level.general.interlaced_source_flag);
  EXPECT_EQ(1, vps_->profile_tier_level.general.non_packed_constraint_flag);
  EXPECT_EQ(1, vps_->profile_tier_level.general.frame_only_constraint_flag);
  EXPECT_EQ(0, vps_->profile_tier_level.general.max_12bit_constraint_flag);
  EXPECT_EQ(0, vps_->profile_tier_level.general.max_10bit_constraint_flag);
  EXPECT_EQ(0, vps_->profile_tier_level.general.max_8bit_constraint_flag);
  EXPECT_EQ(0, vps_->profile_tier_level.general.max_422chroma_constraint_flag);
  EXPECT_EQ(0, vps_->profile_tier_level.general.max_420chroma_constraint_flag);
  EXPECT_EQ(0, vps_->profile_tier_level.general.max_monochrome_constraint_flag);
  EXPECT_EQ(0, vps_->profile_tier_level.general.intra_constraint_flag);
  EXPECT_EQ(0,
            vps_->profile_tier_level.general.one_picture_only_constraint_flag);
  EXPECT_EQ(0, vps_->profile_tier_level.general.lower_bit_rate_constraint_flag);
  EXPECT_EQ(0, vps_->profile_tier_level.general.max_14bit_constraint_flag);
  EXPECT_EQ(0, vps_->profile_tier_level.general.reserved_zero_33bits);
  EXPECT_EQ(0, vps_->profile_tier_level.general.reserved_zero_34bits);
  EXPECT_EQ(0, vps_->profile_tier_level.general.reserved_zero_43bits);
  EXPECT_EQ(0, vps_->profile_tier_level.general.inbld_flag);
  EXPECT_EQ(0, vps_->profile_tier_level.general.reserved_zero_bit);
  EXPECT_EQ(93, vps_->profile_tier_level.general_level_idc);
  EXPECT_EQ(0, vps_->profile_tier_level.sub_layer_profile_present_flag.size());
  EXPECT_EQ(0, vps_->profile_tier_level.sub_layer_level_present_flag.size());
  EXPECT_EQ(0, vps_->profile_tier_level.reserved_zero_2bits.size());
  EXPECT_EQ(0, vps_->profile_tier_level.sub_layer.size());
  // profile_tier_level end
  EXPECT_EQ(1, vps_->vps_sub_layer_ordering_info_present_flag);
  EXPECT_THAT(vps_->vps_max_dec_pic_buffering_minus1,
              ::testing::ElementsAreArray({1}));
  EXPECT_THAT(vps_->vps_max_num_reorder_pics,
              ::testing::ElementsAreArray({0}));
  EXPECT_THAT(vps_->vps_max_latency_increase_plus1,
              ::testing::ElementsAreArray({0}));
  EXPECT_EQ(5, vps_->vps_max_layer_id);
  EXPECT_EQ(0, vps_->vps_num_layer_sets_minus1);
  EXPECT_EQ(0, vps_->layer_id_included_flag.size());
  EXPECT_EQ(0, vps_->vps_timing_info_present_flag);
  EXPECT_EQ(0, vps_->vps_num_units_in_tick);
  EXPECT_EQ(0, vps_->vps_time_scale);
  EXPECT_EQ(0, vps_->vps_poc_proportional_to_timing_flag);
  EXPECT_EQ(0, vps_->vps_num_ticks_poc_diff_one_minus1);
  EXPECT_EQ(0, vps_->vps_num_hrd_parameters);
  EXPECT_EQ(0, vps_->hrd_layer_set_idx.size());
  EXPECT_EQ(0, vps_->cprms_present_flag.size());
  EXPECT_EQ(0, vps_->vps_extension_flag);
  EXPECT_EQ(0, vps_->vps_extension_data_flag);
}

}  // namespace h265nal
