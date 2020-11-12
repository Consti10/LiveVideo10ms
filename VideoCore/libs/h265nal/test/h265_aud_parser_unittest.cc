/*
 *  Copyright (c) Facebook, Inc. and its affiliates.
 */


#include "h265_aud_parser.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "h265_common.h"
#include "absl/types/optional.h"
#include "rtc_base/arraysize.h"
#include "rtc_base/bit_buffer.h"

namespace h265nal {

class H265AudParserTest : public ::testing::Test {
 public:
  H265AudParserTest() {}
  ~H265AudParserTest() override {}

  absl::optional<H265AudParser::AudState> aud_;
};

TEST_F(H265AudParserTest, TestSampleAUD) {
  const uint8_t buffer[] = {
      0xff
  };
  aud_ = H265AudParser::ParseAud(buffer, arraysize(buffer));
  EXPECT_TRUE(aud_ != absl::nullopt);

  EXPECT_EQ(7, aud_->pic_type);
}

}  // namespace h265nal
