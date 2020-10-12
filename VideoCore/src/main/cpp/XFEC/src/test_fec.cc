
#include <sys/time.h>

#include <iostream>

//#include <cxxopts.hpp>

#include <wifibroadcast/fec.hh>
#include <logging.hh>

#include <wifibroadcast/test_fec.h>

inline double cur_time() {
  struct timeval t;
  gettimeofday(&t, 0);
  return double(t.tv_sec) + double(t.tv_usec) * 1e-6;
}

// return 0 if buffers are equal
// return -1 if size does not match, else return n of bytes that do not match
int compareBuffers(const std::vector<uint8_t>& buff1,const std::vector<uint8_t>& buff2){
  if (buff1.size() != buff2.size()) {
    LOG_ERROR<<"Buffers are different sizes: " << buff1.size() << " != " << buff2.size();
    return -1;
  }
  int nLocationsDiffer=0;
  for (size_t j = 0; j < buff1.size(); ++j) {
    if (buff1[j] != buff2[j]) {
      LOG_ERROR<< "Buffers differ at location " << j << ": " << buff1[j] << " != " << buff2[j];
      nLocationsDiffer++;
    }
  }
  return nLocationsDiffer;
}

std::vector<uint8_t> createRandomDataBuffer(const ssize_t sizeBytes){
  std::vector<uint8_t> buf(sizeBytes);
  for (uint32_t j = 0; j < sizeBytes; ++j) {
    buf[j] = rand() % 255;
  }
  return buf;
}

//
std::vector<std::vector<uint8_t>> createRandomDataBuffers(const ssize_t sizeBytes,const ssize_t nBuffers){
    std::vector<std::vector<uint8_t>> ret;
    for(int i=0;i<nBuffers;i++){
        ret.push_back(std::move(createRandomDataBuffer(sizeBytes)));
    }
    return ret;
}

std::size_t sizeBytes(const std::vector<std::shared_ptr<FECBlock>>& blocks){
  std::size_t fecDataSize=0;
  for(const auto& block:blocks){
    fecDataSize+=block->block_size();
  }
  return fecDataSize;
}

std::pair<uint32_t, double> run_test(FECBufferEncoder &enc,const uint32_t max_block_size,
				     const uint32_t iterations) {
  FECDecoder dec;
  const uint32_t min_buffer_size = max_block_size * 6;
  const uint32_t max_buffer_size = max_block_size * 100;

  uint32_t failed = 0;
  size_t bytes = 0;
  double start_time = cur_time();
  for (uint32_t i = 0; i < iterations; ++i) {

    // Create a random buffer of data
    const uint32_t buf_size = min_buffer_size + rand() % (max_buffer_size - min_buffer_size);
    bytes += buf_size;
    auto buf=createRandomDataBuffer(buf_size);

    // Encode it
    std::vector<std::shared_ptr<FECBlock> > blks = enc.encode_buffer(buf);

    LOG_INFO << "Iteration: " << i << "  buffer size: " << buf_size
    <<"created blocks: "<<blks.size()<<" createdBlocksSizeSum: "<<sizeBytes(blks);

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
    if(compareBuffers(buf,obuf)!=0){
      ++failed;
    }
  }
  return std::make_pair(iterations - failed,
                        8e-6 * static_cast<double>(bytes) / (cur_time() - start_time));
}


void test2(FECBufferEncoder &enc,const uint32_t max_block_size,
           const uint32_t iterations){

    FECDecoder dec;
    const uint32_t min_buffer_size = max_block_size * 6;
    const uint32_t max_buffer_size = max_block_size * 100;


    const uint32_t buf_size = min_buffer_size + rand() % (max_buffer_size - min_buffer_size);
    auto input=createRandomDataBuffers(buf_size,1024);
    for(int i=0;i<input.size();i++){
        // Convert the packet to FEC data
        std::vector<std::shared_ptr<FECBlock> > blks = enc.encode_buffer(input.at(i));

        // emulate transmission over a lossy link
        for (std::shared_ptr<FECBlock> blk : blks) {
            if ((rand() % 10) != 0) {
                dec.add_block(blk->pkt_data(), blk->pkt_length());
            }
            // Check if there is an output block
            if(dec.get_block()){

            }
        }

    }


}

/*int main(int argc, char** argv) {

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
}*/

void XFECTest::test() {
  uint32_t iterations = 1000;
  uint32_t block_size = 32;
  float fec_ratio = 0.5f;

  FECBufferEncoder enc(block_size, fec_ratio);
  std::pair<uint32_t, double> passed = run_test(enc, block_size, iterations);
  LOG_INFO <<"XFECTest::test"<< passed.first << " tests passed out of " << iterations
           << "  " << passed.second << " Mbps";

}
