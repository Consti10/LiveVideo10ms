
#include <sys/time.h>
#include <iostream>
#include <algorithm>
#include <random>
#include <list>
#include <chrono>
//#include <cxxopts.hpp>

#include <wifibroadcast/fec.hh>
#include <logging.hh>

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

// Create a buffer filled with random data of size sizeByes
std::vector<uint8_t> createRandomDataBuffer(const ssize_t sizeBytes){
  std::vector<uint8_t> buf(sizeBytes);
  for (uint32_t j = 0; j < sizeBytes; ++j) {
    buf[j] = rand() % 255;
  }
  return buf;
}

// Create N buffers filled with random data, where each buffer has size of sizeBytes
std::vector<std::vector<uint8_t>> createRandomDataBuffers(const ssize_t nBuffers,const ssize_t bufferSizeBytes){
    std::vector<std::vector<uint8_t>> ret;
    for(int i=0;i<nBuffers;i++){
        ret.push_back(createRandomDataBuffer(bufferSizeBytes));
    }
    return ret;
}

// Returns the cumulative size of all FECBlocks
std::size_t cumulativeSizeBytes(const std::vector<std::shared_ptr<FECBlock>>& blocks){
  std::size_t fecDataSize=0;
  for(const auto& block:blocks){
    fecDataSize+=block->block_size();
  }
  return fecDataSize;
}

// Send all the blocks ( = add them to the decoder )
// A lossy link is emulated by dropping some fec blocks
// @param RANDOMNESS: the smaller the value, the more packets are dropped
// If RANDOMNESS is a negative value no packets are dropped,if RANDOMNESS is 0 all packets are dropped
// If RANDOMNESS is 10 for example, roughly 10 % of the packets are dropped
std::size_t sendDataLossy(FECDecoder& dec,const std::vector<std::shared_ptr<FECBlock>>& blks,const int RANDOMNESS=10){
    std::size_t drop_count=0;
    for (std::shared_ptr<FECBlock> blk : blks) {
        bool dropPacket=false;
        if(RANDOMNESS>0 && ((rand() % RANDOMNESS) == 0)){
            dropPacket=true;
        }
        if(dropPacket){
            drop_count++;
        }else{
            dec.add_block(blk);
        }
    }
    return drop_count;
}


//same as above but also switch the order packets are sent
std::size_t sendDataLossyAndOutOfOrder(FECDecoder&dec,const std::vector<std::shared_ptr<FECBlock>>& blks){
    // we can copy a shared pointer without performance penalty
    std::list<std::shared_ptr<FECBlock>> workingData{blks.begin(),blks.end()};
    // shuffle the data a little bit
    std::list<std::shared_ptr<FECBlock>>::iterator item(workingData.begin());
    for(std::size_t i=0;i<workingData.size();i++){
        if ((rand() % 10) == 0) {
            // swap
            std::iter_swap(item,std::next(item,1));
            //MLOGD<<"Swapping "<<i<<" with "<<i+1;
        }
    }
    return sendDataLossy(dec,blks,-1);
}

// Cumulate all FEC blocks from decoder
std::vector<uint8_t> getDataFromDecoder(FECDecoder& dec){
    std::vector<uint8_t> obuf;
    obuf.reserve(1024*1024);
    for (std::shared_ptr<FECBlock> sblk = dec.get_block(); sblk; sblk = dec.get_block()) {
        MLOGD<<"Blk size "<<sblk->block_size()<<" Data size "<<sblk->data_length();
        std::copy(sblk->data(), sblk->data() + sblk->data_length(),
                  std::back_inserter(obuf));
    }
    return obuf;
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

    // Encode the test data
    std::vector<std::shared_ptr<FECBlock> > blks = enc.encode_buffer(buf);

    // emulate transmission over a lossy link
    const uint32_t drop_count=sendDataLossy(dec,blks,10);
    //drop_count=sendDataLossyAndOutOfOrder(dec,blks);

    const auto droppedPacketsPercentage=drop_count>0 ? ((float)blks.size() / (float)drop_count) : 0;

      // Decode it
    const auto obuf=getDataFromDecoder(dec);

    // Compare
    bool thisIterationFailded=false;
    if(compareBuffers(buf,obuf)!=0){
      ++failed;
      thisIterationFailded=true;
    }
    LOG_INFO << "Iteration: " << i << " Failed:"<<(thisIterationFailded ? "YES" : "NO")<<"  buffer size: " << buf_size
    << " created blocks: " << blks.size() << " createdBlocksSizeSum: " << cumulativeSizeBytes(blks)
    <<" droppedPackets: "<<drop_count<<" droppedPackets%: "<<droppedPacketsPercentage;
  }
  const auto nTestsPassed=iterations - failed;
  const auto testDataRate=8e-6 * static_cast<double>(bytes) / (cur_time() - start_time);
  return std::make_pair(nTestsPassed,testDataRate);
}

void run_test2(){

    const uint32_t block_size = 1024;
    float fec_ratio = 0.5f;

    FECBufferEncoder enc(block_size, fec_ratio);

    FECDecoder dec;

    // create a random data stream with packets of fixed size
    const auto packets=createRandomDataBuffers(100, 1024);
    for(int i=0; i < packets.size(); i++){
        const auto buff=packets.at(i);
        // Convert the current packet to FEC data
        std::vector<std::shared_ptr<FECBlock> > blks = enc.encode_buffer(buff);
        MLOGD<<"N created blocks "<<blks.size();

        for(const auto& blk:blks){
            MLOGD<<"Blk size "<<blk->block_size()<<" Data size "<<blk->data_length();
        }
        // Send fec blocks over emulated lossy link
        sendDataLossy(dec,blks,10);
        // Check if the decoder properly outputs the data buffer
        const auto obuf=getDataFromDecoder(dec);
        bool buffersAreEqual=true;
        if(compareBuffers(buff,obuf)!=0){
            buffersAreEqual=false;
        }
        MLOGD<<"Iteration "<<i<<" Equal "<<(buffersAreEqual ? "YES" : "NO");
    }
    MLOGD<<"Done";
}




#ifndef __ANDROID__
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
#endif

void test() {
  uint32_t iterations = 100;
  uint32_t block_size = 1024;
  float fec_ratio = 0.5f;

  FECBufferEncoder enc(block_size, fec_ratio);
  std::pair<uint32_t, double> passed = run_test(enc, block_size, iterations);
  LOG_INFO <<"XFECTest::test"<< passed.first << " tests passed out of " << iterations
           << "  " << passed.second << " Mbps";
}


#ifdef __ANDROID__

#include <jni.h>
//----------------------------------------------------JAVA bindings---------------------------------------------------------------
#define JNI_METHOD(return_type, method_name) \
  JNIEXPORT return_type JNICALL              \
      Java_constantin_video_core_TestFEC_##method_name


extern "C" {

JNI_METHOD(void , nativeTestFec)
(JNIEnv *env, jclass jclass1) {
    test();
    run_test2();
}

}
#endif

