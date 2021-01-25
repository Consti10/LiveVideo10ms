/*
 *  Copyright (c) Facebook, Inc. and its affiliates.
 */


#include "h265_nal_unit_parser.h"

#include <stdio.h>

#include <cstdint>
#include <vector>

#include "h265_common.h"
#include "h265_aud_parser.h"
#include "h265_pps_parser.h"
#include "h265_slice_parser.h"
#include "h265_sps_parser.h"
#include "h265_vps_parser.h"
#include "absl/types/optional.h"

namespace {
typedef absl::optional<h265nal::H265NalUnitHeaderParser::
    NalUnitHeaderState> OptionalNalUnitHeader;
typedef absl::optional<h265nal::H265NalUnitPayloadParser::
    NalUnitPayloadState> OptionalNalUnitPayload;
typedef absl::optional<h265nal::H265NalUnitParser::
    NalUnitState> OptionalNalUnit;
typedef absl::optional<h265nal::H265VpsParser::VpsState> OptionalVps;
typedef absl::optional<h265nal::H265SpsParser::SpsState> OptionalSps;
typedef absl::optional<h265nal::H265PpsParser::PpsState> OptionalPps;
typedef absl::optional<h265nal::H265AudParser::AudState> OptionalAud;
typedef absl::optional<
    h265nal::H265SliceSegmentLayerParser::SliceSegmentLayerState>
    OptionalSliceSegmentLayer;
}  // namespace

namespace h265nal {

// General note: this is based off the 2016/12 version of the H.265 standard.
// You can find it on this page:
// http://www.itu.int/rec/T-REC-H.265

// Unpack RBSP and parse NAL Unit state from the supplied buffer.
absl::optional<H265NalUnitParser::NalUnitState> H265NalUnitParser::ParseNalUnit(
    const uint8_t* data, size_t length,
    struct H265BitstreamParserState* bitstream_parser_state) {
  std::vector<uint8_t> unpacked_buffer = UnescapeRbsp(data, length);
  rtc::BitBuffer bit_buffer(unpacked_buffer.data(), unpacked_buffer.size());

  return ParseNalUnit(&bit_buffer, bitstream_parser_state);
}

absl::optional<H265NalUnitParser::NalUnitState> H265NalUnitParser::ParseNalUnit(
    rtc::BitBuffer* bit_buffer,
    struct H265BitstreamParserState* bitstream_parser_state) {
  // H265 NAL Unit (nal_unit()) parser.
  // Section 7.3.1.1 ("General NAL unit header syntax") of the H.265
  // standard for a complete description.
  NalUnitState nal_unit;

  // nal_unit_header()
  OptionalNalUnitHeader nal_unit_header =
      H265NalUnitHeaderParser::ParseNalUnitHeader(bit_buffer);
  if (nal_unit_header != absl::nullopt) {
    nal_unit.nal_unit_header = *nal_unit_header;
  }

  // nal_unit_payload()
  OptionalNalUnitPayload nal_unit_payload =
      H265NalUnitPayloadParser::ParseNalUnitPayload(
          bit_buffer, nal_unit.nal_unit_header.nal_unit_type,
          bitstream_parser_state);
  if (nal_unit_payload != absl::nullopt) {
    nal_unit.nal_unit_payload = *nal_unit_payload;
  }

  return OptionalNalUnit(nal_unit);
}

// Unpack RBSP and parse NAL Unit header state from the supplied buffer.
absl::optional<H265NalUnitHeaderParser::NalUnitHeaderState>
H265NalUnitHeaderParser::ParseNalUnitHeader(
    const uint8_t* data, size_t length) {
  std::vector<uint8_t> unpacked_buffer = UnescapeRbsp(data, length);
  rtc::BitBuffer bit_buffer(unpacked_buffer.data(), unpacked_buffer.size());

  return ParseNalUnitHeader(&bit_buffer);
}

absl::optional<H265NalUnitHeaderParser::NalUnitHeaderState>
H265NalUnitHeaderParser::ParseNalUnitHeader(
    rtc::BitBuffer* bit_buffer) {
  // H265 NAL Unit Header (nal_unit_header()) parser.
  // Section 7.3.1.2 ("NAL unit header syntax") of the H.265
  // standard for a complete description.
  NalUnitHeaderState nal_unit_header;

  // forbidden_zero_bit  f(1)
  if (!bit_buffer->ReadBits(&nal_unit_header.forbidden_zero_bit, 1)) {
    return absl::nullopt;
  }

  // nal_unit_type  u(6)
  if (!bit_buffer->ReadBits(&nal_unit_header.nal_unit_type, 6)) {
    return absl::nullopt;
  }

  // nuh_layer_id  u(6)
  if (!bit_buffer->ReadBits(&nal_unit_header.nuh_layer_id, 6)) {
    return absl::nullopt;
  }

  // nuh_temporal_id_plus1  u(3)
  if (!bit_buffer->ReadBits(&nal_unit_header.nuh_temporal_id_plus1, 3)) {
    return absl::nullopt;
  }

  return OptionalNalUnitHeader(nal_unit_header);
}

// Unpack RBSP and parse NAL Unit payload state from the supplied buffer.
absl::optional<H265NalUnitPayloadParser::NalUnitPayloadState>
H265NalUnitPayloadParser::ParseNalUnitPayload(
    const uint8_t* data, size_t length,
    uint32_t nal_unit_type,
    struct H265BitstreamParserState* bitstream_parser_state) {
  std::vector<uint8_t> unpacked_buffer = UnescapeRbsp(data, length);
  rtc::BitBuffer bit_buffer(unpacked_buffer.data(), unpacked_buffer.size());

