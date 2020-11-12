/*
 *  Copyright (c) Facebook, Inc. and its affiliates.
 */


#include "h265_sps_parser.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "h265_common.h"
#include "absl/types/optional.h"
#include "rtc_base/arraysize.h"
#include "rtc_base/bit_buffer.h"

namespace h265nal {

class H265SpsParserTest : public ::testing::Test {
 public:
  H265SpsParserTest() {}
  ~H265SpsParserTest() override {}

  absl::optional<H265SpsParser::SpsState> sps_;
};

TEST_F(H265SpsParserTest, TestSampleSPS) {
  // SPS for a 1280x736 camera capture.
  const uint8_t buffer[] = {
      0x01, 0x01, 0x60, 0x00, 0x00, 0x03, 0x00, 0xb0,
      0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x5d,
      0xa0, 0x02, 0x80, 0x80, 0x2e, 0x1f, 0x13, 0x96,
      0xbb, 0x93, 0x24, 0xbb, 0x95, 0x82, 0x83, 0x03,
      0x01, 0x76, 0x85, 0x09, 0x40
  };

  sps_ = H265SpsParser::ParseSps(buffer, arraysize(buffer));
  EXPECT_TRUE(sps_ != absl::nullopt);

  EXPECT_EQ(0, sps_->sps_video_parameter_set_id);
  EXPECT_EQ(0, sps_->sps_max_sub_layers_minus1);
  EXPECT_EQ(1, sps_->sps_temporal_id_nesting_flag);
  // profile_tier_level start
  EXPECT_EQ(0, sps_->profile_tier_level.general.profile_space);
  EXPECT_EQ(0, sps_->profile_tier_level.general.tier_flag);
  EXPECT_EQ(1, sps_->profile_tier_level.general.profile_idc);
  EXPECT_THAT(sps_->profile_tier_level.general.profile_compatibility_flag,
              ::testing::ElementsAreArray(
              {0, 1, 1, 0, 0, 0, 0, 0,
               0, 0, 0, 0, 0, 0, 0, 0,
               0, 0, 0, 0, 0, 0, 0, 0,
               0, 0, 0, 0, 0, 0, 0, 0}));
  EXPECT_EQ(1, sps_->profile_tier_level.general.progressive_source_flag);
  EXPECT_EQ(0, sps_->profile_tier_level.general.interlaced_source_flag);
  EXPECT_EQ(1, sps_->profile_tier_level.general.non_packed_constraint_flag);
  EXPECT_EQ(1, sps_->profile_tier_level.general.frame_only_constraint_flag);
  EXPECT_EQ(0, sps_->profile_tier_level.general.max_12bit_constraint_flag);
  EXPECT_EQ(0, sps_->profile_tier_level.general.max_10bit_constraint_flag);
  EXPECT_EQ(0, sps_->profile_tier_level.general.max_8bit_constraint_flag);
  EXPECT_EQ(0, sps_->profile_tier_level.general.max_422chroma_constraint_flag);
  EXPECT_EQ(0, sps_->profile_tier_level.general.max_420chroma_constraint_flag);
  EXPECT_EQ(0, sps_->profile_tier_level.general.max_monochrome_constraint_flag);
  EXPECT_EQ(0, sps_->profile_tier_level.general.intra_constraint_flag);
  EXPECT_EQ(0,
            sps_->profile_tier_level.general.one_picture_only_constraint_flag);
  EXPECT_EQ(0, sps_->profile_tier_level.general.lower_bit_rate_constraint_flag);
  EXPECT_EQ(0, sps_->profile_tier_level.general.max_14bit_constraint_flag);
  EXPECT_EQ(0, sps_->profile_tier_level.general.reserved_zero_33bits);
  EXPECT_EQ(0, sps_->profile_tier_level.general.reserved_zero_34bits);
  EXPECT_EQ(0, sps_->profile_tier_level.general.reserved_zero_43bits);
  EXPECT_EQ(0, sps_->profile_tier_level.general.inbld_flag);
  EXPECT_EQ(0, sps_->profile_tier_level.general.reserved_zero_bit);
  EXPECT_EQ(93, sps_->profile_tier_level.general_level_idc);
  EXPECT_EQ(0, sps_->profile_tier_level.sub_layer_profile_present_flag.size());
  EXPECT_EQ(0, sps_->profile_tier_level.sub_layer_level_present_flag.size());
  EXPECT_EQ(0, sps_->profile_tier_level.reserved_zero_2bits.size());
  EXPECT_EQ(0, sps_->profile_tier_level.sub_layer.size());
  // profile_tier_level end
  EXPECT_EQ(0, sps_->sps_seq_parameter_set_id);
  EXPECT_EQ(1, sps_->chroma_format_idc);
  EXPECT_EQ(1280, sps_->pic_width_in_luma_samples);
  EXPECT_EQ(736, sps_->pic_height_in_luma_samples);
  EXPECT_EQ(1, sps_->conformance_window_flag);
  EXPECT_EQ(0, sps_->conf_win_left_offset);
  EXPECT_EQ(0, sps_->conf_win_right_offset);
  EXPECT_EQ(0, sps_->conf_win_top_offset);
  EXPECT_EQ(8, sps_->conf_win_bottom_offset);
  EXPECT_EQ(0, sps_->bit_depth_luma_minus8);
  EXPECT_EQ(0, sps_->bit_depth_chroma_minus8);
  EXPECT_EQ(4, sps_->log2_max_pic_order_cnt_lsb_minus4);
  EXPECT_EQ(1, sps_->sps_sub_layer_ordering_info_present_flag);
  EXPECT_THAT(sps_->sps_max_dec_pic_buffering_minus1,
              ::testing::ElementsAreArray({1}));
  EXPECT_THAT(sps_->sps_max_num_reorder_pics,
              ::testing::ElementsAreArray({0}));
  EXPECT_THAT(sps_->sps_max_latency_increase_plus1,
              ::testing::ElementsAreArray({0}));
  EXPECT_EQ(0, sps_->log2_min_luma_coding_block_size_minus3);
  EXPECT_EQ(2, sps_->log2_diff_max_min_luma_coding_block_size);
  EXPECT_EQ(0, sps_->log2_min_luma_transform_block_size_minus2);
  EXPECT_EQ(3, sps_->log2_diff_max_min_luma_transform_block_size);
  EXPECT_EQ(0, sps_->max_transform_hierarchy_depth_inter);
  EXPECT_EQ(0, sps_->max_transform_hierarchy_depth_intra);
  EXPECT_EQ(0, sps_->scaling_list_enabled_flag);
  EXPECT_EQ(0, sps_->amp_enabled_flag);
  EXPECT_EQ(1, sps_->sample_adaptive_offset_enabled_flag);
  EXPECT_EQ(0, sps_->pcm_enabled_flag);
  EXPECT_EQ(1, sps_->num_short_term_ref_pic_sets);

  // st_ref_pic_set(i)
  EXPECT_EQ(1, sps_->st_ref_pic_set[0].num_negative_pics);
  EXPECT_EQ(0, sps_->st_ref_pic_set[0].num_positive_pics);
  EXPECT_THAT(sps_->st_ref_pic_set[0].delta_poc_s0_minus1,
              ::testing::ElementsAreArray({0}));
  EXPECT_THAT(sps_->st_ref_pic_set[0].delta_poc_s0_minus1,
              ::testing::ElementsAreArray({0}));

  EXPECT_EQ(0, sps_->long_term_ref_pics_present_flag);
  EXPECT_EQ(1, sps_->sps_temporal_mvp_enabled_flag);
  EXPECT_EQ(1, sps_->strong_intra_smoothing_enabled_flag);
  EXPECT_EQ(1, sps_->vui_parameters_present_flag);

  // vui_parameters()
  EXPECT_EQ(0, sps_->vui_parameters.aspect_ratio_info_present_flag);
  EXPECT_EQ(0, sps_->vui_parameters.overscan_info_present_flag);
  EXPECT_EQ(1, sps_->vui_parameters.video_signal_type_present_flag);
  EXPECT_EQ(2, sps_->vui_parameters.video_format);
  EXPECT_EQ(1, sps_->vui_parameters.video_full_range_flag);
  EXPECT_EQ(1, sps_->vui_parameters.colour_description_present_flag);
  EXPECT_EQ(5, sps_->vui_parameters.colour_primaries);
  EXPECT_EQ(6, sps_->vui_parameters.transfer_characteristics);
  EXPECT_EQ(6, sps_->vui_parameters.matrix_coeffs);
  EXPECT_EQ(0, sps_->vui_parameters.chroma_loc_info_present_flag);
  EXPECT_EQ(0, sps_->vui_parameters.neutral_chroma_indication_flag);
  EXPECT_EQ(0, sps_->vui_parameters.field_seq_flag);
  EXPECT_EQ(0, sps_->vui_parameters.frame_field_info_present_flag);
  EXPECT_EQ(0, sps_->vui_parameters.default_display_window_flag);
  EXPECT_EQ(0, sps_->vui_parameters.vui_timing_info_present_flag);
  EXPECT_EQ(1, sps_->vui_parameters.bitstream_restriction_flag);
  EXPECT_EQ(0, sps_->vui_parameters.tiles_fixed_structure_flag);
  EXPECT_EQ(1, sps_->vui_parameters.motion_vectors_over_pic_boundaries_flag);
  EXPECT_EQ(1, sps_->vui_parameters.restricted_ref_pic_lists_flag);
  EXPECT_EQ(0, sps_->vui_parameters.min_spatial_segmentation_idc);
  EXPECT_EQ(2, sps_->vui_parameters.max_bytes_per_pic_denom);
  EXPECT_EQ(1, sps_->vui_parameters.max_bits_per_min_cu_denom);
  EXPECT_EQ(9, sps_->vui_parameters.log2_max_mv_length_horizontal);
  EXPECT_EQ(8, sps_->vui_parameters.log2_max_mv_length_vertical);

  EXPECT_EQ(0, sps_->sps_extension_present_flag);
#if 0
  EXPECT_EQ(1, sps_->rbsp_stop_one_bit);
  EXPECT_EQ(0, sps_->rbsp_alignment_zero_bit);
#endif

  // derived values
  EXPECT_EQ(920, sps_->getPicSizeInCtbsY());
}

}  // namespace h265nal
