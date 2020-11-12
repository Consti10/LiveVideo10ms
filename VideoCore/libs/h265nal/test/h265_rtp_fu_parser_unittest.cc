/*
 *  Copyright (c) Facebook, Inc. and its affiliates.
 */


#include "h265_rtp_fu_parser.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "h265_common.h"
#include "absl/types/optional.h"
#include "rtc_base/arraysize.h"
#include "rtc_base/bit_buffer.h"

namespace h265nal {

class H265RtpFuParserTest : public ::testing::Test {
 public:
  H265RtpFuParserTest() {}
  ~H265RtpFuParserTest() override {}

  absl::optional<H265RtpFuParser::RtpFuState> rtp_fu_;
};

TEST_F(H265RtpFuParserTest, TestSampleStart) {
  // FU (Aggregation Packet) containing the start of an IDR_W_RADL.
  const uint8_t buffer[] = {
    0x62, 0x01, 0x93, 0xaf, 0x0d, 0x70, 0xfd, 0xf4,
    0x6e, 0xf0, 0x3c, 0x7e, 0x63, 0xc8, 0x15, 0xf5,
    0xf7, 0x6e, 0x52, 0x0f, 0xd3, 0xb5, 0x44, 0x61,
    0x58, 0x24, 0x68, 0xe0
  };
  H265BitstreamParserState bitstream_parser_state;
  rtp_fu_ = H265RtpFuParser::ParseRtpFu(
      buffer, arraysize(buffer),
      &bitstream_parser_state);
  EXPECT_TRUE(rtp_fu_ != absl::nullopt);

  // check the common header
  auto header = rtp_fu_->header;
  EXPECT_EQ(0, header.forbidden_zero_bit);
  EXPECT_EQ(NalUnitType::FU, header.nal_unit_type);
  EXPECT_EQ(0, header.nuh_layer_id);
  EXPECT_EQ(1, header.nuh_temporal_id_plus1);

  // check the fu header
  EXPECT_EQ(1, rtp_fu_->s_bit);
  EXPECT_EQ(0, rtp_fu_->e_bit);
  EXPECT_EQ(NalUnitType::IDR_W_RADL, rtp_fu_->fu_type);
}

TEST_F(H265RtpFuParserTest, TestSampleMiddle) {
  // FU (Aggregation Packet) containing the middle of an IDR_W_RADL.
  const uint8_t buffer[] = {
    0x62, 0x01, 0x13, 0x8e, 0xaa, 0x12, 0xcc, 0xef,
    0x6a, 0xf6, 0xb0, 0x7b, 0x7a, 0xbf, 0xea, 0xf1,
    0x3c, 0xa7, 0x20, 0xe8, 0x05, 0x9a, 0xfe, 0x6b
  };
  H265BitstreamParserState bitstream_parser_state;
  rtp_fu_ = H265RtpFuParser::ParseRtpFu(
      buffer, arraysize(buffer),
      &bitstream_parser_state);
  EXPECT_TRUE(rtp_fu_ != absl::nullopt);

  // check the common header
  auto header = rtp_fu_->header;
  EXPECT_EQ(0, header.forbidden_zero_bit);
  EXPECT_EQ(NalUnitType::FU, header.nal_unit_type);
  EXPECT_EQ(0, header.nuh_layer_id);
  EXPECT_EQ(1, header.nuh_temporal_id_plus1);

  // check the fu header
  EXPECT_EQ(0, rtp_fu_->s_bit);
  EXPECT_EQ(0, rtp_fu_->e_bit);
  EXPECT_EQ(NalUnitType::IDR_W_RADL, rtp_fu_->fu_type);
}

TEST_F(H265RtpFuParserTest, TestSampleEnd) {
  // FU (Aggregation Packet) containing the end of an IDR_W_RADL.
  const uint8_t buffer[] = {
    0x62, 0x01, 0x53, 0x85, 0xfe, 0xde, 0xe8, 0x9c,
    0x2f, 0xd5, 0xed, 0xb9, 0x2c, 0xec, 0x8f, 0x08,
    0x5b, 0x95, 0x02, 0x79, 0xc3, 0xb7, 0x47, 0x16
  };
  H265BitstreamParserState bitstream_parser_state;
  rtp_fu_ = H265RtpFuParser::ParseRtpFu(
      buffer, arraysize(buffer),
      &bitstream_parser_state);
  EXPECT_TRUE(rtp_fu_ != absl::nullopt);

  // check the common header
  auto header = rtp_fu_->header;
  EXPECT_EQ(0, header.forbidden_zero_bit);
  EXPECT_EQ(NalUnitType::FU, header.nal_unit_type);
  EXPECT_EQ(0, header.nuh_layer_id);
  EXPECT_EQ(1, header.nuh_temporal_id_plus1);

  // check the fu header
  EXPECT_EQ(0, rtp_fu_->s_bit);
  EXPECT_EQ(1, rtp_fu_->e_bit);
  EXPECT_EQ(NalUnitType::IDR_W_RADL, rtp_fu_->fu_type);
}

}  // namespace h265nal