  return ParseNalUnitPayload(&bit_buffer, nal_unit_type,
                             bitstream_parser_state);
}

absl::optional<H265NalUnitPayloadParser::NalUnitPayloadState>
H265NalUnitPayloadParser::ParseNalUnitPayload(
    rtc::BitBuffer* bit_buffer,
    uint32_t nal_unit_type,
    struct H265BitstreamParserState* bitstream_parser_state) {
  // H265 NAL Unit Payload (nal_unit()) parser.
  // Section 7.3.1.1 ("General NAL unit header syntax") of the H.265
  // standard for a complete description.
  NalUnitPayloadState nal_unit_payload;

  // payload (Table 7-1, Section 7.4.2.2)
  switch (nal_unit_type) {
    case TRAIL_N:
    case TRAIL_R:
    case TSA_N:
    case TSA_R:
    case STSA_N:
    case STSA_R:
    case RADL_N:
    case RADL_R:
    case RASL_N:
    case RASL_R:
      {
      // slice_segment_layer_rbsp()
      OptionalSliceSegmentLayer slice_segment_layer =
          H265SliceSegmentLayerParser::ParseSliceSegmentLayer(
              bit_buffer, nal_unit_type,
              bitstream_parser_state);
      if (slice_segment_layer != absl::nullopt) {
        nal_unit_payload.slice_segment_layer = *slice_segment_layer;
      }
      break;
      }
    case RSV_VCL_N10:
    case RSV_VCL_R11:
    case RSV_VCL_N12:
    case RSV_VCL_R13:
    case RSV_VCL_N14:
    case RSV_VCL_R15:
      // reserved, non-IRAP, sub-layer non-reference pictures
      break;
    case BLA_W_LP:
    case BLA_W_RADL:
    case BLA_N_LP:
    case IDR_W_RADL:
    case IDR_N_LP:
    case CRA_NUT:
      {
      // slice_segment_layer_rbsp()
      OptionalSliceSegmentLayer slice_segment_layer =
          H265SliceSegmentLayerParser::ParseSliceSegmentLayer(
              bit_buffer, nal_unit_type,
              bitstream_parser_state);
      if (slice_segment_layer != absl::nullopt) {
        nal_unit_payload.slice_segment_layer = *slice_segment_layer;
      }
      break;
      }
    case RSV_IRAP_VCL22:
    case RSV_IRAP_VCL23:
      // reserved, IRAP pictures
      break;
    case RSV_VCL24:
    case RSV_VCL25:
    case RSV_VCL26:
    case RSV_VCL27:
    case RSV_VCL28:
    case RSV_VCL29:
    case RSV_VCL30:
    case RSV_VCL31:
      // reserved, non-IRAP pictures
      break;
    case VPS_NUT:
      {
      // video_parameter_set_rbsp()
      OptionalVps vps = H265VpsParser::ParseVps(bit_buffer);
      if (vps != absl::nullopt) {
        nal_unit_payload.vps = *vps;
        uint32_t vps_id =
              nal_unit_payload.vps.vps_video_parameter_set_id;
        bitstream_parser_state->vps[vps_id] = nal_unit_payload.vps;
      }

      break;
      }
    case SPS_NUT:
      {
      // seq_parameter_set_rbsp()
      OptionalSps sps = H265SpsParser::ParseSps(bit_buffer);
      if (sps != absl::nullopt) {
        nal_unit_payload.sps = *sps;
        uint32_t sps_id =
            nal_unit_payload.sps.sps_seq_parameter_set_id;
        bitstream_parser_state->sps[sps_id] = nal_unit_payload.sps;
      }
      break;
      }
    case PPS_NUT:
      {
      // pic_parameter_set_rbsp()
      OptionalPps pps = H265PpsParser::ParsePps(bit_buffer);
      if (pps != absl::nullopt) {
        nal_unit_payload.pps = *pps;
        uint32_t pps_id =
            nal_unit_payload.pps.pps_pic_parameter_set_id;
        bitstream_parser_state->pps[pps_id] = nal_unit_payload.pps;
      }
      break;
      }
    case AUD_NUT:
      {
      // access_unit_delimiter_rbsp()
      OptionalAud aud = H265AudParser::ParseAud(bit_buffer);
      if (aud != absl::nullopt) {
        nal_unit_payload.aud = *aud;
      }
      break;
      }
    case EOS_NUT:
      // end_of_seq_rbsp()
      break;
    case EOB_NUT:
      // end_of_bitstream_rbsp()
      break;
    case FD_NUT:
      // filler_data_rbsp()
      break;
    case PREFIX_SEI_NUT:
      // sei_rbsp()
      break;
    case SUFFIX_SEI_NUT:
      // sei_rbsp()
      break;
    case RSV_NVCL41:
    case RSV_NVCL42:
    case RSV_NVCL43:
    case RSV_NVCL44:
    case RSV_NVCL45:
    case RSV_NVCL46:
    case RSV_NVCL47:
      // reserved
      break;
    default:
      // unspecified
      break;
  }

