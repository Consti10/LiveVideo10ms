/*
 *  Copyright (c) Facebook, Inc. and its affiliates.
 */


#include "h265_st_ref_pic_set_parser.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "h265_common.h"
#include "absl/types/optional.h"
#include "rtc_base/arraysize.h"
#include "rtc_base/bit_buffer.h"

namespace h265nal {

class H265StRefPicSetParserTest : public ::testing::Test {
 public:
  H265StRefPicSetParserTest() {}
  ~H265StRefPicSetParserTest() override {}

  absl::optional<H265StRefPicSetParser::StRefPicSetState> st_ref_pic_set_;
};

TEST_F(H265StRefPicSetParserTest, TestSampleStRefPicSet) {
  // st_ref_pic_set
  const uint8_t buffer[] = {
      0x5d
  };

  st_ref_pic_set_ = H265StRefPicSetParser::ParseStRefPicSet(
      buffer, arraysize(buffer), 0, 1);
  EXPECT_TRUE(st_ref_pic_set_ != absl::nullopt);

  EXPECT_EQ(1, st_ref_pic_set_->num_negative_pics);
  EXPECT_EQ(0, st_ref_pic_set_->num_positive_pics);
  EXPECT_THAT(st_ref_pic_set_->delta_poc_s0_minus1,
              ::testing::ElementsAreArray({0}));
  EXPECT_THAT(st_ref_pic_set_->delta_poc_s0_minus1,
              ::testing::ElementsAreArray({0}));
}

}  // namespace h265nal
