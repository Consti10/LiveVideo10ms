/*
 *  Copyright (c) Facebook, Inc. and its affiliates.
 */


#include "h265_bitstream_parser.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "h265_common.h"
#include "absl/types/optional.h"
#include "rtc_base/arraysize.h"
#include "rtc_base/bit_buffer.h"

namespace h265nal {

class H265BitstreamParserTest : public ::testing::Test {
 public:
  H265BitstreamParserTest() {}
  ~H265BitstreamParserTest() override {}

  absl::optional<H265BitstreamParser::BitstreamState> bitstream_;
};

TEST_F(H265BitstreamParserTest, TestSampleBitstream) {
  // VPS, SPS, PPS for a 1280x720 camera capture.
  const uint8_t buffer[] = {
      // VPS
      0x00, 0x00, 0x00, 0x01, 0x40, 0x01, 0x0c, 0x01,
      0xff, 0xff, 0x01, 0x60, 0x00, 0x00, 0x03, 0x00,
      0xb0, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00,
      0x5d, 0xac, 0x59, 0x00,
      // SPS
      0x00, 0x00, 0x01, 0x42, 0x01, 0x01, 0x01, 0x60,
      0x00, 0x00, 0x03, 0x00, 0xb0, 0x00, 0x00, 0x03,
      0x00, 0x00, 0x03, 0x00, 0x5d, 0xa0, 0x02, 0x80,
      0x80, 0x2e, 0x1f, 0x13, 0x96, 0xbb, 0x93, 0x24,
      0xbb, 0x95, 0x82, 0x83, 0x03, 0x01, 0x76, 0x85,
      0x09, 0x40, 0x00,
      // PPS
      0x00, 0x00, 0x01, 0x44, 0x01, 0xc0, 0xf3, 0xc0,
      0x02, 0x10, 0x00,
      // slice
      0x00, 0x00, 0x01, 0x26, 0x01, 0xaf, 0x09, 0x40,
      0xf3, 0xb8, 0xd5, 0x39, 0xba, 0x1f, 0xe4, 0xa6,
      0x08, 0x5c, 0x6e, 0xb1, 0x8f, 0x00, 0x38, 0xf1,
      0xa6, 0xfc, 0xf1, 0x40, 0x04, 0x3a, 0x86, 0xcb,
      0x90, 0x74, 0xce, 0xf0, 0x46, 0x61, 0x93, 0x72,
      0xd6, 0xfc, 0x35, 0xe3, 0xc5, 0x6f, 0x0a, 0xc4,
      0x9e, 0x27, 0xc4, 0xdb, 0xe3, 0xfb, 0x38, 0x98,
      0xd0, 0x8b, 0xd5, 0xb9, 0xb9, 0x15, 0xb4, 0x92,
      0x49, 0x97, 0xe5, 0x3d, 0x36, 0x4d, 0x45, 0x32,
      0x5c, 0xe6, 0x89, 0x53, 0x76, 0xce, 0xbb, 0x83,
      0xa1, 0x27, 0x35, 0xfb, 0xf3, 0xc7, 0xd4, 0x85,
      0x32, 0x37, 0x94, 0x09, 0xec, 0x10
  };
  bitstream_ = H265BitstreamParser::ParseBitstream(buffer, arraysize(buffer));
  EXPECT_TRUE(bitstream_ != absl::nullopt);

  // check there are 4 NAL units
  EXPECT_EQ(4, bitstream_->nal_units.size());

  // check the 1st NAL unit
  EXPECT_EQ(0, bitstream_->nal_units[0].nal_unit_header.forbidden_zero_bit);
  EXPECT_EQ(NalUnitType::VPS_NUT,
      bitstream_->nal_units[0].nal_unit_header.nal_unit_type);
  EXPECT_EQ(0, bitstream_->nal_units[0].nal_unit_header.nuh_layer_id);
  EXPECT_EQ(1, bitstream_->nal_units[0].nal_unit_header.nuh_temporal_id_plus1);

  // check the 2nd NAL unit
  EXPECT_EQ(0, bitstream_->nal_units[1].nal_unit_header.forbidden_zero_bit);
  EXPECT_EQ(NalUnitType::SPS_NUT,
      bitstream_->nal_units[1].nal_unit_header.nal_unit_type);
  EXPECT_EQ(0, bitstream_->nal_units[1].nal_unit_header.nuh_layer_id);
  EXPECT_EQ(1, bitstream_->nal_units[1].nal_unit_header.nuh_temporal_id_plus1);

  // check the 3rd NAL unit
  EXPECT_EQ(0, bitstream_->nal_units[2].nal_unit_header.forbidden_zero_bit);
  EXPECT_EQ(NalUnitType::PPS_NUT,
      bitstream_->nal_units[2].nal_unit_header.nal_unit_type);
  EXPECT_EQ(0, bitstream_->nal_units[2].nal_unit_header.nuh_layer_id);
  EXPECT_EQ(1, bitstream_->nal_units[2].nal_unit_header.nuh_temporal_id_plus1);

  // check the 4th NAL unit
  EXPECT_EQ(0, bitstream_->nal_units[3].nal_unit_header.forbidden_zero_bit);
  EXPECT_EQ(NalUnitType::IDR_W_RADL,
      bitstream_->nal_units[3].nal_unit_header.nal_unit_type);
  EXPECT_EQ(0, bitstream_->nal_units[3].nal_unit_header.nuh_layer_id);
  EXPECT_EQ(1, bitstream_->nal_units[3].nal_unit_header.nuh_temporal_id_plus1);
}

}  // namespace h265nal