  return OptionalNalUnitPayload(nal_unit_payload);
}

void H265NalUnitParser::NalUnitState::fdump(
    XFILE outfp, int indent_level, bool add_offset, bool add_length) const {
  XPrintf(outfp, "nal_unit {");
  indent_level = indent_level_incr(indent_level);

  // nal unit offset (starting at NAL unit header)
  if (add_offset) {
    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "offset: 0x%08zx", offset);
  }

  // nal unit length (starting at NAL unit header)
  if (add_length) {
    fdump_indent_level(outfp, indent_level);
    XPrintf(outfp, "length: %zu", length);
  }

  // header
  fdump_indent_level(outfp, indent_level);
  nal_unit_header.fdump(outfp, indent_level);

  // payload
  fdump_indent_level(outfp, indent_level);
  nal_unit_payload.fdump(outfp, indent_level, nal_unit_header.nal_unit_type);

  indent_level = indent_level_decr(indent_level);
  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "}");
}

void H265NalUnitHeaderParser::NalUnitHeaderState::fdump(
    XFILE outfp, int indent_level) const {

  XPrintf(outfp, "nal_unit_header {");
  indent_level = indent_level_incr(indent_level);
  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "forbidden_zero_bit: %i", forbidden_zero_bit);
  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "nal_unit_type: %i", nal_unit_type);
  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "nuh_layer_id: %i", nuh_layer_id);
  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "nuh_temporal_id_plus1: %i", nuh_temporal_id_plus1);
  indent_level = indent_level_decr(indent_level);
  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "}");
}

void H265NalUnitPayloadParser::NalUnitPayloadState::fdump(
    XFILE outfp, int indent_level, uint32_t nal_unit_type) const {
  XPrintf(outfp, "nal_unit_payload {");
  indent_level = indent_level_incr(indent_level);

  fdump_indent_level(outfp, indent_level);
  switch (nal_unit_type) {
    case TRAIL_N:
    case TRAIL_R:
    case TSA_N:
    case TSA_R:
    case STSA_N:
    case STSA_R:
    case RADL_N:
    case RADL_R:
    case RASL_N:
    case RASL_R:
      slice_segment_layer.fdump(outfp, indent_level);
      break;
    case RSV_VCL_N10:
    case RSV_VCL_R11:
    case RSV_VCL_N12:
    case RSV_VCL_R13:
    case RSV_VCL_N14:
    case RSV_VCL_R15:
      // reserved, non-IRAP, sub-layer non-reference pictures
      break;
    case BLA_W_LP:
    case BLA_W_RADL:
    case BLA_N_LP:
    case IDR_W_RADL:
    case IDR_N_LP:
    case CRA_NUT:
      slice_segment_layer.fdump(outfp, indent_level);
      break;
    case RSV_IRAP_VCL22:
    case RSV_IRAP_VCL23:
      // reserved, IRAP pictures
      break;
    case RSV_VCL24:
    case RSV_VCL25:
    case RSV_VCL26:
    case RSV_VCL27:
    case RSV_VCL28:
    case RSV_VCL29:
    case RSV_VCL30:
    case RSV_VCL31:
      // reserved, non-IRAP pictures
      break;
    case VPS_NUT:
      vps.fdump(outfp, indent_level);
      break;
    case SPS_NUT:
      sps.fdump(outfp, indent_level);
      break;
    case PPS_NUT:
      pps.fdump(outfp, indent_level);
      break;
    case AUD_NUT:
      aud.fdump(outfp, indent_level);
      break;
    case EOS_NUT:
      // end_of_seq_rbsp()
      break;
    case EOB_NUT:
      // end_of_bitstream_rbsp()
      break;
    case FD_NUT:
      // filler_data_rbsp()
      break;
    case PREFIX_SEI_NUT:
      // sei_rbsp()
      break;
    case SUFFIX_SEI_NUT:
      // sei_rbsp()
      break;
    case RSV_NVCL41:
    case RSV_NVCL42:
    case RSV_NVCL43:
    case RSV_NVCL44:
    case RSV_NVCL45:
    case RSV_NVCL46:
    case RSV_NVCL47:
      // reserved
      break;
    default:
      // unspecified
      break;
  }

  indent_level = indent_level_decr(indent_level);
  fdump_indent_level(outfp, indent_level);
  XPrintf(outfp, "}");
}

}  // namespace h265nal
