/*
 *  Copyright (c) Facebook, Inc. and its affiliates.
 */

#pragma once

#include <stdio.h>

#include <cstdint>
#include <vector>
#include <sstream>

#include "rtc_base/bit_buffer.h"


namespace h265nal {

enum NalUnitType : uint8_t {
  TRAIL_N = 0,
  TRAIL_R = 1,
  TSA_N = 2,
  TSA_R = 3,
  STSA_N = 4,
  STSA_R = 5,
  RADL_N = 6,
  RADL_R = 7,
  RASL_N = 8,
  RASL_R = 9,
  RSV_VCL_N10 = 10,
  RSV_VCL_R11 = 11,
  RSV_VCL_N12 = 12,
  RSV_VCL_R13 = 13,
  RSV_VCL_N14 = 14,
  RSV_VCL_R15 = 15,
  BLA_W_LP = 16,
  BLA_W_RADL = 17,
  BLA_N_LP = 18,
  IDR_W_RADL = 19,
  IDR_N_LP = 20,
  CRA_NUT = 21,
  RSV_IRAP_VCL22 = 22,
  RSV_IRAP_VCL23 = 23,
  RSV_VCL24 = 24,
  RSV_VCL25 = 25,
  RSV_VCL26 = 26,
  RSV_VCL27 = 27,
  RSV_VCL28 = 28,
  RSV_VCL29 = 29,
  RSV_VCL30 = 30,
  RSV_VCL31 = 31,
  VPS_NUT = 32,
  SPS_NUT = 33,
  PPS_NUT = 34,
  AUD_NUT = 35,
  EOS_NUT = 36,
  EOB_NUT = 37,
  FD_NUT = 38,
  PREFIX_SEI_NUT = 39,
  SUFFIX_SEI_NUT = 40,
  RSV_NVCL41 = 41,
  RSV_NVCL42 = 42,
  RSV_NVCL43 = 43,
  RSV_NVCL44 = 44,
  RSV_NVCL45 = 45,
  RSV_NVCL46 = 46,
  RSV_NVCL47 = 47,
  // 48-63: unspecified
  AP = 48,
  FU = 49,
};

// Methods for parsing RBSP. See section 7.4.1 of the H265 spec.
//
// Decoding is simply a matter of finding any 00 00 03 sequence and removing
// the 03 byte (emulation byte).

// Remove any emulation byte escaping from a buffer. This is needed for
// byte-stream format packetization (e.g. Annex B data), but not for
// packet-stream format packetization (e.g. RTP payloads).
std::vector<uint8_t> UnescapeRbsp(const uint8_t* data, size_t length);

// Syntax functions and descriptors) (Section 7.2)
bool byte_aligned(rtc::BitBuffer *bit_buffer);
bool more_rbsp_data(rtc::BitBuffer *bit_buffer);
bool rbsp_trailing_bits(rtc::BitBuffer *bit_buffer);

// fdump() indentation help
int indent_level_incr(int indent_level);
int indent_level_decr(int indent_level);

#include <string>
#include <sstream>
static inline void XPrintf(std::stringstream& ss,const std::string fmt, ...) {
  int size = ((int)fmt.size()) * 2 + 50;   // Use a rubric appropriate for your code
  std::string str;
  va_list ap;
  while (1) {     // Maximum two passes on a POSIX system...
    str.resize(size);
    va_start(ap, fmt);
    int n = vsnprintf((char *)str.data(), size, fmt.c_str(), ap);
    va_end(ap);
    if (n > -1 && n < size) {  // Everything worked
      str.resize(n);
      ss<<str;
      return;
    }
    if (n > -1)  // Needed size returned
      size = n + 1;   // For null char
    else
      size *= 2;      // Guess at a larger size (OS specific)
  }
  ss<<str;
}

static inline void XPrintf(FILE* fp,const char* fmt,...){
  va_list args;
  va_start(args, fmt);
  fprintf(fp,fmt,args);
  va_end(args);
}

//using XFILE=FILE*;
using XFILE=std::stringstream&;

void fdump_indent_level(FILE* outfp, int indent_level);
void fdump_indent_level(std::stringstream& ss, int indent_level);
}  // namespace h265nal
