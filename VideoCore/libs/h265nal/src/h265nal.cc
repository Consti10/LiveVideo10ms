/*
 *  Copyright (c) Facebook, Inc. and its affiliates.
 */


#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "config.h"
#include "h265_bitstream_parser.h"
#include "h265_common.h"
#include "absl/types/optional.h"
#include "rtc_base/bit_buffer.h"


extern int optind;

// default option values
constexpr int kDefaultDebug = 0;

typedef struct arg_options {
  int debug;
  int integer;
  bool as_one_line;
  bool add_offset;
  bool add_length;
  char *str;
  char *infile;
  char *outfile;
  int nrem;
  char** rem;
} arg_options;


void usage(char *name) {
  fprintf(stderr, "usage: %s [options]\n", name);
  fprintf(stderr, "where options are:\n");
  fprintf(stderr, "\t-d:\t\tIncrease debug verbosity [%i]\n", kDefaultDebug);
  fprintf(stderr, "\t-q:\t\tZero debug verbosity\n");
  fprintf(stderr, "\t--as-one-line:\tSet as_one_line flag\n");
  fprintf(stderr, "\t--noas-one-line:\tReset as_one_line flag\n");
  fprintf(stderr, "\t--add-offset:\tSet add_offset flag\n");
  fprintf(stderr, "\t--noadd-offset:\tReset add_offset flag\n");
  fprintf(stderr, "\t--add-length:\tSet add_length flag\n");
  fprintf(stderr, "\t--noadd-length:\tReset add_length flag\n");
  fprintf(stderr, "\t--version:\t\tDump version number\n");
  fprintf(stderr, "\t-h:\t\tHelp\n");
  exit(-1);
}


// long options with no equivalent short option
enum {
  QUIET_OPTION = CHAR_MAX + 1,
  AS_ONE_LINE_FLAG_OPTION,
  NO_AS_ONE_LINE_FLAG_OPTION,
  ADD_OFFSET_FLAG_OPTION,
  NO_ADD_OFFSET_FLAG_OPTION,
  ADD_LENGTH_FLAG_OPTION,
  NO_ADD_LENGTH_FLAG_OPTION,
  VERSION_OPTION
};


arg_options *parse_args(int argc, char** argv) {
  int c;
  char *endptr;
  static arg_options options;

  // set default options
  options.debug = kDefaultDebug;
  options.as_one_line = true;
  options.add_offset = false;
  options.add_length = false;
  options.infile = nullptr;
  options.outfile = nullptr;

  // getopt_long stores the option index here
  int optindex = 0;

  // long options
  static struct option longopts[] = {
    // matching options to short options
    {"debug", no_argument, NULL, 'd'},
    // options without a short option
    {"quiet", no_argument, NULL, QUIET_OPTION},
    {"as-one-line", no_argument, NULL, AS_ONE_LINE_FLAG_OPTION},
    {"noas-one-line", no_argument, NULL, NO_AS_ONE_LINE_FLAG_OPTION},
    {"add-offset", no_argument, NULL, ADD_OFFSET_FLAG_OPTION},
    {"noadd-offset", no_argument, NULL, NO_ADD_OFFSET_FLAG_OPTION},
    {"add-length", no_argument, NULL, ADD_LENGTH_FLAG_OPTION},
    {"noadd-length", no_argument, NULL, NO_ADD_LENGTH_FLAG_OPTION},
    {"version", no_argument, NULL, VERSION_OPTION},
    {NULL, 0, NULL, 0}
  };

  // parse arguments
  while ((c = getopt_long(argc, argv, "d", longopts, &optindex)) != -1) {
    switch (c) {
      case 0:
        // long options that define flag
        // if this option set a flag, do nothing else now
        if (longopts[optindex].flag != NULL) {
          break;
        }
        printf("option %s", longopts[optindex].name);
        if (optarg) {
          printf(" with arg %s", optarg);
        }
        break;

      case 'd':
        options.debug += 1;
        break;

      case QUIET_OPTION:
        options.debug = 0;
        break;

      case AS_ONE_LINE_FLAG_OPTION:
        options.as_one_line = true;
        break;

      case NO_AS_ONE_LINE_FLAG_OPTION:
        options.as_one_line = false;
        break;

      case ADD_OFFSET_FLAG_OPTION:
        options.add_offset = true;
        break;

      case NO_ADD_OFFSET_FLAG_OPTION:
        options.add_offset = false;
        break;

      case ADD_LENGTH_FLAG_OPTION:
        options.add_length = true;
        break;

      case NO_ADD_LENGTH_FLAG_OPTION:
        options.add_length = false;
        break;

      case VERSION_OPTION:
        printf("version: %s\n", PROJECT_VER);
        exit(0);
        break;

      default:
        printf("Unsupported option: %c\n", c);
        usage(argv[0]);
    }
  }

  // require 2 extra parameters
  if ((argc - optind != 1) && (argc - optind != 2)) {
    fprintf(stderr, "need infile (outfile is optional)\n");
    usage(argv[0]);
    return nullptr;
  }

  options.infile = argv[optind];
  if (argc - optind == 2) {
    options.outfile = argv[optind + 1];
  }
  return &options;
}


int main(int argc, char **argv) {
  arg_options *options;

  // parse args
  options = parse_args(argc, argv);
  if (options == nullptr) {
    usage(argv[0]);
    exit(-1);
  }

  // print args
  if (options->debug > 1) {
    printf("options->integer = %i\n", options->integer);
    printf("options->debug = %i\n", options->debug);
    printf("options->infile = %s\n",
           (options->infile == NULL) ? "null" : options->infile);
    printf("options->outfile = %s\n",
           (options->outfile == NULL) ? "null" : options->outfile);
    for (uint32_t i = 0; i < options->nrem; ++i) {
      printf("options->rem[%i] = %s\n", i, options->rem[i]);
    }
  }

  // read infile
  // TODO(chemag): read the infile incrementally
  FILE* infp = fopen(options->infile, "rb");
  if (infp == nullptr) {
    // did not work
    fprintf(stderr, "Could not open input file: \"%s\"\n", options->infile);
    return -1;
  }
  fseek(infp, 0, SEEK_END);
  int64_t size = ftell(infp);
  fseek(infp, 0, SEEK_SET);
  // read file into buffer
  std::vector<uint8_t> buffer(size);
  fread(reinterpret_cast<char *>(buffer.data()), 1, size, infp);

  // create bitstream parser state
  absl::optional<h265nal::H265BitstreamParser::BitstreamState> bitstream;

  // parse the file
  bitstream = h265nal::H265BitstreamParser::ParseBitstream(
      buffer.data(), buffer.size());
  if (bitstream == absl::nullopt) {
    // did not work
    fprintf(stderr, "Could not init h265 bitstream parser\n");
    return -1;
  }
  bitstream->add_offset = options->add_offset;
  bitstream->add_length = options->add_length;

  // get outfile file descriptor
  FILE* outfp;
  if (options->outfile == nullptr ||
      (strlen(options->outfile) == 1 && options->outfile[0] == '-')) {
    // use stdout
    outfp = stdout;
  } else {
    outfp = fopen(options->outfile, "wb");
    if (outfp == nullptr) {
      // did not work
      fprintf(stderr, "Could not open output file: \"%s\"\n", options->outfile);
      return -1;
    }
  }

  int indent_level = (options->as_one_line) ? -1 : 0;
  //bitstream->fdump(outfp, indent_level);

  return 0;
}
