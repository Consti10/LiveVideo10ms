
#include <sys/time.h>

#include <iostream>

#include <cxxopts.hpp>

#include <wifibroadcast/fec.hh>
#include <logging.hh>

inline double cur_time() {
  struct timeval t;
  gettimeofday(&t, 0);
  return double(t.tv_sec) + double(t.tv_usec) * 1e-6;
}

std::pair<uint32_t, double> run_test(FECBufferEncoder &enc, uint32_t max_block_size,
				     uint32_t iterations) {
  FECDecoder dec;
  uint32_t min_buffer_size = max_block_size * 6;
  uint32_t max_buffer_size = max_block_size * 100;

  uint32_t failed = 0;
  size_t bytes = 0;
  double start_time = cur_time();
  for (uint32_t i = 0; i < iterations; ++i) {

    // Create a random buffer of data
    uint32_t buf_size = min_buffer_size + rand() % (max_buffer_size - min_buffer_size);
    bytes += buf_size;
    std::vector<uint8_t> buf(buf_size);
    for (uint32_t j = 0; j < buf_size; ++j) {
      buf[j] = rand() % 255;
    }
    std::cout << "Iteration: " << i << "  buffer size: " << buf_size << std::endl;

    // Encode it
    std::vector<std::shared_ptr<FECBlock> > blks = enc.encode_buffer(buf);
    std::cerr << blks.size() << " blocks created\n";

    // Decode it
    std::vector<uint8_t> obuf;
    uint32_t drop_count = 0;
    for (std::shared_ptr<FECBlock> blk : blks) {
      if ((rand() % 10) != 0) {
	dec.add_block(blk->pkt_data(), blk->pkt_length());
      }
    }
    for (std::shared_ptr<FECBlock> sblk = dec.get_block(); sblk; sblk = dec.get_block()) {
      std::copy(sblk->data(), sblk->data() + sblk->data_length(),
		std::back_inserter(obuf));
    }

    // Compare
    if (obuf.size() != buf.size()) {
      std::cerr << "Buffers are different sizes: " << obuf.size() << " != " << buf.size()
                << std::endl;
      ++failed;
    } else {
      for (size_t j = 0; j < buf.size(); ++j) {
	if (obuf[j] != buf[j]) {
	  std::cerr << "Buffers differ at location " << j << ": " << obuf[j] << " != " << buf[j]
                    << std::endl;
	  ++failed;
	  break;
	}
      }
    }
  }

  return std::make_pair(iterations - failed,
			8e-6 * static_cast<double>(bytes) / (cur_time() - start_time));
}


int main(int argc, char** argv) {

  cxxopts::Options options(argv[0], "Allowed options");
  options.add_options()
    ("h,help", "produce help message")
    ("iterations", "the number of iterations to run",
     cxxopts::value<uint32_t>()->default_value("1000"))
    ("block_size", "the block size to test",
     cxxopts::value<uint32_t>()->default_value("1400"))
    ("fec_ratio", "the FEC block / data block ratio",
     cxxopts::value<float>()->default_value("0.5"))
    ;

  auto result = options.parse(argc, argv);
  if (result.count("help")) {
    std::cout << options.help() << std::endl;
    return EXIT_SUCCESS;
  }
  uint32_t iterations = result["iterations"].as<uint32_t>();
  uint32_t block_size = result["block_size"].as<uint32_t>();
  float fec_ratio = result["fec_ratio"].as<float>();

  // Configure logging
  log4cpp::Appender *appender1 = new log4cpp::OstreamAppender("console", &std::cout);
  appender1->setLayout(new log4cpp::BasicLayout());
  log4cpp::Category& root = log4cpp::Category::getRoot();
  root.setPriority(log4cpp::Priority::DEBUG);
  root.addAppender(appender1);
  
  FECBufferEncoder enc(block_size, fec_ratio);
  std::pair<uint32_t, double> passed = run_test(enc, block_size, iterations);
  LOG_INFO << passed.first << " tests passed out of " << iterations
           << "  " << passed.second << " Mbps";
  
  return (passed.first == iterations) ? EXIT_SUCCESS : EXIT_FAILURE;
}
