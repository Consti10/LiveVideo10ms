/*
 *  Copyright (c) Facebook, Inc. and its affiliates.
 */


#include "h265_vui_parameters_parser.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "h265_common.h"
#include "absl/types/optional.h"
#include "rtc_base/arraysize.h"
#include "rtc_base/bit_buffer.h"

namespace h265nal {

class H265VuiParametersParserTest : public ::testing::Test {
 public:
  H265VuiParametersParserTest() {}
  ~H265VuiParametersParserTest() override {}

  absl::optional<H265VuiParametersParser::VuiParametersState> vui_parameters_;
};

TEST_F(H265VuiParametersParserTest, TestSampleVuiParameters) {
  // VUI
  const uint8_t buffer[] = {
      0x2b, 0x05, 0x06, 0x06, 0x02, 0xed, 0x0a, 0x12
  };
  vui_parameters_ = H265VuiParametersParser::ParseVuiParameters(
      buffer, arraysize(buffer));
  EXPECT_TRUE(vui_parameters_ != absl::nullopt);

  EXPECT_EQ(0, vui_parameters_->aspect_ratio_info_present_flag);
  EXPECT_EQ(0, vui_parameters_->overscan_info_present_flag);
  EXPECT_EQ(1, vui_parameters_->video_signal_type_present_flag);
  EXPECT_EQ(2, vui_parameters_->video_format);
  EXPECT_EQ(1, vui_parameters_->video_full_range_flag);
  EXPECT_EQ(1, vui_parameters_->colour_description_present_flag);
  EXPECT_EQ(5, vui_parameters_->colour_primaries);
  EXPECT_EQ(6, vui_parameters_->transfer_characteristics);
  EXPECT_EQ(6, vui_parameters_->matrix_coeffs);
  EXPECT_EQ(0, vui_parameters_->chroma_loc_info_present_flag);
  EXPECT_EQ(0, vui_parameters_->neutral_chroma_indication_flag);
  EXPECT_EQ(0, vui_parameters_->field_seq_flag);
  EXPECT_EQ(0, vui_parameters_->frame_field_info_present_flag);
  EXPECT_EQ(0, vui_parameters_->default_display_window_flag);
  EXPECT_EQ(0, vui_parameters_->vui_timing_info_present_flag);
  EXPECT_EQ(1, vui_parameters_->bitstream_restriction_flag);
  EXPECT_EQ(0, vui_parameters_->tiles_fixed_structure_flag);
  EXPECT_EQ(1, vui_parameters_->motion_vectors_over_pic_boundaries_flag);
  EXPECT_EQ(1, vui_parameters_->restricted_ref_pic_lists_flag);
  EXPECT_EQ(0, vui_parameters_->min_spatial_segmentation_idc);
  EXPECT_EQ(2, vui_parameters_->max_bytes_per_pic_denom);
  EXPECT_EQ(1, vui_parameters_->max_bits_per_min_cu_denom);
  EXPECT_EQ(9, vui_parameters_->log2_max_mv_length_horizontal);
  EXPECT_EQ(8, vui_parameters_->log2_max_mv_length_vertical);
}

}  // namespace h265nal
