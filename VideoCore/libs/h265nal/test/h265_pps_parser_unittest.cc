/*
 *  Copyright (c) Facebook, Inc. and its affiliates.
 */


#include "h265_pps_parser.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "h265_common.h"
#include "absl/types/optional.h"
#include "rtc_base/arraysize.h"
#include "rtc_base/bit_buffer.h"

namespace h265nal {

class H265PpsParserTest : public ::testing::Test {
 public:
  H265PpsParserTest() {}
  ~H265PpsParserTest() override {}

  absl::optional<H265PpsParser::PpsState> pps_;
};

TEST_F(H265PpsParserTest, TestSamplePPS) {
  // PPS example
  const uint8_t buffer[] = {
      0xc0, 0xf3, 0xc0, 0x02, 0x10, 0x00
  };

  pps_ = H265PpsParser::ParsePps(buffer, arraysize(buffer));
  EXPECT_TRUE(pps_ != absl::nullopt);

  EXPECT_EQ(0, pps_->pps_pic_parameter_set_id);
  EXPECT_EQ(0, pps_->pps_pic_parameter_set_id);
  EXPECT_EQ(0, pps_->pps_seq_parameter_set_id);
  EXPECT_EQ(0, pps_->dependent_slice_segments_enabled_flag);
  EXPECT_EQ(0, pps_->output_flag_present_flag);
  EXPECT_EQ(0, pps_->num_extra_slice_header_bits);
  EXPECT_EQ(0, pps_->sign_data_hiding_enabled_flag);
  EXPECT_EQ(1, pps_->cabac_init_present_flag);
  EXPECT_EQ(0, pps_->num_ref_idx_l0_default_active_minus1);
  EXPECT_EQ(0, pps_->num_ref_idx_l1_default_active_minus1);
  EXPECT_EQ(0, pps_->init_qp_minus26);
  EXPECT_EQ(0, pps_->constrained_intra_pred_flag);
  EXPECT_EQ(0, pps_->transform_skip_enabled_flag);
  EXPECT_EQ(1, pps_->cu_qp_delta_enabled_flag);
  EXPECT_EQ(0, pps_->diff_cu_qp_delta_depth);
  EXPECT_EQ(0, pps_->pps_cb_qp_offset);
  EXPECT_EQ(0, pps_->pps_cr_qp_offset);
  EXPECT_EQ(0, pps_->pps_slice_chroma_qp_offsets_present_flag);
  EXPECT_EQ(0, pps_->weighted_pred_flag);
  EXPECT_EQ(0, pps_->weighted_bipred_flag);
  EXPECT_EQ(0, pps_->transquant_bypass_enabled_flag);
  EXPECT_EQ(0, pps_->tiles_enabled_flag);
  EXPECT_EQ(0, pps_->entropy_coding_sync_enabled_flag);
  EXPECT_EQ(0, pps_->pps_loop_filter_across_slices_enabled_flag);
  EXPECT_EQ(0, pps_->deblocking_filter_control_present_flag);
  EXPECT_EQ(0, pps_->pps_scaling_list_data_present_flag);
  EXPECT_EQ(0, pps_->lists_modification_present_flag);
  EXPECT_EQ(3, pps_->log2_parallel_merge_level_minus2);
  EXPECT_EQ(0, pps_->slice_segment_header_extension_present_flag);
  EXPECT_EQ(0, pps_->pps_extension_present_flag);

#if 0
  EXPECT_EQ(1, pps_->rbsp_stop_one_bit);
#endif
}

}  // namespace h265nal
