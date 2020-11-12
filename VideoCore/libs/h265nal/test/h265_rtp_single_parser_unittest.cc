/*
 *  Copyright (c) Facebook, Inc. and its affiliates.
 */


#include "h265_rtp_single_parser.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "h265_common.h"
#include "absl/types/optional.h"
#include "rtc_base/arraysize.h"
#include "rtc_base/bit_buffer.h"

namespace h265nal {

class H265RtpSingleParserTest : public ::testing::Test {
 public:
  H265RtpSingleParserTest() {}
  ~H265RtpSingleParserTest() override {}

  absl::optional<H265RtpSingleParser::RtpSingleState> rtp_single_;
};

TEST_F(H265RtpSingleParserTest, TestSampleVps) {
  // VPS for a 1280x720 camera capture.
  const uint8_t buffer[] = {0x40, 0x01, 0x0c, 0x01, 0xff, 0xff, 0x01, 0x60,
                            0x00, 0x00, 0x03, 0x00, 0xb0, 0x00, 0x00, 0x03,
                            0x00, 0x00, 0x03, 0x00, 0x5d, 0xac, 0x59, 0x00};
  H265BitstreamParserState bitstream_parser_state;
  rtp_single_ = H265RtpSingleParser::ParseRtpSingle(
      buffer, arraysize(buffer),
      &bitstream_parser_state);
  EXPECT_TRUE(rtp_single_ != absl::nullopt);

  // check the header
  EXPECT_EQ(0, rtp_single_->nal_unit_header.forbidden_zero_bit);
  EXPECT_EQ(NalUnitType::VPS_NUT, rtp_single_->nal_unit_header.nal_unit_type);
  EXPECT_EQ(0, rtp_single_->nal_unit_header.nuh_layer_id);
  EXPECT_EQ(1, rtp_single_->nal_unit_header.nuh_temporal_id_plus1);
}

}  // namespace h265nal
