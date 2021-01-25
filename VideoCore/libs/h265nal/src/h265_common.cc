/*
 *  Copyright (c) Facebook, Inc. and its affiliates.
 */

#include "h265_common.h"

#include <stdio.h>

#include <cstdint>

namespace h265nal {

std::vector<uint8_t> UnescapeRbsp(const uint8_t* data, size_t length) {
  std::vector<uint8_t> out;
  out.reserve(length);

  for (size_t i = 0; i < length;) {
    // Be careful about over/underflow here. byte_length_ - 3 can underflow, and
    // i + 3 can overflow, but byte_length_ - i can't, because i < byte_length_
    // above, and that expression will produce the number of bytes left in
    // the stream including the byte at i.
    if (length - i >= 3 && data[i] == 0x00 && data[i + 1] == 0x00 &&
        data[i + 2] == 0x03) {
      // Two RBSP bytes.
      out.push_back(data[i++]);
      out.push_back(data[i++]);
      // Skip the emulation byte.
      i++;
    } else {
      // Single rbsp byte.
      out.push_back(data[i++]);
    }
  }
  return out;
}

// Syntax functions and descriptors) (Section 7.2)
bool byte_aligned(rtc::BitBuffer *bit_buffer) {
  // If the current position in the bitstream is on a byte boundary, i.e.,
  // the next bit in the bitstream is the first bit in a byte, the return
  // value of byte_aligned() is equal to TRUE.
  // Otherwise, the return value of byte_aligned() is equal to FALSE.
  size_t out_byte_offset, out_bit_offset;
  bit_buffer->GetCurrentOffset(&out_byte_offset, &out_bit_offset);

  return (out_bit_offset == 0);
}

bool more_rbsp_data(rtc::BitBuffer *bit_buffer) {
  // If there is no more data in the raw byte sequence payload (RBSP), the
  // return value of more_rbsp_data() is equal to FALSE.
  // Otherwise, the RBSP data are searched for the last (least significant,
  // right-most) bit equal to 1 that is present in the RBSP. Given the
  // position of this bit, which is the first bit (rbsp_stop_one_bit) of
  // the rbsp_trailing_bits() syntax structure, the following applies:
  // - If there is more data in an RBSP before the rbsp_trailing_bits()
  // syntax structure, the return value of more_rbsp_data() is equal to TRUE.
  // - Otherwise, the return value of more_rbsp_data() is equal to FALSE.
  // The method for enabling determination of whether there is more data
  // in the RBSP is specified by the application (or in Annex B for
  // applications that use the byte stream format).
  // TODO(chemag): fix more_rbsp_data()
  return false;
}

bool rbsp_trailing_bits(rtc::BitBuffer *bit_buffer) {
  uint32_t bits_tmp;

  // rbsp_stop_one_bit  f(1) // equal to 1
  if (!bit_buffer->ReadBits(&bits_tmp, 1)) {
    return false;
  }
  if (bits_tmp != 1) {
    return false;
  }

  while (!byte_aligned(bit_buffer)) {
    // rbsp_alignment_zero_bit  f(1) // equal to 0
    if (!bit_buffer->ReadBits(&bits_tmp, 1)) {
      return false;
    }
    if (bits_tmp != 0) {
      return false;
    }
  }
  return true;
}

int indent_level_incr(int indent_level) {
  return (indent_level == -1) ? -1 : (indent_level + 1);
}

int indent_level_decr(int indent_level) {
  return (indent_level == -1) ? -1 : (indent_level - 1);
}

void fdump_indent_level(FILE* outfp, int indent_level) {
  if (indent_level == -1) {
    // no indent
    fprintf(outfp, " ");
    return;
  }
  fprintf(outfp, "\n");
  fprintf(outfp, "%*s", 2 * indent_level, "");
}

void fdump_indent_level(std::stringstream& ss, int indent_level) {
  if (indent_level == -1) {
    // no indent
     ss<<" ";
    return;
  }
  ss<<"\n";
}


}  // namespace h265nal
